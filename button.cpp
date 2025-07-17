#include "button.hpp"
#include <wiringPi.h>

Button::Button(uint8_t gpio_pin, isr_callback_t callback) : m_pin(gpio_pin) {
    pinMode(m_pin, INPUT);
    pullUpDnControl(m_pin, PUD_UP);
    wiringPiISR(m_pin, INT_EDGE_FALLING, callback);
}

Button::~Button() {
    wiringPiISRStop(m_pin);
}
