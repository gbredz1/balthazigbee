#include "Zigbee.h"

#include <esp_check.h>
#include <zcl/esp_zigbee_zcl_power_config.h>

#include "AppEvents.h"
#include "ZigbeeMapping.hpp"

namespace Zigbee {

constexpr auto TAG = "BALTHAZAR_ZIGBEE";
constexpr uint8_t ZB_ENDPOINT = 0x01;

inline static Zigbee *_instance = nullptr;
Zigbee::Zigbee() {
    if (_instance != nullptr) {
        ESP_LOGE(TAG, "duplicate instance !!!");
        abort();
    }
    _instance = this;
}
Zigbee::~Zigbee() {
    _instance = nullptr;
}

auto Zigbee::setup(const esp_event_loop_handle_t loop_handle, State *state) -> Zigbee & {
    this->event_loop = loop_handle;
    this->state = state;

    esp_zb_platform_config_t config;
    config.radio_config.radio_mode = ZB_RADIO_MODE_NATIVE;
    config.host_config.host_connection_mode = ZB_HOST_CONNECTION_MODE_NONE;

    ESP_ERROR_CHECK(esp_zb_platform_config(&config));
    return *this;
}
auto Zigbee::start() -> Zigbee & {
    // Network config
    esp_zb_cfg_t network_config;
    network_config.esp_zb_role = ESP_ZB_DEVICE_TYPE_ED;
    network_config.install_code_policy = false;
    network_config.nwk_cfg.zed_cfg.ed_timeout = ESP_ZB_ED_AGING_TIMEOUT_64MIN;
    network_config.nwk_cfg.zed_cfg.keep_alive = 3000; // 3 secondes
    esp_zb_init(&network_config);
    esp_zb_set_rx_on_when_idle(false); // reset MAC Capability -> https://github.com/espressif/esp-zigbee-sdk/issues/7

    // Cluster config
    esp_zb_cluster_list_t *cluster_list = esp_zb_zcl_cluster_list_create();
    ESP_ERROR_CHECK(add_basic_cluster(cluster_list));
    ESP_ERROR_CHECK(add_identify_cluster(cluster_list));
    ESP_ERROR_CHECK(add_power_config_cluster(cluster_list));
    ESP_ERROR_CHECK(add_metering_cluster(cluster_list));

    // Endpoint config
    esp_zb_ep_list_t *endpoint_list = esp_zb_ep_list_create();
    constexpr esp_zb_endpoint_config_t endpoint = {
        .endpoint = ZB_ENDPOINT,
        .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
        .app_device_id = ESP_ZB_HA_METER_INTERFACE_DEVICE_ID,
        .app_device_version = 0}; // TODO: set version
    ESP_ERROR_CHECK(esp_zb_ep_list_add_ep(endpoint_list, cluster_list, endpoint));

    // Register the device
    ESP_ERROR_CHECK(esp_zb_device_register(endpoint_list));

    // Start
    esp_zb_core_action_handler_register(
        [](esp_zb_core_action_callback_id_t callback_id, const void *message) {
            return _instance->action(callback_id, message);
        });
    esp_zb_identify_notify_handler_register(
        ZB_ENDPOINT,
        [](uint8_t identify_on) {
            _instance->action_identify_notify(identify_on);
        });

    ESP_ERROR_CHECK(esp_zb_set_primary_network_channel_set(ESP_ZB_TRANSCEIVER_ALL_CHANNELS_MASK));
    ESP_ERROR_CHECK(esp_zb_start(false));

    return *this;
}
auto Zigbee::loop() -> void {
    esp_zb_stack_main_loop();
}
auto Zigbee::reset() -> void {
    esp_zb_factory_reset();
}

auto Zigbee::update_then_report(const ClusterAttributeTypes &type) const -> void {
    update(type);
    report(type);
}
auto Zigbee::update(const ClusterAttributeTypes &type) const -> void {
    const auto &attribute_mapping = k_attribute_mappings[type];

    esp_zb_lock_acquire(portMAX_DELAY);
    const auto status = esp_zb_zcl_set_attribute_val(ZB_ENDPOINT,
                                                     attribute_mapping.cluster_id,
                                                     ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
                                                     attribute_mapping.attr_id,
                                                     attribute_mapping.value_p(*state),
                                                     false);

    esp_zb_lock_release();
    if (status != ESP_ZB_ZCL_STATUS_SUCCESS) {
        ESP_LOGE(TAG, "Updating value of battery percentage: %d", status);
    }
}
auto Zigbee::report(const ClusterAttributeTypes &type) -> void {
    const auto &attribute_mapping = k_attribute_mappings[type];

    esp_zb_zcl_report_attr_cmd_t report_attr_cmd;
    report_attr_cmd.zcl_basic_cmd.src_endpoint = ZB_ENDPOINT;
    report_attr_cmd.address_mode = ESP_ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT;
    report_attr_cmd.direction = ESP_ZB_ZCL_CMD_DIRECTION_TO_CLI;
    report_attr_cmd.clusterID = attribute_mapping.cluster_id;
    report_attr_cmd.attributeID = attribute_mapping.attr_id;

    esp_zb_lock_acquire(portMAX_DELAY);
    const auto status = esp_zb_zcl_report_attr_cmd_req(&report_attr_cmd);
    esp_zb_lock_release();

    if (status != ESP_ZB_ZCL_STATUS_SUCCESS) {
        ESP_LOGE(TAG, "@SetAttribute failed!: %04x|%04x (status: 0x%02x)",
                 report_attr_cmd.clusterID,
                 report_attr_cmd.attributeID,
                 status);
    }
}

// Clusters
auto Zigbee::add_basic_cluster(esp_zb_cluster_list_t *cluster_list) -> esp_err_t {
    esp_zb_basic_cluster_cfg_t config;
    config.zcl_version = ESP_ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
    config.power_source = 0x03; // Battery
    esp_zb_attribute_list_t *cluster = esp_zb_basic_cluster_create(&config);

    uint32_t app_version = 0x0001;
    uint32_t stack_version = 0x0002;
    uint32_t hw_version = 0x0001;
    uint8_t manufacturer_name[] = {7, 'g', 'b', 'r', 'e', 'd', 'z', '1'};
    uint8_t model_identifier[] = {9, 'B', 'a', 'l', 't', 'h', 'a', 'Z', 'a', 'r'};
    uint8_t date_code[] = {8, '2', '0', '2', '3', '1', '0', '0', '6'};
    uint8_t sw_build[] = {5, '0', '.', '0', '.', '1'};
    esp_zb_basic_cluster_add_attr(cluster, ESP_ZB_ZCL_ATTR_BASIC_APPLICATION_VERSION_ID, &app_version);
    esp_zb_basic_cluster_add_attr(cluster, ESP_ZB_ZCL_ATTR_BASIC_STACK_VERSION_ID, &stack_version);
    esp_zb_basic_cluster_add_attr(cluster, ESP_ZB_ZCL_ATTR_BASIC_HW_VERSION_ID, &hw_version);
    esp_zb_basic_cluster_add_attr(cluster, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID, manufacturer_name);
    esp_zb_basic_cluster_add_attr(cluster, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID, model_identifier);
    esp_zb_basic_cluster_add_attr(cluster, ESP_ZB_ZCL_ATTR_BASIC_DATE_CODE_ID, date_code);
    esp_zb_basic_cluster_add_attr(cluster, ESP_ZB_ZCL_ATTR_BASIC_SW_BUILD_ID, sw_build);

    return esp_zb_cluster_list_add_basic_cluster(cluster_list, cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
}
auto Zigbee::add_identify_cluster(esp_zb_cluster_list_t *cluster_list) -> esp_err_t {
    esp_zb_identify_cluster_cfg_t config;
    config.identify_time = ESP_ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;

    esp_zb_attribute_list_t *cluster = esp_zb_identify_cluster_create(&config);

    return esp_zb_cluster_list_add_identify_cluster(cluster_list, cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
}
auto Zigbee::add_power_config_cluster(esp_zb_cluster_list_t *cluster_list) const -> esp_err_t {
    esp_zb_power_config_cluster_cfg_t config;
    esp_zb_attribute_list_t *cluster = esp_zb_power_config_cluster_create(&config);

    // uint8_t battery_voltage = 33; // 0x01 = 100mV, 0x02 = 200mV, 0xFF = invalid or unknown
    // esp_zb_power_config_cluster_add_attr(cluster, ESP_ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_VOLTAGE_ID, &battery_voltage);

    // uint8_t battery_percent_remaining = 150; // 0x00 = 0%, 0x64|100=50%, 0xC8|200=100%
    esp_zb_cluster_add_attr(cluster,
                            ESP_ZB_ZCL_CLUSTER_ID_POWER_CONFIG,
                            ESP_ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_PERCENTAGE_REMAINING_ID,
                            ESP_ZB_ZCL_ATTR_TYPE_U8,
                            ESP_ZB_ZCL_ATTR_ACCESS_READ_ONLY | ESP_ZB_ZCL_ATTR_ACCESS_REPORTING,
                            &state->battery_percent_remaining);

    return esp_zb_cluster_list_add_power_config_cluster(cluster_list, cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
}
auto Zigbee::add_metering_cluster(esp_zb_cluster_list_t *cluster_list) const -> esp_err_t {
    esp_zb_attribute_list_t *cluster = esp_zb_zcl_attr_list_create(ESP_ZB_ZCL_CLUSTER_ID_METERING);

    esp_zb_cluster_add_attr(cluster,
                            ESP_ZB_ZCL_CLUSTER_ID_METERING,
                            ESP_ZB_ZCL_ATTR_METERING_CURRENT_SUMMATION_DELIVERED_ID,
                            ESP_ZB_ZCL_ATTR_TYPE_U48,
                            ESP_ZB_ZCL_ATTR_ACCESS_READ_WRITE | ESP_ZB_ZCL_ATTR_ACCESS_REPORTING,
                            &state->summation_delivered);

    // 5 digit + 3 decimal
    uint8_t formating = ESP_ZB_ZCL_METERING_FORMATTING_SET(false, 5, 3);
    esp_zb_cluster_add_attr(cluster,
                            ESP_ZB_ZCL_CLUSTER_ID_METERING,
                            ESP_ZB_ZCL_ATTR_METERING_SUMMATION_FORMATTING_ID,
                            ESP_ZB_ZCL_ATTR_TYPE_U16,
                            ESP_ZB_ZCL_ATTR_ACCESS_READ_ONLY,
                            &formating);

    auto unit = ESP_ZB_ZCL_METERING_UNIT_M3_M3H_BINARY;
    esp_zb_cluster_add_attr(cluster,
                            ESP_ZB_ZCL_CLUSTER_ID_METERING,
                            ESP_ZB_ZCL_ATTR_METERING_UNIT_OF_MEASURE_ID,
                            ESP_ZB_ZCL_ATTR_TYPE_8BIT_ENUM,
                            ESP_ZB_ZCL_ATTR_ACCESS_READ_ONLY,
                            &unit);

    auto type = ESP_ZB_ZCL_METERING_GAS_METERING;
    esp_zb_cluster_add_attr(cluster,
                            ESP_ZB_ZCL_CLUSTER_ID_METERING,
                            ESP_ZB_ZCL_ATTR_METERING_METERING_DEVICE_TYPE_ID,
                            ESP_ZB_ZCL_ATTR_TYPE_8BIT_ENUM,
                            ESP_ZB_ZCL_ATTR_ACCESS_READ_ONLY,
                            &type);

    esp_zb_uint24_t multiplier = {.low = 1, .high = 0};
    esp_zb_cluster_add_attr(cluster,
                            ESP_ZB_ZCL_CLUSTER_ID_METERING,
                            ESP_ZB_ZCL_ATTR_METERING_MULTIPLIER_ID,
                            ESP_ZB_ZCL_ATTR_TYPE_U24,
                            ESP_ZB_ZCL_ATTR_ACCESS_READ_ONLY,
                            &multiplier);

    esp_zb_uint24_t divisor = {.low = 1000, .high = 0};
    esp_zb_cluster_add_attr(cluster,
                            ESP_ZB_ZCL_CLUSTER_ID_METERING,
                            ESP_ZB_ZCL_ATTR_METERING_DIVISOR_ID,
                            ESP_ZB_ZCL_ATTR_TYPE_U24,
                            ESP_ZB_ZCL_ATTR_ACCESS_READ_ONLY,
                            &divisor);

    return esp_zb_cluster_list_add_metering_cluster(cluster_list, cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
}

// Actions handlers
auto Zigbee::action(const esp_zb_core_action_callback_id_t callback_id, const void *message) -> esp_err_t {
    esp_err_t result = ESP_OK;

    switch (callback_id) {
        case ESP_ZB_CORE_SET_ATTR_VALUE_CB_ID: {
            const auto *const value_message = static_cast<const esp_zb_zcl_set_attr_value_message_t *>(message);
            result = this->action_set_attribute(value_message);
        } break;

        case ESP_ZB_CORE_CMD_DEFAULT_RESP_CB_ID: {
            const auto *const response_message = static_cast<const esp_zb_zcl_cmd_default_resp_message_s *>(message);
            ESP_LOGI(TAG, "Default Response to cmd: 0x%02x (status: 0x%02x)",
                     response_message->resp_to_cmd,
                     response_message->status_code);
        } break;

        default:
            ESP_LOGW(TAG, "Unhandled Zigbee action callback (id: 0x%x)", callback_id);
            break;
    }

    return result;
}

auto Zigbee::action_set_attribute(const esp_zb_zcl_set_attr_value_message_t *message) -> esp_err_t {
    ESP_RETURN_ON_FALSE(message, ESP_FAIL, TAG, "Empty message");
    ESP_RETURN_ON_FALSE(message->info.status == ESP_ZB_ZCL_STATUS_SUCCESS,
                        ESP_ERR_INVALID_ARG,
                        TAG,
                        "Received message: error status(%d)", message->info.status);

    ESP_LOGI(TAG, "Received message: endpoint(%d), cluster(0x%x), attribute(0x%x), data size(%d) data type(0x%x)",
             message->info.dst_endpoint,
             message->info.cluster,
             message->attribute.id,
             message->attribute.data.size,
             message->attribute.data.type);

    if (message->info.dst_endpoint == ZB_ENDPOINT &&
        message->info.cluster == ESP_ZB_ZCL_CLUSTER_ID_METERING &&
        message->attribute.id == ESP_ZB_ZCL_ATTR_METERING_CURRENT_SUMMATION_DELIVERED_ID &&
        message->attribute.data.type == ESP_ZB_ZCL_ATTR_TYPE_U48) {

        ESP_ERROR_CHECK(esp_event_post_to(event_loop,
                                          APP_EVENTS,
                                          EV_ZB_SET_SUMMATION_DELIVERED,
                                          message->attribute.data.value,
                                          message->attribute.data.size,
                                          portMAX_DELAY));
    }

    return ESP_OK;
}
auto Zigbee::action_identify_notify(uint8_t identify_on) const -> void {
    ESP_ERROR_CHECK(esp_event_post_to(event_loop,
                                      APP_EVENTS,
                                      EV_ZB_IDENTIFY,
                                      &identify_on,
                                      1,
                                      portMAX_DELAY));
}

// Signals
auto Zigbee::signal(const esp_zb_app_signal_t *signal) -> void {
    const auto err_status = signal->esp_err_status;
    const auto sig_type = static_cast<esp_zb_app_signal_type_t>(*signal->p_app_signal);

    switch (sig_type) {
        case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
            signal_skip_startup();
            break;

        case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
            signal_boot(err_status);
            break;

        case ESP_ZB_BDB_SIGNAL_STEERING:
            signal_steering(err_status);
            break;

        case ESP_ZB_COMMON_SIGNAL_CAN_SLEEP:
            // ESP_LOGW(TAG, "ZDO signal: %s (0x%x), status: %s",
            //          esp_zb_zdo_signal_to_string(sig_type),
            //          sig_type,
            //          esp_err_to_name(err_status));
            break;

        default:
            ESP_LOGW(TAG, "ZDO signal: %s (0x%x), status: %s",
                     esp_zb_zdo_signal_to_string(sig_type),
                     sig_type,
                     esp_err_to_name(err_status));
            break;
    }
}
auto Zigbee::signal_skip_startup() -> void {
    ESP_LOGI(TAG, "Zigbee stack initialized");
    esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_INITIALIZATION);
}
auto Zigbee::signal_boot(const esp_err_t err) -> void {
    if (err != ESP_OK) {
        /* commissioning failed */
        ESP_LOGW(TAG, "Failed to initialize Zigbee stack (status: %s)", esp_err_to_name(err));
        // esp_zb_scheduler_alarm((esp_zb_callback_t)bdb_start_top_level_commissioning_cb, ESP_ZB_BDB_MODE_NETWORK_STEERING, 1000);
        return;
    }

    ESP_LOGI(TAG, "Device started up in %s factory-reset mode", esp_zb_bdb_is_factory_new() ? "" : "non");
    if (esp_zb_bdb_is_factory_new()) {
        ESP_LOGI(TAG, "Start network steering");
        esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
        ESP_ERROR_CHECK(esp_event_post_to(event_loop,
                                          APP_EVENTS,
                                          EV_ZB_STEERING_START,
                                          nullptr,
                                          0,
                                          portMAX_DELAY));
    } else {
        // zb_deep_sleep_start();
        ESP_ERROR_CHECK(esp_event_post_to(event_loop,
                                          APP_EVENTS,
                                          EV_ZB_READY,
                                          nullptr,
                                          0,
                                          portMAX_DELAY));
        ESP_LOGI(TAG, "Device rebooted");
    }
}
auto Zigbee::signal_steering(const esp_err_t err) -> void {
    if (err == ESP_OK) {
        esp_zb_ieee_addr_t extended_pan_id;
        esp_zb_get_extended_pan_id(extended_pan_id);
        ESP_LOGI(TAG, "Joined network successfully (Extended PAN ID: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x, PAN ID: 0x%04hx, Channel:%d)",
                 extended_pan_id[7], extended_pan_id[6], extended_pan_id[5], extended_pan_id[4],
                 extended_pan_id[3], extended_pan_id[2], extended_pan_id[1], extended_pan_id[0],
                 esp_zb_get_pan_id(), esp_zb_get_current_channel());

        // esp_zb_zdo_ieee_addr_req_param_t ieee_req;
        // ieee_req.addr_of_interest = 0;
        // ieee_req.dst_nwk_addr = 0;
        // ieee_req.request_type = 0;
        // ieee_req.start_index = 0;

        // esp_zb_zdo_ieee_addr_req(
        //     &ieee_req, [](esp_zb_zdp_status_t zdo_status, esp_zb_ieee_addr_t ieee_addr, void *user_ctx) {
        //         if (zdo_status != ESP_ZB_ZDP_STATUS_SUCCESS) {
        //             ESP_LOGE(TAG, "zdo_status: 0x%02x", zdo_status);
        //             return;
        //         }

        //         ESP_LOGI(TAG, "IEEE address: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
        //                  ieee_addr[7], ieee_addr[6], ieee_addr[5], ieee_addr[4],
        //                  ieee_addr[3], ieee_addr[2], ieee_addr[1], ieee_addr[0]);
        //     },
        //     NULL);

    } else {
        ESP_LOGI(TAG, "Network steering was not successful (status: %s)", esp_err_to_name(err));
        // esp_zb_scheduler_alarm((esp_zb_callback_t)bdb_start_top_level_commissioning_cb, ESP_ZB_BDB_MODE_NETWORK_STEERING, 1000);
    }

    bool data = err == ESP_OK;
    ESP_ERROR_CHECK(esp_event_post_to(event_loop,
                                      APP_EVENTS,
                                      EV_ZB_STEERING_DONE,
                                      &data,
                                      1,
                                      portMAX_DELAY));
}

} // namespace Zigbee

/**  esp zboss  **/
void esp_zb_app_signal_handler(const esp_zb_app_signal_t *signal_s) {
    if (Zigbee::_instance != nullptr) {
        Zigbee::_instance->signal(signal_s);
    }
}
