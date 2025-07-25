#include "button.hpp"
#include <wiringPi.h>

static void buttonISR(struct WPIWfiStatus status, void *data)
{
    ((isr_callback_t)data)();
}

Button::Button(uint8_t gpio_pin, isr_callback_t callback) : m_pin(gpio_pin) {
    pinMode(m_pin, INPUT);
    pullUpDnControl(m_pin, PUD_UP);
    wiringPiISR2(m_pin, INT_EDGE_FALLING, buttonISR, 150000, callback);
}

Button::~Button() {
    wiringPiISRStop(m_pin);
}
