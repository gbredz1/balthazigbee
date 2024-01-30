#pragma once

#include <esp_event.h>
#include <esp_zigbee_core.h>

/**
 * @brief Zigbee stuffs
 */
class Zigbee {
  private:
    esp_event_loop_handle_t event_loop;

  public:
    Zigbee();
    ~Zigbee();

    auto setup(esp_event_loop_handle_t) -> Zigbee &;
    auto start() -> Zigbee &;
    auto loop() -> void;
    auto reset() -> void;
    auto signal(esp_zb_app_signal_t *) -> void;
    auto action(esp_zb_core_action_callback_id_t, const void *) -> esp_err_t;
    auto update(uint32_t) -> void;

  private:
    // Clusters
    static auto create_basic_cluster() -> esp_zb_attribute_list_t *;
    static auto create_identify_cluster() -> esp_zb_attribute_list_t *;
    static auto create_power_config_cluster() -> esp_zb_attribute_list_t *;
    static auto create_time_cluster() -> esp_zb_attribute_list_t *;
    static auto create_metering_cluster() -> esp_zb_attribute_list_t *;

    // Actions handlers
    auto action_set_attribut(esp_zb_zcl_set_attr_value_message_t *) -> esp_err_t;
    auto action_identify_notify(uint8_t) -> void;

    // Signals
    auto signal_skip_startup() -> void;
    auto signal_sterring(esp_err_t) -> void;
};
