#pragma once
#include <nvs_flash.h>

/**
 * @brief Counter
 * @details Manage a simple counter with saving and loading in NVS
 */
class Counter {
  private:
    uint32_t value = 0;
    nvs_handle_t store;
    bool initialized;

  public:
    Counter();
    ~Counter();

    auto init() -> void;
    auto read() -> uint32_t;
    auto increment() -> uint32_t;
    auto set(uint32_t) -> void;

  private:
    auto nvs_store() -> esp_err_t;
    auto nvs_read() -> esp_err_t;
};