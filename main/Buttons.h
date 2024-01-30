#pragma once
#include <esp_event.h>
#include <iot_button.h>

/**
 * @brief Buttons
 * @details Create events with buttons.
 */
class Buttons {
  private:
    bool has_been_setup = false;
    button_handle_t bt_0_handle;
    button_handle_t bt_1_handle;

  public:
    Buttons();
    ~Buttons();

    auto setup(esp_event_loop_handle_t) -> void;
};