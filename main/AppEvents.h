#pragma once
#include "esp_event.h"

/**
 * @enum APP_EVENTS
 * @brief Application events
 */
ESP_EVENT_DECLARE_BASE(APP_EVENTS);
enum APP_EVENTS {
    EV_RESET = 0,
    EV_BT_0,
    EV_BT_1,
    EV_ZB_STEERING_START,
    EV_ZB_STEERING_DONE,
    EV_ZB_IDENTIFY,
    EV_ZB_SET_SUMMATION_DELIVERED,
    EV_ZB_READY,
};
