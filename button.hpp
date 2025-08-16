#pragma once
#include <cstdint>
#include <bits/stdc++.h>
#include <wiringPi.h>

typedef void(*isr_callback_t)(int pin);

class Button {
    uint8_t m_pin;
    public:
    Button(uint8_t gpio_pin, isr_callback_t);
    ~Button();
};
