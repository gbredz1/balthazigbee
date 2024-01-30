#pragma once

#include <led_strip.h>

/**
 * @brief RGB Color
 */
struct Color {
    uint32_t red;
    uint32_t green;
    uint32_t blue;
    Color(uint32_t r, uint32_t g, uint32_t b) : red{r}, green{g}, blue{b} {}
};

/**
 * @brief Led animations
 */
class Led {
  private:
    double m_anim = 0;
    led_strip_handle_t m_handle;
    Color m_color;
    bool m_running = false;

  public:
    Led();
    ~Led();

    auto run() -> void;
    auto stop() -> void;
    auto toggle() -> void;
    auto update() -> void;
};