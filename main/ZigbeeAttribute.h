#pragma once

#include <variant>
#include <esp_zigbee_type.h>

namespace Zigbee {
using attr_var_t = std::variant<uint8_t, esp_zb_uint48_t>;

struct Attribute {
    const uint16_t cluster_id;
    const uint16_t attribute_id;
    const attr_var_t value;

    Attribute(uint16_t, uint16_t, attr_var_t);
};

struct AttributeSummationDelivered : public Attribute {
    explicit AttributeSummationDelivered(uint64_t value);
};

struct AttributeBatteryPercentage : public Attribute {
    explicit AttributeBatteryPercentage(uint8_t value);
};
}