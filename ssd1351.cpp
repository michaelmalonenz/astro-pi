#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wiringPi.h>
#include <iostream>
#include <thread>
#include <chrono>
#include "ssd1351.hpp"

#define SPI_SPEED 8000000

#define REMAP_VALUE (SSD1351_REMAP_HORIZONTAL | SSD1351_REMAP_0_FIRST | SSD1351_REMAP_COLOUR_ORDER_RGB | SSD1351_REMAP_ENABLE_COM_SPLIT | SSD1351_REMAP_SCAN_N_TO_0 | SSD1351_REMAP_262K_COLOURS)


Ssd1351::Ssd1351(const char *spi_dev, int cs, int dc, int rst)
    : Display(spi_dev, SPI_SPEED, cs, dc, rst)
{
    this->m_spi = std::make_unique<spidevpp::Spi>(spi_dev);
    this->m_spi->setBitsPerWord(8);
    this->m_spi->setSpeed(SPI_SPEED);
    wiringPiSetup();
    wiringPiSetupGpio();

    pinMode(this->m_cs, OUTPUT);
    pinMode(this->m_dc, OUTPUT);

    // INIT DISPLAY ------------------------------------------------------------
    this->reset();

    // consider waiting 300ms before sending commands?
    this->sendCommand(SSD1351_CMD_COMMANDLOCK, 0x12); // Set command lock, 1 arg
    this->sendCommand(SSD1351_CMD_COMMANDLOCK, 0xB1); // Set command lock, 1 arg
    this->sendCommand(SSD1351_CMD_DISPLAYOFF); // Display off, no args
    this->sendCommand(SSD1351_CMD_DISPLAYENHANCE, 0xA4, 0x00, 0x00); // performance mode
    this->sendCommand(SSD1351_CMD_CLOCKDIV, 0xF1);
    this->sendCommand(SSD1351_CMD_MUXRATIO, 0x7F);
    this->sendCommand(SSD1351_CMD_DISPLAYOFFSET, 0x0);
    this->sendCommand(SSD1351_CMD_STARTLINE, 0x00);
    this->sendCommand(SSD1351_CMD_SETREMAP, REMAP_VALUE);
    this->sendCommand(SSD1351_CMD_SETGPIO, 0x00);
    this->sendCommand(SSD1351_CMD_FUNCTIONSELECT, 0x01); // internal (diode drop), set SPI mode
    this->sendCommand(SSD1351_CMD_SETVSL, 0xA0, 0xB5, 0x55);
    this->sendCommand(SSD1351_CMD_CONTRASTABC, 0xFF, 0xFF, 0xFF);
    this->sendCommand(SSD1351_CMD_CONTRASTMASTER, 0x0F);

    uint8_t gamma_values[63] = {
        0x02, 0x03, 0x04, 0x05,
        0x06, 0x07, 0x08, 0x09,
        0x0A, 0x0B, 0x0C, 0x0D,
        0x0E, 0x0F, 0x10, 0x11,
        0x12, 0x13, 0x15, 0x17,
        0x19, 0x1B, 0x1D, 0x1F,
        0x21, 0x23, 0x25, 0x27,
        0x2A, 0x2D, 0x30, 0x33,
        0x36, 0x39, 0x3C, 0x3F,
        0x42, 0x45, 0x48, 0x4C,
        0x50, 0x54, 0x58, 0x5C,
        0x60, 0x64, 0x68, 0x6C,
        0x70, 0x74, 0x78, 0x7D,
        0x82, 0x87, 0x8C, 0x91,
        0x96, 0x9B, 0xA0, 0xA5,
        0xAA, 0xAF, 0xB4,
    };
    this->sendCommand(SSD1351_CMD_SETGRAY, gamma_values, 63);
    this->sendCommand(SSD1351_CMD_PRECHARGE, 0x32);

    uint8_t display_enhance_values[3] = {0xA4, 0x00, 0x00};
    this->sendCommand(SSD1351_CMD_DISPLAYENHANCE, display_enhance_values, 3);
    this->sendCommand(SSD1351_CMD_PRECHARGELEVEL, 0x17);
    this->sendCommand(SSD1351_CMD_PRECHARGE2, 0x01);
    this->sendCommand(SSD1351_CMD_VCOMH, 0x05);
    this->sendCommand(SSD1351_CMD_NORMALDISPLAY);
    this->sendCommand(SSD1351_CMD_DISPLAYON);  // Main screen turn on
    this->fillWithColour(0x0); // black / clear the screen
}

void Ssd1351::drawImage(libcamera::Span<uint8_t>& data)
{
    uint8_t *buffer = data.data();
    for (uint8_t y = 0; y < SSD1351HEIGHT; y += 8)
    {
        this->setAddrWindow(0, y, SSD1351WIDTH, 8);
        this->sendData(&buffer[y*384], 3072);
    }
}

void Ssd1351::drawPixel(int16_t x, int16_t y, uint32_t colour)
{
    if ((x >= 0) && (x < SSD1351WIDTH) && (y >= 0) && (y < SSD1351HEIGHT)) {
        uint8_t buffer[2] = {(uint8_t)(colour>>8), (uint8_t) (colour & 0x00FF)};
        this->setAddrWindow(x, y, 1, 1);
        this->sendData(buffer, 2);
    }
}

void Ssd1351::fillWithColour(uint32_t colour)
{
    uint32_t numPixels = SSD1351WIDTH * SSD1351HEIGHT;
    uint8_t buffer[3072];
    for (uint32_t i = 0; i < 3072; i+=3)
    {
        buffer[i]   = (uint8_t)((colour >> 18) & 0xFF);
        buffer[i+1] = (uint8_t)((colour >> 10) & 0xFF);
        buffer[i+2] = (uint8_t)((colour >> 2 ) & 0xFF);
    }

    for (uint8_t x = 0; x < SSD1351WIDTH; x += 8)
    {
        this->setAddrWindow(x, 0, 8, SSD1351HEIGHT);
        this->sendData(buffer, 3072);
    }
}

void Ssd1351::displayOff()
{
    this->sendCommand(SSD1351_CMD_DISPLAYOFF); // Display off, no args
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

