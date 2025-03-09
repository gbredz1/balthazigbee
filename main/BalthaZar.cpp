#include "BalthaZar.h"

#include "AppEvents.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

ESP_EVENT_DEFINE_BASE(APP_EVENTS);

constexpr const char *TAG = "BALTHAZAR";

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Zigbee Gazpar");

    BalthaZar::instance()->init().load().start();
}

auto BalthaZar::init() -> BalthaZar & {
    ESP_LOGI(TAG, "init");
    ESP_ERROR_CHECK(nvs_flash_init());
    event_init();

    stateStore.init();
    zigbee.setup(event_handle);
    buttons.setup(event_handle);

    led.stop();

    return *this;
}

auto BalthaZar::start() -> void {
    ESP_LOGI(TAG, "start");
    running = true;

    xTaskCreate([](void *m_this) { ((BalthaZar *)m_this)->led_task(); },
                "Led",
                configMINIMAL_STACK_SIZE + 256,
                this,
                tskIDLE_PRIORITY + 5,
                nullptr);

    xTaskCreate([](void *m_this) { ((BalthaZar *)m_this)->zigbee_task(); },
                "Zigbee",
                configMINIMAL_STACK_SIZE + 4096,
                this,
                tskIDLE_PRIORITY + 10,
                nullptr);
}

auto BalthaZar::save() const -> void {
    if (stateStore.save(&state) == ESP_OK) {
        ESP_LOGI(TAG, "Save state OK");
    } else {
        ESP_LOGE(TAG, "Save state FAILED");
    }
}

auto BalthaZar::load() -> BalthaZar & {
    if (stateStore.load(state) == ESP_OK) {
        ESP_LOGI(TAG, "Load state OK");
    } else {
        ESP_LOGE(TAG, "Load state FAILED");
    }

    return *this;
}

auto BalthaZar::event_init() -> void {
    esp_event_loop_args_t loop_args = {
        .queue_size = 10,
        .task_name = "app_event_task",
        .task_priority = uxTaskPriorityGet(nullptr),
        .task_stack_size = 32768,
        .task_core_id = tskNO_AFFINITY,
    };
    ESP_ERROR_CHECK(esp_event_loop_create(&loop_args, &event_handle));
    ESP_ERROR_CHECK(esp_event_handler_register_with(
        event_handle,
        APP_EVENTS,
        ESP_EVENT_ANY_ID,
        [](void *m_this,
           esp_event_base_t event_base,
           int32_t event_id,
           void *event_data) {
            ((BalthaZar *)m_this)->event_handler(event_base, event_id, event_data);
        },
        this));
}

auto BalthaZar::event_handler(esp_event_base_t,
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
            ESP_LOGI(TAG, "counter: %llu", state.summation_delivered);
            debug_info();

            state.battery_percent = state.summation_delivered * 5 % 200;
            zigbee.update_then_report(Zigbee::AttributeBatteryPercentage(state.battery_percent));

            break;
        case EV_BT_1:
            ESP_LOGI(TAG, "EV_BT_1");
            state.summation_delivered += 1;
            save();
            zigbee.update_then_report(Zigbee::AttributeSummationDelivered(state.summation_delivered));

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

    ESP_LOGI(TAG, "Led task finished");
    vTaskDelete(nullptr);
}

auto BalthaZar::zigbee_task() -> void {
    ESP_LOGI(TAG, "Zigbee task started");

    zigbee.start(state);
    zigbee.loop();

    ESP_LOGI(TAG, "Zigbee task finished");
    vTaskDelete(nullptr);
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
             uxTaskGetStackHighWaterMark(nullptr),
             heap_caps_get_free_size(MALLOC_CAP_8BIT),
             heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
}