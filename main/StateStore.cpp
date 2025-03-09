#include "StateStore.h"

#include "../../../../esp/esp-idf-v5.4.1/components/esp_hw_support/port/include/esp_hw_log.h"

#include <esp_err.h>
#include <nvs.h>

constexpr auto TAG = "BALTHAZAR_NVS";
constexpr auto NAMESPACE = "storage";
constexpr auto KEY = "state";

StateStore::StateStore() = default;

StateStore::~StateStore() {
    if (m_initialized) {
        nvs_close(m_store);
    }
}

auto StateStore::init() -> void {
    ESP_LOGW(TAG, "Initializing NVS");

    const auto error = nvs_open(NAMESPACE, NVS_READWRITE, &m_store);
    if (error != ESP_OK) {
        ESP_LOGW(TAG, "Failed to open NVS namespace %s", NAMESPACE);
    }

    m_initialized = true;
}

auto StateStore::load(State &state) const -> esp_err_t {
    size_t size = sizeof(State);

    return nvs_get_blob(m_store, KEY, &state, &size);
}

auto StateStore::save(const State *state) const -> esp_err_t {
    return nvs_set_blob(m_store, KEY, state, sizeof(State));
}