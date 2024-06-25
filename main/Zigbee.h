#pragma once

#include "StateStore.h"
#include "ZigbeeAttribute.h"
#include <esp_event.h>
#include <esp_zigbee_core.h>

namespace Zigbee {

/**
 * @brief Zigbee stuffs
 */
class Zigbee {
  private:
    esp_event_loop_handle_t m_event_loop;

  public:
    Zigbee();
    ~Zigbee();

    auto setup(esp_event_loop_handle_t) -> Zigbee &;
    auto start(const State) -> Zigbee &;
    auto loop() const -> void;
    auto reset() const -> void;
    auto update_then_report(const Attribute) const -> void;
    auto update(const Attribute) const -> void;
    auto report(const Attribute) const -> void;

    auto signal(const esp_zb_app_signal_t *) -> void;
    auto action(esp_zb_core_action_callback_id_t, const void *) -> esp_err_t;

  private:
    // Clusters
    static auto add_basic_cluster(esp_zb_cluster_list_t *) -> esp_err_t;
    static auto add_identify_cluster(esp_zb_cluster_list_t *) -> esp_err_t;
    static auto add_groups_cluster(esp_zb_cluster_list_t *) -> esp_err_t;
    static auto add_power_config_cluster(esp_zb_cluster_list_t *) -> esp_err_t;
    static auto add_metering_cluster(esp_zb_cluster_list_t *) -> esp_err_t;

    // Actions handlers
    auto action_set_attribut(const esp_zb_zcl_set_attr_value_message_t *) -> esp_err_t;
    auto action_identify_notify(uint8_t) -> void;

    // Signals
    auto signal_skip_startup() const -> void;
    auto signal_boot(esp_err_t) -> void;
    auto signal_sterring(esp_err_t) -> void;
};
}