#include "BalthaZar.h"

#include "AppEvents.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs_flash.h>

ESP_EVENT_DEFINE_BASE(APP_EVENTS);

static const char *TAG = "BALTHAZAR";
static BalthaZar app;

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Zigbee Gazpar");

    app.init()
        .start();
}

auto BalthaZar::init() -> BalthaZar & {
    ESP_LOGI(TAG, "init");
    ESP_ERROR_CHECK(nvs_flash_init());
    event_init();

    counter.init();
    zigbee.setup(event_handle, counter.read());
    buttons.setup(event_handle);

    led.stop();

    return *this;
}

auto BalthaZar::start() -> void {
    ESP_LOGI(TAG, "start");
    running = true;

    xTaskCreate([](void *) { app.led_task(); },
                "Led",
                configMINIMAL_STACK_SIZE + 256,
                NULL,
                tskIDLE_PRIORITY + 5,
                NULL);

    xTaskCreate([](void *) { app.zigbee_task(); },
                "Zigbee",
                configMINIMAL_STACK_SIZE + 2048,
                NULL,
                tskIDLE_PRIORITY + 10,
                NULL);

    // // DEBUG
    //
    // for (;;) {
    //     vTaskGetRunTimeStats(buffer);
    //     ESP_LOGI(TAG, "Runtime Stats:\nTask\t\tAbs Time\t%% Time\n%s", buffer);
    //     vTaskList(buffer);
    //     ESP_LOGI(TAG, "TaskList:\nTask\t\tState Priority Stack\tNum\n%s", buffer);

    //     ESP_LOGI(TAG, "stack %d, free dram heaps %d, free internal memory %d",
    //              uxTaskGetStackHighWaterMark(NULL),
    //              heap_caps_get_free_size(MALLOC_CAP_8BIT),
    //              heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

    //     vTaskDelay(pdMS_TO_TICKS(10000));
    // }
}

auto BalthaZar::event_init() -> void {
    esp_event_loop_args_t loop_args = {
        .queue_size = 10,
        .task_name = "app_event_task",
        .task_priority = uxTaskPriorityGet(NULL),
        .task_stack_size = 32768,
        .task_core_id = tskNO_AFFINITY,
    };
    ESP_ERROR_CHECK(esp_event_loop_create(&loop_args, &event_handle));
    ESP_ERROR_CHECK(esp_event_handler_register_with(
        event_handle,
        APP_EVENTS,
        ESP_EVENT_ANY_ID,
        [](void *arg,
           esp_event_base_t event_base,
           int32_t event_id,
           void *event_data) {
            app.event_handler(arg, event_base, event_id, event_data);
        },
        NULL));
}

auto BalthaZar::event_handler(void *arg,
                              esp_event_base_t event_base,
                              int32_t event_id,
                              void *event_data) -> void {

    switch (event_id) {
        case EV_RESET:
            ESP_LOGI(TAG, "APP_RESET!!!!");
            zigbee.reset();
            break;
        case EV_ZB_STEERING_START:
            ESP_LOGI(TAG, "ZB_STEERING");
            led.run();
            break;
        case EV_ZB_STEERING_DONE:
            ESP_LOGI(TAG, "ZB_STEERING_DONE");
            led.stop();
            break;
        case EV_ZB_IDENTIFY:
            identity_mode(0 < *(uint8_t *)event_data);
            break;
        case EV_BT_0:
            ESP_LOGI(TAG, "EV_BT_0");
            ESP_LOGI(TAG, "counter: %lu", counter.read());
            debug_info();
            break;
        case EV_BT_1:
            ESP_LOGI(TAG, "EV_BT_1");
            counter.increment();
            zigbee.update(counter.read());
            break;
        default:
            ESP_LOGI(TAG, "Event %lu", event_id);
            break;
    }
}

auto BalthaZar::led_task() -> void {
    ESP_LOGI(TAG, "Led task started");

    led.run();

    while (running) {
        vTaskDelay(pdMS_TO_TICKS(10));
        led.update();
    }

    vTaskDelete(NULL);
    ESP_LOGI(TAG, "Led task finished");
}

auto BalthaZar::zigbee_task() -> void {
    ESP_LOGI(TAG, "Zigbee task started");

    zigbee.start();
    zigbee.loop();

    vTaskDelete(NULL);
    ESP_LOGI(TAG, "Zigbee task finished");
}

auto BalthaZar::identity_mode(bool enabled) -> void {
    ESP_LOGI(TAG, "Identify Mode: %s", enabled ? "On" : "Off");
    if (enabled) {
        led.run();
    } else {
        led.stop();
    }
}

auto BalthaZar::debug_info() -> void {
    char buffer[512] = {0};

    vTaskGetRunTimeStats(buffer);
    ESP_LOGI(TAG, "Runtime Stats:\nTask\t\tAbs Time\t%% Time\n%s", buffer);
    vTaskList(buffer);
    ESP_LOGI(TAG, "TaskList:\nTask\t\tState Priority Stack\tNum\n%s", buffer);

    ESP_LOGI(TAG, "stack %d, free dram heaps %d, free internal memory %d",
             uxTaskGetStackHighWaterMark(NULL),
             heap_caps_get_free_size(MALLOC_CAP_8BIT),
             heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
}