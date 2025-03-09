#pragma once

#include "Buttons.h"
#include "Led.h"
#include "StateStore.h"
#include "Zigbee.h"

/**
 * @brief BalthaZar
 * @details Main class, initializes components and manage events.
 */
class BalthaZar {
  private:
    Led led;
    Zigbee::Zigbee zigbee;
    Buttons buttons;
    StateStore stateStore;
    bool running = false;
    esp_event_loop_handle_t event_handle = nullptr;
    State state = {};

  public:
    auto init() -> BalthaZar &;
    auto start() -> void;
    auto load() -> BalthaZar &;
    auto save() const -> void;

  private:
    auto event_init() -> void;
    auto event_handler(int32_t, void *) -> void;
    auto led_task() -> void;
    auto zigbee_task() -> void;

    // events
    auto identity_mode(bool) -> void;
    static auto debug_info() -> void;
};