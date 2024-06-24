#include "StateStore.h"

#include <esp_err.h>
#include <esp_log.h>
#include <nvs_flash.h>

constexpr const char *NAMESPACE = "storage";
constexpr const char *KEY = "state";

StateStore::StateStore() = default;

StateStore::~StateStore() {
    if (m_initialized) {
        nvs_close(m_store);
    }
}

auto StateStore::init() -> void {
    ESP_ERROR_CHECK(nvs_open(NAMESPACE, NVS_READWRITE, &m_store));
    m_initialized = true;
}

auto StateStore::load(State &state) const -> esp_err_t {
    size_t size = sizeof(State);

    return nvs_get_blob(m_store, KEY, &state, &size);
}

auto StateStore::save(const State *state) const -> esp_err_t {
    return nvs_set_blob(m_store, KEY, state, sizeof(State));
}