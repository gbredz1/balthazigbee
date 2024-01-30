#include "Counter.h"
#include <esp_err.h>
#include <esp_log.h>
#include <nvs_flash.h>

static const char *TAG = "BALTHAZAR_COUNTER";
const char *NAMESPACE = "storage";
const char *KEY = "counter";

Counter::Counter() {}
Counter::~Counter() {
    if (initialized) {
        nvs_close(store);
    }
}

auto Counter::init() -> void {
    ESP_ERROR_CHECK(nvs_open(NAMESPACE, NVS_READWRITE, &store));
    initialized = true;

    esp_err_t err = nvs_read();
    if (ESP_OK != err && ESP_ERR_NVS_NOT_FOUND != err) {
        ESP_LOGE(TAG, "Error %s reading!\n", esp_err_to_name(err));
    }
}

auto Counter::nvs_read() -> esp_err_t {
    return nvs_get_u32(store, KEY, &value);
}
auto Counter::nvs_store() -> esp_err_t {
    esp_err_t err = nvs_set_u32(store, KEY, value);
    if (ESP_OK != err) {
        return err;
    }

    return nvs_commit(store);
}

auto Counter::read() -> uint32_t {
    return value;
}

auto Counter::increment() -> uint32_t {
    value += 1;

    ESP_ERROR_CHECK(nvs_store());

    return value;
}

auto Counter::set(uint32_t value) -> void {
    this->value = value;
    ESP_ERROR_CHECK(nvs_store());
}
