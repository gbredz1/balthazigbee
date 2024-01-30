#pragma once

#include "Buttons.h"
#include "Counter.h"
#include "Led.h"
#include "Zigbee.h"

/**
 * @brief BalthaZar
 * @details Main class, initializes components and manage events.
 */
class BalthaZar {
  private:
    Led led;
    Zigbee zigbee;
    Buttons buttons;
    Counter counter;
    bool running;
    esp_event_loop_handle_t event_handle;

  public:
    auto init() -> BalthaZar &;
    auto start() -> void;

  private:
    auto event_init() -> void;
    auto event_handler(void *, esp_event_base_t, int32_t, void *) -> void;
    auto led_task() -> void;
    auto zigbee_task() -> void;

    // events
    auto identity_mode(bool) -> void;
    static auto debug_info() -> void;
};