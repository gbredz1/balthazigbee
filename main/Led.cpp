#include "Led.h"

#include <cmath>
#include <esp_log.h>
#include <led_strip.h>
#include <led_strip_rmt.h>

constexpr uint8_t BLINK_GPIO = 8;

constexpr uint32_t HALF_LED_MAX = 16;
constexpr double LOOP_STEP = 0.02;

constexpr auto TAG = "BALTHAZAR_LED";

Led::Led() {
    constexpr led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO,                                // The GPIO that connected to the LED strip's data line
        .max_leds = 1,                                               // The number of LEDs in the strip,
        .led_model = LED_MODEL_WS2812,                               // LED strip model
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB, // Pixel format of your LED strip
        .flags{.invert_out = 0U},                                    // whether to invert the output signal (useful when your hardware has a level inverter)
    };

    constexpr led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,    // different clock source can lead to different power consumption
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .mem_block_symbols = 0,
        .flags{.with_dma = 0U}, // whether to enable the DMA feature
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &m_handle));
}

Led::~Led() {
    led_strip_del(m_handle);
}

auto Led::update() -> void {
    if (!m_running) {
        led_strip_clear(m_handle);
        return;
    }

    m_anim = fmod(m_anim + LOOP_STEP, M_PI * 2.0);

    m_color.red = static_cast<uint32_t>(HALF_LED_MAX + round(sin(m_anim) * HALF_LED_MAX));
    m_color.green = static_cast<uint32_t>(HALF_LED_MAX + round(sin(m_anim + M_PI * 0.67) * HALF_LED_MAX));
    m_color.blue = static_cast<uint32_t>(HALF_LED_MAX + round(sin(m_anim + M_PI * 1.34) * HALF_LED_MAX));

    led_strip_set_pixel(m_handle, 0, m_color.red, m_color.green, m_color.blue);
    led_strip_refresh(m_handle);

    ESP_LOGD(TAG, "%3lu %3lu %3lu | %f", m_color.red, m_color.green, m_color.blue, m_anim);
}

auto Led::run() -> void {
    ESP_LOGI(TAG, "run");
    m_running = true;
}

auto Led::stop() -> void {
    ESP_LOGI(TAG, "stop");
    m_running = false;
}

auto Led::toggle() -> void {
    if (m_running) {
        stop();
    } else {
        run();
    }
}