#pragma once

#include "StateStore.h"
#include <esp_event.h>
#include <esp_zigbee_core.h>

namespace Zigbee {

enum ClusterAttributeTypes {
    SUMMATION_DELIVERED,
    BATTERY_PERCENT,
};

/**
 * @brief Zigbee stuffs
 */
class Zigbee {
  private:
    esp_event_loop_handle_t event_loop;
    State *state;

  public:
    Zigbee();
    ~Zigbee();

    auto setup(esp_event_loop_handle_t, State *) -> Zigbee &;
    auto start() -> Zigbee &;
    static auto loop() -> void;
    static auto reset() -> void;
    auto update_then_report(const ClusterAttributeTypes &) const -> void;
    auto update(const ClusterAttributeTypes &) const -> void;
    static auto report(const ClusterAttributeTypes &) -> void;

    auto signal(const esp_zb_app_signal_t *) -> void;
    auto action(esp_zb_core_action_callback_id_t, const void *) -> esp_err_t;

  private:
    // Clusters
    static auto add_basic_cluster(esp_zb_cluster_list_t *) -> esp_err_t;
    static auto add_identify_cluster(esp_zb_cluster_list_t *) -> esp_err_t;
    static auto add_groups_cluster(esp_zb_cluster_list_t *) -> esp_err_t;
    auto add_power_config_cluster(esp_zb_cluster_list_t *) const -> esp_err_t;
    auto add_metering_cluster(esp_zb_cluster_list_t *) const -> esp_err_t;

    // Actions handlers
    auto action_set_attribute(const esp_zb_zcl_set_attr_value_message_t *) -> esp_err_t;
    auto action_identify_notify(uint8_t) const -> void;

    // Signals
    static auto signal_skip_startup() -> void;
    auto signal_boot(esp_err_t) -> void;
    auto signal_steering(esp_err_t) -> void;
};
} // namespace Zigbee