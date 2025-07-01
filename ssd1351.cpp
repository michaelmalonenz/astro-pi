#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wiringPi.h>
#include <iostream>
#include <thread>
#include <chrono>
#include "ssd1351.hpp"

static spi_config_t spi_config;

#define SPI_SPEED 8000000

Ssd1351::Ssd1351(const char *spi_dev, int cs, int dc, int rst)
    : m_cs(cs), m_dc(dc), m_rst(rst)
{
    spi_config.mode = 0;
    spi_config.speed = SPI_SPEED;
    spi_config.delay = 0;
    spi_config.bits_per_word = 8;
    this->m_spi = std::make_unique<SPI>(spi_dev, &spi_config);
    if (!this->m_spi->begin())
    {
        std::cerr << "Unable to open SPI, display won't work" << std::endl;
    }
    wiringPiSetup();
    wiringPiSetupGpio();

    pinMode(this->m_cs, OUTPUT);
    pinMode(this->m_dc, OUTPUT);

    // INIT DISPLAY ------------------------------------------------------------
    this->reset();
    this->sendCommand(SSD1351_CMD_COMMANDLOCK, 1, 0x12); // Set command lock, 1 arg
    this->sendCommand(SSD1351_CMD_COMMANDLOCK, 1, 0xB1); // Set command lock, 1 arg
    this->sendCommand(SSD1351_CMD_DISPLAYOFF, 0); // Display off, no args
    this->sendCommand(SSD1351_CMD_DISPLAYENHANCE, 3, 0xA4, 0x00, 0x00);
    this->sendCommand(SSD1351_CMD_CLOCKDIV, 1, 0xF0); // 7:4 = Oscillator Freq, 3:0 = CLK Div Ratio (A[3:0]+1 = 1..16)
    this->sendCommand(SSD1351_CMD_MUXRATIO, 1, 127);
    this->sendCommand(SSD1351_CMD_SETREMAP, 1, 0x74);
    this->sendCommand(SSD1351_CMD_STARTLINE, 0);
    this->sendCommand(SSD1351_CMD_DISPLAYOFFSET, 1, 0x0);
    this->sendCommand(SSD1351_CMD_SETGPIO, 1, 0x00);
    this->sendCommand(SSD1351_CMD_FUNCTIONSELECT, 1, 0x01); // internal (diode drop)
    this->sendCommand(SSD1351_CMD_PRECHARGE, 1, 0x32);
    this->sendCommand(SSD1351_CMD_PRECHARGELEVEL, 1, 0x1F);
    this->sendCommand(SSD1351_CMD_VCOMH, 1, 0x05);
    this->sendCommand(SSD1351_CMD_NORMALDISPLAY, 0);
    this->sendCommand(SSD1351_CMD_CONTRASTMASTER, 1, 0x0A);
    this->sendCommand(SSD1351_CMD_CONTRASTABC, 3, 0xC8, 0x80, 0xC8);
    this->sendCommand(SSD1351_CMD_SETVSL, 3, 0xA0, 0xB5, 0x55);
    this->sendCommand(SSD1351_CMD_PRECHARGE2, 1, 0x01);
    this->sendCommand(SSD1351_CMD_DISPLAYON, 0);  // Main screen turn on
}

void Ssd1351::drawImage(libcamera::Span<uint8_t>& data)
{
    this->setChipSelect(true);
    this->setAddrWindow(0, 0, SSD1351WIDTH-1, SSD1351HEIGHT-1);
    this->m_spi->write(data.data(), data.size());
    this->setChipSelect(false);
}

void Ssd1351::drawPixel(int16_t x, int16_t y, uint16_t colour)
{
    if ((x >= 0) && (x < SSD1351WIDTH) && (y >= 0) && (y < SSD1351HEIGHT)) {
        uint8_t buffer[2] = {(uint8_t)(colour>>8), (uint8_t) (colour & 0x00FF)};
        this->setChipSelect(true);
        this->setAddrWindow(x, y, 1, 1);
        this->m_spi->write(buffer, 2);
        this->setChipSelect(false);
    }
}

void Ssd1351::fillWithColour(uint16_t colour)
{
    uint32_t numPixels = SSD1351WIDTH * SSD1351HEIGHT;
    uint8_t buffer[numPixels * 2];
    for (uint32_t i = 0; i < numPixels*2; i+=2)
    {
        buffer[i] = (uint8_t)(colour>>8);
        buffer[i] = (uint8_t)(colour & 0xFF);
    }
    this->setChipSelect(true);
    this->setAddrWindow(0, 0, SSD1351WIDTH-1, SSD1351HEIGHT-1);
    this->m_spi->write(buffer, numPixels*2);
    this->setChipSelect(false);
}

void Ssd1351::sendCommand(uint8_t byte, uint8_t *argBuffer, int bufferLen)
{
    digitalWrite(this->m_dc, LOW);
    this->setChipSelect(true);
    this->m_spi->write(&byte, 1);
    this->setChipSelect(false);
    if (bufferLen > 0)
        this->m_spi->write(argBuffer, bufferLen);
}

void Ssd1351::sendCommand(uint8_t byte, uint8_t arg1)
{
    this->sendCommand(byte, &arg1, 1);
}

void Ssd1351::sendCommand(uint8_t byte, uint8_t arg1, uint8_t arg2)
{
    uint8_t buffer[2] = {arg1, arg2};
    this->sendCommand(byte, buffer, 2);
}

void Ssd1351::sendCommand(uint8_t byte, uint8_t arg1, uint8_t arg2, uint8_t arg3, uint8_t arg4)
{
    uint8_t buffer[4] = {arg1, arg2, arg3, arg4};
    this->sendCommand(byte, buffer, 4);
}

void Ssd1351::setChipSelect(bool asserted)
{
    if (asserted)
        digitalWrite(this->m_cs, LOW);
    else
        digitalWrite(this->m_cs, HIGH);
}

/*!
    @brief   Set the "address window" - the rectangle we will write to
             graphics RAM with the next chunk of SPI data writes. The
             SSD1351 will automatically wrap the data as each row is filled.
    @param   x1
             Leftmost column of rectangle (screen pixel coordinates).
    @param   y1
             Topmost row of rectangle (screen pixel coordinates).
    @param   w
             Width of rectangle.
    @param   h
             Height of rectangle.
    @return  None (void).
*/
void Ssd1351::setAddrWindow(uint16_t x1, uint16_t y1, uint16_t w, uint16_t h)
{

    uint16_t x2 = x1 + w - 1, y2 = y1 + h - 1;
    this->sendCommand(SSD1351_CMD_SETCOLUMN, x1, x2); // X range
    this->sendCommand(SSD1351_CMD_SETROW, y1, y2); // Y range
    this->sendCommand(SSD1351_CMD_WRITERAM, nullptr, 0); // Begin write
}

void Ssd1351::reset()
{
    if (this->m_rst != -1)
    {
        digitalWrite(this->m_rst, LOW);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        digitalWrite(this->m_rst, HIGH);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

Ssd1351::~Ssd1351()
{
    this->m_spi.reset();
}

