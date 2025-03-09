#include "Buttons.h"

#include <button_gpio.h>
#include <esp_log.h>
#include <hal/gpio_types.h> // for gpio_num_t

#include "AppEvents.h"

constexpr auto TAG = "BALTHAZAR_BUTTONS";
constexpr auto BT0_GPIO = GPIO_NUM_4;
constexpr auto BT1_GPIO = GPIO_NUM_5;

Buttons::Buttons() = default;
Buttons::~Buttons() {
    iot_button_delete(bt_0_handle);
    iot_button_delete(bt_1_handle);
}

auto Buttons::setup(const esp_event_loop_handle_t loop_handle) -> void {
    constexpr button_gpio_config_t bt0_gpio_config = {
        .gpio_num = BT0_GPIO,
        .active_level = 0,
        .enable_power_save = true,
        .disable_pull = false};

    if (constexpr button_config_t config = {
            .long_press_time = BUTTON_LONG_PRESS_TIME_MS,
            .short_press_time = 380};
        ESP_OK != iot_button_new_gpio_device(&config, &bt0_gpio_config, &bt_0_handle)) {
        ESP_LOGE(TAG, "handle bt0 creation failed");
        return;
    }

    constexpr button_gpio_config_t bt1_gpio_config = {
        .gpio_num = BT1_GPIO,
        .active_level = 0,
        .enable_power_save = true,
        .disable_pull = false};

    if (constexpr button_config_t config = {
            .long_press_time = BUTTON_LONG_PRESS_TIME_MS,
            .short_press_time = BUTTON_SHORT_PRESS_TIME_MS};
        ESP_OK != iot_button_new_gpio_device(&config, &bt1_gpio_config, &bt_1_handle)) {
        ESP_LOGE(TAG, "handle bt1 creation failed");
        return;
    }

    // One click on button 0
    iot_button_register_cb(
        bt_0_handle,
        BUTTON_SINGLE_CLICK,
        nullptr,
        [](void *, void *usr_data) {
            ESP_ERROR_CHECK(esp_event_post_to(usr_data,
                                              APP_EVENTS,
                                              EV_BT_0,
                                              nullptr,
                                              0,
                                              portMAX_DELAY));
        },
        loop_handle);

    // Four click on button 1
    button_event_args_t button_event;
    button_event.multiple_clicks.clicks = 4;
    iot_button_register_cb(
        bt_0_handle,
        BUTTON_MULTIPLE_CLICK,
        &button_event,
        [](void *, void *usr_data) {
            ESP_ERROR_CHECK(esp_event_post_to(usr_data,
                                              APP_EVENTS,
                                              EV_RESET,
                                              nullptr,
                                              0,
                                              portMAX_DELAY));
        },
        loop_handle);

    // One click on button 1
    iot_button_register_cb(
        bt_1_handle,
        BUTTON_SINGLE_CLICK,
        nullptr,
        [](void *, void *usr_data) {
            ESP_ERROR_CHECK(esp_event_post_to(usr_data,
                                              APP_EVENTS,
                                              EV_BT_1,
                                              nullptr,
                                              0,
                                              portMAX_DELAY));
        },
        loop_handle);
}