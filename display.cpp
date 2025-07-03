#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wiringPi.h>
#include <iostream>
#include <thread>
#include <chrono>
#include "display.hpp"

Display::Display(const char *spi_dev, int spi_speed, int cs, int dc, int rst)
    : m_cs(cs), m_dc(dc), m_rst(rst)
{
    this->m_spi = std::make_unique<spidevpp::Spi>(spi_dev);
    this->m_spi->setBitsPerWord(8);
    this->m_spi->setSpeed(spi_speed);
    wiringPiSetup();
    wiringPiSetupGpio();

    pinMode(this->m_cs, OUTPUT);
    pinMode(this->m_dc, OUTPUT);
}

void Display::sendCommand(uint8_t byte, uint8_t *argBuffer, int bufferLen)
{
    digitalWrite(this->m_dc, LOW);
    this->setChipSelect(true);
    this->m_spi->write(&byte, 1);
    this->setChipSelect(false);
    if (bufferLen > 0)
    {
        this->sendData(argBuffer, bufferLen);
    }
}

void Display::sendCommand(uint8_t byte)
{
    this->sendCommand(byte, nullptr, 0);
}

void Display::sendCommand(uint8_t byte, uint8_t arg1)
{
    this->sendCommand(byte, &arg1, 1);
}

void Display::sendCommand(uint8_t byte, uint8_t arg1, uint8_t arg2)
{
    uint8_t buffer[2] = {arg1, arg2};
    this->sendCommand(byte, buffer, 2);
}

void Display::sendCommand(uint8_t byte, uint8_t arg1, uint8_t arg2, uint8_t arg3)
{
    uint8_t buffer[3] = {arg1, arg2, arg3};
    this->sendCommand(byte, buffer, 3);
}

void Display::sendCommand(uint8_t byte, uint8_t arg1, uint8_t arg2, uint8_t arg3, uint8_t arg4)
{
    uint8_t buffer[4] = {arg1, arg2, arg3, arg4};
    this->sendCommand(byte, buffer, 4);
}

void Display::sendCommand(uint8_t byte, uint8_t arg1, uint8_t arg2, uint8_t arg3, uint8_t arg4, uint8_t arg5)
{
    uint8_t buffer[5] = {arg1, arg2, arg3, arg4, arg5};
    this->sendCommand(byte, buffer, 5);
}

void Display::setChipSelect(bool asserted)
{
    if (asserted)
        digitalWrite(this->m_cs, LOW);
    else
        digitalWrite(this->m_cs, HIGH);
}

void Display::sendData(uint8_t *buffer, int bufferLen)
{
    digitalWrite(this->m_dc, HIGH);
    this->setChipSelect(true);
    this->m_spi->write(buffer, bufferLen);
    this->setChipSelect(false);
}

void Display::reset()
{
    if (this->m_rst != -1)
    {
        digitalWrite(this->m_rst, LOW);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        digitalWrite(this->m_rst, HIGH);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

Display::~Display()
{
    this->m_spi.reset();
}

