#pragma once

#include <esp_err.h>
#include <nvs_flash.h>

struct State {
    uint64_t summation_delivered;
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