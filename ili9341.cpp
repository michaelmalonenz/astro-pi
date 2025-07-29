#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wiringPi.h>
#include <iostream>
#include "ili9341.hpp"


#define SPI_SPEED 8000000

#define BYTES_PER_PIXEL 3


ILI9341::ILI9341(const char *spi_dev, int cs, int dc, int rst)
    : Display(spi_dev, SPI_SPEED, cs, dc, rst)
{
    m_spi = std::make_unique<spidevpp::Spi>(spi_dev);
    m_spi->setBitsPerWord(8);
    m_spi->setSpeed(SPI_SPEED);

    pinMode(m_cs, OUTPUT);
    pinMode(m_dc, OUTPUT);

    // INIT DISPLAY ------------------------------------------------------------
    reset();

    sendCommand(0xEF, 0x03, 0x80, 0x02);
    sendCommand(0xCF, 0x00, 0xC1, 0x30);
    sendCommand(0xED, 0x64, 0x03, 0x12, 0x81);
    sendCommand(0xE8, 0x85, 0x00, 0x78);
    sendCommand(0xCB, 0x39, 0x2C, 0x00, 0x34, 0x02);
    sendCommand(0xF7, 0x20);
    sendCommand(0xEA, 0x00, 0x00);
    sendCommand(ILI9341_PWCTR1  , 0x23);             // Power control VRH[5:0]
    sendCommand(ILI9341_PWCTR2  , 0x10);             // Power control SAP[2:0];BT[3:0]
    sendCommand(ILI9341_VMCTR1  , 0x3e, 0x28);       // VCM control
    sendCommand(ILI9341_VMCTR2  , 0x86);             // VCM control2
    sendCommand(ILI9341_MADCTL  , 0x48);             // Memory Access Control
    sendCommand(ILI9341_VSCRSADD, 0x00);             // Vertical scroll zero
    sendCommand(ILI9341_PIXFMT  , 0x55);             // 18 bits per pixel
    sendCommand(ILI9341_FRMCTR1 , (uint8_t)0x00, 0x18);
    sendCommand(ILI9341_DFUNCTR , 0x08, 0x82, 0x27); // Display Function Control
    sendCommand(0xF2, 0x00);                         // 3Gamma Function Disable
    sendCommand(ILI9341_GAMMASET, 0x01);             // Gamma curve selected

    uint8_t buffer[15] = {0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, // Set Positive Gamma
    0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00};
    sendCommand(ILI9341_GMCTRP1, buffer, 15);

    uint8_t buffer2[15] = {0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, // Set Negative Gamma
    0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F};
    sendCommand(ILI9341_GMCTRN1, buffer2, 15);

    sendCommand(ILI9341_SLPOUT);                // Exit Sleep
    sendCommand(ILI9341_DISPON);                // Display on

    fillWithColour(0x0); // black / clear the screen
}

void ILI9341::drawImage(libcamera::Span<uint8_t>& data)
{
    uint8_t *buffer = data.data();
    for (uint8_t y = 0; y < ILI9341_TFTHEIGHT; y += 8)
    {
        this->setAddrWindow(0, y, ILI9341_TFTWIDTH, 8);
        this->sendData(&buffer[y*ILI9341_TFTHEIGHT*BYTES_PER_PIXEL], 3072);
    }
}

void ILI9341::drawPixel(int16_t x, int16_t y, uint32_t colour)
{
    if ((x >= 0) && (x < ILI9341_TFTWIDTH) && (y >= 0) && (y < ILI9341_TFTHEIGHT)) {
        uint8_t buffer[2] = {(uint8_t)(colour>>8), (uint8_t) (colour & 0x00FF)};
        this->setAddrWindow(x, y, 1, 1);
        this->sendData(buffer, 2);
    }
}

void ILI9341::fillWithColour(uint32_t colour)
{
    uint32_t numPixels = ILI9341_TFTWIDTH * ILI9341_TFTHEIGHT;
    uint8_t buffer[3072];
    for (uint32_t i = 0; i < 3072; i+=3)
    {
        buffer[i]   = (uint8_t)((colour >> 18) & 0xFF);
        buffer[i+1] = (uint8_t)((colour >> 10) & 0xFF);
        buffer[i+2] = (uint8_t)((colour >> 2 ) & 0xFF);
    }

    for (uint8_t x = 0; x < ILI9341_TFTWIDTH; x += 8)
    {
        this->setAddrWindow(x, 0, 8, ILI9341_TFTHEIGHT);
        this->sendData(buffer, 3072);
    }
}

void ILI9341::displayOff()
{
    this->sendCommand(ILI9341_DISPOFF, 0x80);
}

/*!
    @brief   Set the "address window" - the rectangle we will write to
             graphics RAM with the next chunk of SPI data writes. The
             ILI9341 will automatically wrap the data as each row is filled.
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
void ILI9341::setAddrWindow(uint16_t x1, uint16_t y1, uint16_t w, uint16_t h)
{
    static uint16_t old_x1 = 0xffff, old_x2 = 0xffff;
    static uint16_t old_y1 = 0xffff, old_y2 = 0xffff;

    uint16_t x2 = (x1 + w - 1), y2 = (y1 + h - 1);
    if (x1 != old_x1 || x2 != old_x2) {
        sendCommand(ILI9341_CASET); // Column address set
        write16Data(x1);
        write16Data(x2);
        old_x1 = x1;
        old_x2 = x2;
    }
    if (y1 != old_y1 || y2 != old_y2) {
        sendCommand(ILI9341_PASET); // Row address set
        write16Data(y1);
        write16Data(y2);
        old_y1 = y1;
        old_y2 = y2;
    }
    sendCommand(ILI9341_RAMWR); // Write to RAM
}

