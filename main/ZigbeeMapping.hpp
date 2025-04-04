#pragma once
#include "Zigbee.h"

namespace ZigbeeMapping {

struct AttributeMapping {
    Zigbee::ClusterAttributeTypes type;
    uint16_t cluster_id;
    uint16_t attr_id;
    void *(*value_p)(const State &);
} __attribute__((aligned(16)));

} // namespace ZigbeeMapping

static constexpr auto p_attr(const State &state, const void *attribute) -> void * {
    return const_cast<void *>(attribute);
}

inline static constexpr ZigbeeMapping::AttributeMapping k_attribute_mappings[] = {
    {Zigbee::CURRENT_SUMMATION_DELIVERED,
     ESP_ZB_ZCL_CLUSTER_ID_METERING,
     ESP_ZB_ZCL_ATTR_METERING_CURRENT_SUMMATION_DELIVERED_ID,
     [](const State &state) { return p_attr(state, &state.summation_delivered); }},

    {Zigbee::BATTERY_PERCENTAGE_REMAINING,
     ESP_ZB_ZCL_CLUSTER_ID_POWER_CONFIG,
     ESP_ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_PERCENTAGE_REMAINING_ID,
     [](const State &state) { return p_attr(state, &state.battery_percent_remaining); }}};