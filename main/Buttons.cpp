#include "Buttons.h"

#include "AppEvents.h"
#include <esp_log.h>

static const char *TAG = "BALTHAZAR_BUTTONS";

Buttons::Buttons() {
}

Buttons::~Buttons() {
    if (!has_been_setup) {
        return;
    }

    iot_button_delete(bt_0_handle);
    iot_button_delete(bt_1_handle);
}

auto Buttons::setup(esp_event_loop_handle_t loop_handle) -> void {
    if (has_been_setup) {
        ESP_LOGE(TAG, "Already setup!");
        return;
    }

    button_config_t bt0_config = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = BUTTON_LONG_PRESS_TIME_MS,
        .short_press_time = 380,
        .gpio_button_config = {
            .gpio_num = 0,
            .active_level = 0,
        },
    };

    bt_0_handle = iot_button_create(&bt0_config);
    if (NULL == bt_0_handle) {
        ESP_LOGE(TAG, "bt_0_handle create failed");
        return;
    }

    button_config_t bt1_config = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = BUTTON_LONG_PRESS_TIME_MS,
        .short_press_time = BUTTON_SHORT_PRESS_TIME_MS,
        .gpio_button_config = {
            .gpio_num = 1,
            .active_level = 0,
        },
    };

    bt_1_handle = iot_button_create(&bt1_config);
    if (NULL == bt_1_handle) {
        ESP_LOGE(TAG, "bt_1_handle create failed");
        return;
    }

    has_been_setup = true;

    iot_button_register_cb(
        bt_0_handle, BUTTON_SINGLE_CLICK, [](void *arg, void *usr_data) {
            ESP_ERROR_CHECK(esp_event_post_to((esp_event_loop_handle_t)usr_data,
                                              APP_EVENTS,
                                              EV_BT_0,
                                              NULL,
                                              0,
                                              portMAX_DELAY));
        },
        loop_handle);

    button_event_config_t btn_cfg;
    btn_cfg.event = BUTTON_MULTIPLE_CLICK;
    btn_cfg.event_data.multiple_clicks.clicks = 4;
    iot_button_register_event_cb(
        bt_0_handle, btn_cfg, [](void *arg, void *usr_data) {
            ESP_ERROR_CHECK(esp_event_post_to((esp_event_loop_handle_t)usr_data,
                                              APP_EVENTS,
                                              EV_RESET,
                                              NULL,
                                              0,
                                              portMAX_DELAY));
        },
        loop_handle);

    iot_button_register_cb(
        bt_1_handle, BUTTON_SINGLE_CLICK, [](void *arg, void *usr_data) {
            ESP_ERROR_CHECK(esp_event_post_to((esp_event_loop_handle_t)usr_data,
                                              APP_EVENTS,
                                              EV_BT_1,
                                              NULL,
                                              0,
                                              portMAX_DELAY));
        },
        loop_handle);
}