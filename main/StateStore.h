#pragma once

#include <esp_zigbee_type.h>
#include <nvs_flash.h>

struct State {
    esp_zb_int48_t summation_delivered;
    uint8_t battery_percent_remaining;

    auto set_summation_delivered(const uint64_t value) -> void {
        summation_delivered.low = static_cast<uint32_t>(value);
        summation_delivered.high = static_cast<int16_t>(value >> 32);
    }

    auto get_summation_delivered() const -> uint64_t {
        return static_cast<uint64_t>(summation_delivered.high) << 32 | summation_delivered.low;
    }

    auto inc_summation_delivered() -> void {
        summation_delivered.low += 1;

        if (summation_delivered.low == 0) {
            summation_delivered.high += 1;
        }
    }
};

/**
 * @brief StateStore
 * @details Manage state saving and loading in NVS
 */
class StateStore {
  private:
    bool m_initialized;
    nvs_handle_t m_store;

  public:
    StateStore();
    ~StateStore();

    auto init() -> void;
    auto load(State &) const -> esp_err_t;
    auto save(const State *) const -> esp_err_t;
};