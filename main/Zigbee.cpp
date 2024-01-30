#include "Zigbee.h"

#include "AppEvents.h"
#include <esp_check.h>
#include <ha/esp_zigbee_ha_standard.h>

static const char *TAG = "BALTHAZAR_ZIGBEE";
static const uint8_t ZB_ENDPOINT = 0x01;

/* Ugly wrap */
Zigbee *z = nullptr;
void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct) {
    if (z == nullptr) {
        return;
    }

    z->signal(signal_struct);
}
/* Ugly wrap End */

Zigbee::Zigbee() {
    // There can be only one !
    if (z != nullptr) {
        delete (z);
    }
    z = this;
}
Zigbee::~Zigbee() {
    z = nullptr;
}

auto Zigbee::setup(esp_event_loop_handle_t loop_handle) -> Zigbee & {
    this->event_loop = loop_handle;

    esp_zb_platform_config_t config = {
        .radio_config = {
            .radio_mode = RADIO_MODE_NATIVE,
            .radio_uart_config = {},
        },
        .host_config = {
            .host_connection_mode = HOST_CONNECTION_MODE_NONE,
            .host_uart_config = {},
        },
    };

    ESP_ERROR_CHECK(esp_zb_platform_config(&config));

    return *this;
}
auto Zigbee::start() -> Zigbee & {
    // Network config
    esp_zb_cfg_t network_config = {
        .esp_zb_role = ESP_ZB_DEVICE_TYPE_ED,
        .install_code_policy = false, // enable the install code policy for security
        .nwk_cfg = {
            .zed_cfg = {
                .ed_timeout = ESP_ZB_ED_AGING_TIMEOUT_64MIN,
                .keep_alive = 3000, // 3000 millisecond
            }}};
    esp_zb_init(&network_config);

    // Cluster list
    esp_zb_cluster_list_t *cluster_list = esp_zb_zcl_cluster_list_create();
    esp_zb_cluster_list_add_basic_cluster(cluster_list, create_basic_cluster(), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_cluster_list_add_identify_cluster(cluster_list, create_identify_cluster(), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_cluster_list_add_power_config_cluster(cluster_list, create_power_config_cluster(), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_cluster_list_add_time_cluster(cluster_list, create_time_cluster(), ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE);
    esp_zb_cluster_list_add_metering_cluster(cluster_list, create_metering_cluster(), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

    // Endpoint list
    esp_zb_ep_list_t *endpoint_list = esp_zb_ep_list_create();
    ESP_ERROR_CHECK(esp_zb_ep_list_add_ep(endpoint_list, cluster_list, ZB_ENDPOINT, ESP_ZB_AF_HA_PROFILE_ID, ESP_ZB_HA_METER_INTERFACE_DEVICE_ID));
    ESP_ERROR_CHECK(esp_zb_device_register(endpoint_list));

    // Start
    esp_zb_core_action_handler_register(
        [](esp_zb_core_action_callback_id_t callback_id,
           const void *message) -> esp_err_t {
            if (z == nullptr) {
                return ESP_FAIL;
            }
            return z->action(callback_id, message);
        });
    esp_zb_identify_notify_handler_register(
        ZB_ENDPOINT,
        [](uint8_t identify_on) {
            if (z == nullptr) {
                return;
            }
            z->action_identify_notify(identify_on);
        });

    // W (10103) ESP_ZIGBEE_CORE: Device callback id(0x6) has not been handled ????
    //
    esp_zb_device_cb_id_handler_register([](uint8_t bufid) -> bool {
        ESP_LOGW(TAG, "device_cb: 0x%02x", bufid);
        return false;
    });
    esp_zb_raw_command_handler_register([](uint8_t bufid) -> bool {
        ESP_LOGW(TAG, "raw_command: 0x%02x", bufid);
        return false;
    });

    esp_zb_set_primary_network_channel_set(ESP_ZB_TRANSCEIVER_ALL_CHANNELS_MASK);
    ESP_ERROR_CHECK(esp_zb_start(false));

    return *this;
}
auto Zigbee::loop() -> void {
    esp_zb_main_loop_iteration();
}
auto Zigbee::reset() -> void {
    esp_zb_factory_reset();
}

#include <string.h>
auto Zigbee::update(uint32_t value) -> void {

    // -> esp_zb_zcl_set_attribute_val(). It will automatically report the change using the configured reporting interval.

    esp_zb_uint48_t current_summation_delivered = {
        .low = value,
        .high = 0,
    };

    auto status = esp_zb_zcl_set_attribute_val(ZB_ENDPOINT,
                                               ESP_ZB_ZCL_CLUSTER_ID_METERING,
                                               ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
                                               ESP_ZB_ZCL_ATTR_METERING_CURRENT_SUMMATION_DELIVERED_ID,
                                               &current_summation_delivered,
                                               false);
    if (status != ESP_ZB_ZCL_STATUS_SUCCESS) {
        ESP_LOGW(TAG, "Set current summation not success (status: 0x%02x)", status);
    }

    esp_zb_zcl_attr_location_info_t attr_info = {
        .endpoint_id = ZB_ENDPOINT,
        .cluster_id = ESP_ZB_ZCL_CLUSTER_ID_METERING,
        .cluster_role = ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
        .manuf_code = 0x00,
        .attr_id = ESP_ZB_ZCL_ATTR_METERING_CURRENT_SUMMATION_DELIVERED_ID,
    };

    auto res = esp_zb_zcl_find_reporting_info(attr_info);
    if (res == NULL) {
        ESP_LOGE(TAG, "null");
    }


    // esp_zb_zcl_reset_all_reporting_info();

    /// --> NOT WORKING
    // auto status = esp_zb_zcl_set_attribute_val(ZB_ENDPOINT,
    //                                            ESP_ZB_ZCL_CLUSTER_ID_METERING,
    //                                            ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
    //                                            ESP_ZB_ZCL_ATTR_METERING_CURRENT_SUMMATION_DELIVERED_ID,
    //                                            &current_summation_delivered,
    //                                            true);
    // if (status != ESP_ZB_ZCL_STATUS_SUCCESS) {
    //     ESP_LOGW(TAG, "Set current summation not success (status: 0x%02x)", status);
    // }

    // /// WORKAROUND
    // auto value_r = esp_zb_zcl_get_attribute(ZB_ENDPOINT,
    //                                         ESP_ZB_ZCL_CLUSTER_ID_METERING,
    //                                         ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
    //                                         ESP_ZB_ZCL_ATTR_METERING_CURRENT_SUMMATION_DELIVERED_ID);
    // memcpy(value_r->data_p, &current_summation_delivered, sizeof(current_summation_delivered));

    // ---> REPORT NOT WORKING
    // esp_zb_zcl_report_attr_cmd_t cmd = {
    //     .zcl_basic_cmd = {
    //         .dst_addr_u = {
    //             .addr_short = 0x0000,
    //         },
    //         .dst_endpoint = ZB_ENDPOINT,
    //         .src_endpoint = ZB_ENDPOINT,
    //     },
    //     .address_mode = ESP_ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT,
    //     .clusterID = ESP_ZB_ZCL_CLUSTER_ID_METERING,
    //     .cluster_role = ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
    //     .attributeID = ESP_ZB_ZCL_ATTR_METERING_CURRENT_SUMMATION_DELIVERED_ID,
    // };

    // esp_zb_zcl_report_attr_cmd_t cmd = {
    //     .zcl_basic_cmd = {
    //         .dst_addr_u = {
    //             .addr_short = 0x0000,
    //         },
    //         .dst_endpoint = ZB_ENDPOINT,
    //         .src_endpoint = ZB_ENDPOINT,
    //     },
    //     .address_mode = ESP_ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT,
    //     .clusterID = ESP_ZB_ZCL_CLUSTER_ID_METERING,
    //     .cluster_role = ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
    //     .attributeID = ESP_ZB_ZCL_ATTR_METERING_INSTANTANEOUS_DEMAND_ID,
    // };
    // esp_zb_zcl_report_attr_cmd_req(&cmd);
}

// Clusters
auto Zigbee::create_basic_cluster() -> esp_zb_attribute_list_t * {
    esp_zb_basic_cluster_cfg_t config = {
        .zcl_version = ESP_ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE,
        .power_source = 0x03, // Battery
    };
    uint32_t app_version = 0x0001;
    uint32_t stack_version = 0x0002;
    uint32_t hw_version = 0x0001;
    uint8_t manufacturer_name[] = {6, 'A', 'q', 'w', 's', 'k', 'i'};
    uint8_t model_identifier[] = {9, 'B', 'a', 'l', 't', 'h', 'a', 'Z', 'a', 'r'};
    uint8_t date_code[] = {8, '2', '0', '2', '3', '1', '0', '0', '6'};
    uint8_t sw_build[] = {5, '0', '.', '0', '.', '1'};
    esp_zb_attribute_list_t *cluster = esp_zb_basic_cluster_create(&config);
    esp_zb_basic_cluster_add_attr(cluster, ESP_ZB_ZCL_ATTR_BASIC_APPLICATION_VERSION_ID, &app_version);
    esp_zb_basic_cluster_add_attr(cluster, ESP_ZB_ZCL_ATTR_BASIC_STACK_VERSION_ID, &stack_version);
    esp_zb_basic_cluster_add_attr(cluster, ESP_ZB_ZCL_ATTR_BASIC_HW_VERSION_ID, &hw_version);
    esp_zb_basic_cluster_add_attr(cluster, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID, manufacturer_name);
    esp_zb_basic_cluster_add_attr(cluster, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID, model_identifier);
    esp_zb_basic_cluster_add_attr(cluster, ESP_ZB_ZCL_ATTR_BASIC_DATE_CODE_ID, date_code);
    esp_zb_basic_cluster_add_attr(cluster, ESP_ZB_ZCL_ATTR_BASIC_SW_BUILD_ID, sw_build);

    return cluster;
}
auto Zigbee::create_identify_cluster() -> esp_zb_attribute_list_t * {
    esp_zb_identify_cluster_cfg_t config = {
        .identify_time = 0,
    };
    esp_zb_attribute_list_t *cluster = esp_zb_identify_cluster_create(&config);

    return cluster;
}
auto Zigbee::create_power_config_cluster() -> esp_zb_attribute_list_t * {
    esp_zb_power_config_cluster_cfg_t config = {};

    esp_zb_attribute_list_t *cluster = esp_zb_power_config_cluster_create(&config);
    uint8_t battery_voltage = 0xFE;           // 0x01 = 100mV, 0x02 = 200mV, 0xFF = invalid or unknown
    uint8_t battery_percent_remaining = 0xC8; // 0x00 = 0%, 0x64=50%, 0xC8=100%
    esp_zb_power_config_cluster_add_attr(cluster, 0x0020, &battery_voltage);
    esp_zb_power_config_cluster_add_attr(cluster, 0x0021, &battery_percent_remaining);

    return cluster;
}
auto Zigbee::create_time_cluster() -> esp_zb_attribute_list_t * {
    return esp_zb_zcl_attr_list_create(ESP_ZB_ZCL_CLUSTER_ID_TIME);
}
auto Zigbee::create_metering_cluster() -> esp_zb_attribute_list_t * {
    esp_zb_metering_cluster_cfg_t config = {
        .current_summation_delivered = {
            .low = 0,
            .high = 0,
        },
        .status = 0,
        .uint_of_measure = ESP_ZB_ZCL_METERING_UNIT_M3_M3H_BINARY,
        .summation_formatting = ESP_ZB_ZCL_METERING_FORMATTING_SET(true, 5, 2),
        .metering_device_type = ESP_ZB_ZCL_METERING_GAS_METERING,
    };
    esp_zb_attribute_list_t *cluster = esp_zb_metering_cluster_create(&config);

    uint8_t update_periode = 10; // 10 secondes
    esp_zb_cluster_add_attr(cluster,
                            ESP_ZB_ZCL_CLUSTER_ID_METERING,
                            ESP_ZB_ZCL_ATTR_METERING_DEFAULT_UPDATE_PERIOD_ID,
                            ESP_ZB_ZCL_ATTR_TYPE_U8,
                            ESP_ZB_ZCL_ATTR_ACCESS_READ_ONLY,
                            &update_periode);

    esp_zb_int24_t instantaneous_demand = {
        .low = 0,
        .high = 0,
    };
    esp_zb_cluster_add_attr(cluster,
                            ESP_ZB_ZCL_CLUSTER_ID_METERING,
                            ESP_ZB_ZCL_ATTR_METERING_INSTANTANEOUS_DEMAND_ID,
                            ESP_ZB_ZCL_ATTR_TYPE_S24,
                            ESP_ZB_ZCL_ATTR_ACCESS_READ_ONLY,
                            &instantaneous_demand);

    return cluster;
}

// Actions handlers
auto Zigbee::action(esp_zb_core_action_callback_id_t callback_id, const void *message) -> esp_err_t {
    esp_err_t result = ESP_OK;

    switch (callback_id) {
        case ESP_ZB_CORE_SET_ATTR_VALUE_CB_ID:
            result = this->action_set_attribut((esp_zb_zcl_set_attr_value_message_t *)message);
            break;

        default:
            ESP_LOGW(TAG, "Receive Zigbee action(0x%x) callback", callback_id);
            break;
    }

    return result;
}
auto Zigbee::action_set_attribut(esp_zb_zcl_set_attr_value_message_t *message) -> esp_err_t {
    ESP_RETURN_ON_FALSE(message, ESP_FAIL, TAG, "Empty message");
    ESP_RETURN_ON_FALSE(message->info.status == ESP_ZB_ZCL_STATUS_SUCCESS, ESP_ERR_INVALID_ARG, TAG,
                        "Received message: error status(%d)", message->info.status);

    ESP_LOGI(TAG, "Received message: endpoint(%d), cluster(0x%x), attribute(0x%x), data size(%d) data type(0x%x)",
             message->info.dst_endpoint, message->info.cluster,
             message->attribute.id, message->attribute.data.size, message->attribute.data.type);

    // switch (data.type) {

    //     case ESP_ZB_ZCL_ATTR_TYPE_BOOL: {
    //         auto v = *(bool *)data.value;
    //         ESP_LOGI(TAG, "bool: {%d}", v);
    //         break;
    //     }

    //     case ESP_ZB_ZCL_ATTR_TYPE_U16: {
    //         auto value = *(uint16_t *)data.value;
    //         ESP_LOGI(TAG, "uint16_t: {%d}", value);
    //         break;
    //     }

    //     default:
    //         ESP_LOGW(TAG, "type: {%d}", data.type);
    //         break;
    // }

    esp_err_t ret = ESP_OK;
    return ret;
}
auto Zigbee::action_identify_notify(uint8_t identify_on) -> void {
    ESP_ERROR_CHECK(esp_event_post_to(event_loop,
                                      APP_EVENTS,
                                      EV_ZB_IDENTIFY,
                                      &identify_on,
                                      1,
                                      portMAX_DELAY));
}

// Signals
auto Zigbee::signal(esp_zb_app_signal_t *signal) -> void {
    esp_err_t err_status = signal->esp_err_status;

    const esp_zb_app_signal_type_t sig_type = (esp_zb_app_signal_type_t)(*(signal->p_app_signal));
    switch (sig_type) {
        case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
            signal_skip_startup();
            break;

        case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
            if (err_status == ESP_OK) {
                ESP_LOGI(TAG, "Start network steering");
                esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
                ESP_ERROR_CHECK(esp_event_post_to(event_loop,
                                                  APP_EVENTS,
                                                  EV_ZB_STEERING_START,
                                                  NULL,
                                                  0,
                                                  portMAX_DELAY));
            } else {
                ESP_LOGW(TAG, "Failed to initialize Zigbee stack (status: %s)", esp_err_to_name(err_status));
            }
            break;

        case ESP_ZB_BDB_SIGNAL_STEERING:
            signal_sterring(err_status);
            break;

        default:
            ESP_LOGI(TAG, "ZDO signal: %s (0x%x), status: %s",
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
auto Zigbee::signal_sterring(esp_err_t err) -> void {
    if (err == ESP_OK) {
        esp_zb_ieee_addr_t extended_pan_id;
        esp_zb_get_extended_pan_id(extended_pan_id);
        ESP_LOGI(TAG, "Joined network successfully (Extended PAN ID: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x, PAN ID: 0x%04hx, Channel:%d)",
                 extended_pan_id[7], extended_pan_id[6], extended_pan_id[5], extended_pan_id[4],
                 extended_pan_id[3], extended_pan_id[2], extended_pan_id[1], extended_pan_id[0],
                 esp_zb_get_pan_id(), esp_zb_get_current_channel());

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

// void reportAttribute(uint8_t endpoint, uint16_t clusterID, uint16_t attributeID, void *value, uint8_t value_length)
// {
//     esp_zb_zcl_report_attr_cmd_t cmd = {
//         .zcl_basic_cmd = {
//             .dst_addr_u.addr_short = 0x0000,
//             .dst_endpoint = endpoint,
//             .src_endpoint = endpoint,
//         },
//         .address_mode = ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
//         .clusterID = clusterID,
//         .attributeID = attributeID,
//         .cluster_role = ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
//     };
//     esp_zb_zcl_attr_t *value_r = esp_zb_zcl_get_attribute(endpoint, clusterID, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE, attributeID);
//     memcpy(value_r->data_p, value, value_length);
//     esp_zb_zcl_report_attr_cmd_req(&cmd);
// }
