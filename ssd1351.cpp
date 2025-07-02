#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wiringPi.h>
#include <iostream>
#include <thread>
#include <chrono>
#include "ssd1351.hpp"

//static spi_config_t spi_config;

#define SPI_SPEED 8000000

Ssd1351::Ssd1351(const char *spi_dev, int cs, int dc, int rst)
    : m_cs(cs), m_dc(dc), m_rst(rst)
{
    this->m_spi = std::make_unique<spidevpp::Spi>(spi_dev);
    this->m_spi->setBitsPerWord(8);
    this->m_spi->setSpeed(SPI_SPEED);
    /*if (!this->m_spi->begin())
    {
        std::cerr << "Unable to open SPI, display won't work" << std::endl;
    }*/
    wiringPiSetup();
    wiringPiSetupGpio();

    pinMode(this->m_cs, OUTPUT);
    pinMode(this->m_dc, OUTPUT);

    // INIT DISPLAY ------------------------------------------------------------
    this->reset();
    this->sendCommand(SSD1351_CMD_COMMANDLOCK, 0x12); // Set command lock, 1 arg
    this->sendCommand(SSD1351_CMD_COMMANDLOCK, 0xB1); // Set command lock, 1 arg
    this->sendCommand(SSD1351_CMD_DISPLAYOFF); // Display off, no args
    this->sendCommand(SSD1351_CMD_DISPLAYENHANCE, 0xA4, 0x00, 0x00);
    this->sendCommand(SSD1351_CMD_CLOCKDIV, 0xF0); // 7:4 = Oscillator Freq, 3:0 = CLK Div Ratio (A[3:0]+1 = 1..16)
    this->sendCommand(SSD1351_CMD_MUXRATIO, 127);
    this->sendCommand(SSD1351_CMD_SETREMAP, 0x74);
    this->sendCommand(SSD1351_CMD_STARTLINE);
    this->sendCommand(SSD1351_CMD_DISPLAYOFFSET, 0x0);
    this->sendCommand(SSD1351_CMD_SETGPIO, 0x00);
    this->sendCommand(SSD1351_CMD_FUNCTIONSELECT, 0x01); // internal (diode drop)
    this->sendCommand(SSD1351_CMD_PRECHARGE, 0x32);
    this->sendCommand(SSD1351_CMD_PRECHARGELEVEL, 0x1F);
    this->sendCommand(SSD1351_CMD_VCOMH, 0x05);
    this->sendCommand(SSD1351_CMD_NORMALDISPLAY);
    this->sendCommand(SSD1351_CMD_CONTRASTMASTER, 0x0A);
    this->sendCommand(SSD1351_CMD_CONTRASTABC, 0xFF, 0xFF, 0xFF);
    this->sendCommand(SSD1351_CMD_SETVSL, 0xA0, 0xB5, 0x55);
    this->sendCommand(SSD1351_CMD_PRECHARGE2, 0x01);
    this->sendCommand(SSD1351_CMD_DISPLAYON);  // Main screen turn on
    this->fillWithColour(0x0); // black / clear the screen
}

void Ssd1351::drawImage(libcamera::Span<uint8_t>& data)
{
    char buffer[data.size()];
    memccpy(buffer, data.data(), sizeof(char), data.size());
    this->setAddrWindow(0, 0, SSD1351WIDTH, SSD1351HEIGHT);
    this->sendData(buffer, data.size());
    std::cout << "Writing image data: " << data.size() << std::endl;
}

void Ssd1351::drawPixel(int16_t x, int16_t y, uint16_t colour)
{
    if ((x >= 0) && (x < SSD1351WIDTH) && (y >= 0) && (y < SSD1351HEIGHT)) {
        char buffer[2] = {(char)(colour>>8), (char) (colour & 0x00FF)};
        this->setAddrWindow(x, y, 1, 1);
        this->sendData(buffer, 2);
    }
}

void Ssd1351::fillWithColour(uint16_t colour)
{
    uint32_t numPixels = SSD1351WIDTH * SSD1351HEIGHT;
    char buffer[2048];
    for (uint32_t i = 0; i < 2048; i+=2)
    {
        buffer[i] = (uint8_t)((colour>>8)&0xFF);
        buffer[i+1] = (uint8_t)(colour & 0xFF);
    }

    for (uint8_t x = 0; x < SSD1351WIDTH; x += 8)
    {
        this->setAddrWindow(x, 0, 8, SSD1351HEIGHT);
        this->sendData(buffer, 2048);
    }
}

void Ssd1351::sendCommand(char byte, char *argBuffer, int bufferLen)
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

void Ssd1351::sendCommand(char byte)
{
    this->sendCommand(byte, nullptr, 0);
}

void Ssd1351::sendCommand(char byte, char arg1)
{
    this->sendCommand(byte, &arg1, 1);
}

void Ssd1351::sendCommand(char byte, char arg1, char arg2)
{
    char buffer[2] = {arg1, arg2};
    this->sendCommand(byte, buffer, 2);
}

void Ssd1351::sendCommand(char byte, char arg1, char arg2, char arg3)
{
    char buffer[3] = {arg1, arg2, arg3};
    this->sendCommand(byte, buffer, 3);
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
    this->sendCommand(SSD1351_CMD_WRITERAM); // Begin write
}

void Ssd1351::sendData(char *buffer, int bufferLen)
{
    digitalWrite(this->m_dc, HIGH);
    this->setChipSelect(true);
    this->m_spi->write(buffer, bufferLen);
    this->setChipSelect(false);
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

