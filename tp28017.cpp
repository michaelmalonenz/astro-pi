#include "tp28017.hpp"

#define SPI_SPEED 8000000


Tp28017::Tp28017(const char *spi_dev, int cs, int dc, int rst)
    : Display(spi_dev, SPI_SPEED, cs, dc, rst)
{
    // INIT DISPLAY ------------------------------------------------------------
    this->sendCommand(0xEF, 0x03, 0x80, 0x02);
    this->sendCommand(0xCF, 0x00, 0xC1, 0x30);
    this->sendCommand(0xED, 0x64, 0x03, 0x12, 0x81);
    this->sendCommand(0xE8, 0x85, 0x00, 0x78);
    this->sendCommand(0xCB, 0x39, 0x2C, 0x00, 0x34, 0x02);
    this->sendCommand(0xF7, 0x20);
    this->sendCommand(0xEA, 0x00, 0x00);
    this->sendCommand(TP28017_PWCTR1, 0x23);             // Power control VRH[5:0]
    this->sendCommand(TP28017_PWCTR2, 0x10);             // Power control SAP[2:0];BT[3:0]
    this->sendCommand(TP28017_VMCTR1, 0x3e, 0x28);       // VCM control
    this->sendCommand(TP28017_VMCTR2, 0x86);             // VCM control2
    this->sendCommand(TP28017_MADCTL, 0x48);             // Memory Access Control
    this->sendCommand(TP28017_VSCRSADD, 0x00);             // Vertical scroll zero
    this->sendCommand(TP28017_PIXFMT, 0x55);
    this->sendCommand(TP28017_FRMCTR1, 0x00, 0x18);
    this->sendCommand(TP28017_DFUNCTR, 0x08, 0x82, 0x27); // Display Function Control
    this->sendCommand(0xF2, 0x00);                         // 3Gamma Function Disable
    this->sendCommand(TP28017_GAMMASET, 0x01);             // Gamma curve selected

    uint8_t buffer[15] = {0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, // Set Gamma
    0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00};
    this->sendCommand(TP28017_GMCTRP1, buffer, 15);

    uint8_t buffer2[15] = {0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, // Set Gamma
    0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F};
    this->sendCommand(TP28017_GMCTRN1, buffer2, 15);
    this->sendCommand(TP28017_SLPOUT, 0x80);                // Exit Sleep
    this->sendCommand(TP28017_DISPON, 0x80);                // Display on
}

void Tp28017::drawImage(libcamera::Span<uint8_t>& data)
{}

void Tp28017::drawPixel(int16_t x, int16_t y, uint16_t color)
{}

void Tp28017::fillWithColour(uint16_t colour)
{}

void Tp28017::displayOff()
{}

void Tp28017::setAddrWindow(uint16_t x1, uint16_t y1, uint16_t w, uint16_t h)
{}
