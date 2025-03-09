#pragma once

#include <cstdint>
#include <led_strip_types.h>

/**
 * @brief RGB Color
 */
struct Color {
    uint32_t red;
    uint32_t green;
    uint32_t blue;
    Color(const uint32_t red, const uint32_t green, const uint32_t blue)
        : red{red}, green{green}, blue{blue} {}
};

/**
 * @brief Led animations
 */
class Led {
  private:
    double m_anim = 0;
    led_strip_handle_t m_handle;
    Color m_color = {0, 0, 0};
    bool m_running = false;

  public:
    Led();
    ~Led();

    auto run() -> void;
    auto stop() -> void;
    auto toggle() -> void;
    auto update() -> void;
};