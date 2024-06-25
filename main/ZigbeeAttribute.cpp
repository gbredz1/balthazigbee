#include "ZigbeeAttribute.h"

#include <esp_zigbee_core.h>
#include <zcl/esp_zigbee_zcl_power_config.h>

namespace Zigbee {

Attribute::Attribute(uint16_t cluster_id,
                       uint16_t attribute_id,
                       attr_var_t value)
    : cluster_id(cluster_id),
      attribute_id(attribute_id),
      value(value) {}

AttributeSummationDelivered::AttributeSummationDelivered(uint64_t value)
    : Attribute(
          ESP_ZB_ZCL_CLUSTER_ID_METERING,
          ESP_ZB_ZCL_ATTR_METERING_CURRENT_SUMMATION_DELIVERED_ID,
          esp_zb_uint48_t{.low = (uint32_t)(value),
                          .high = (uint16_t)(value >> 32)}) {}

AttributeBatteryPercentage::AttributeBatteryPercentage(uint8_t value)
    : Attribute(
          ESP_ZB_ZCL_CLUSTER_ID_POWER_CONFIG,
          ESP_ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_PERCENTAGE_REMAINING_ID,
          uint8_t{value}) {}

} // namespace Zigbee