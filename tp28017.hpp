#pragma once

#include "display.hpp"



#define TP28017_TFTWIDTH 240  ///< TP28017 max TFT width
#define TP28017_TFTHEIGHT 320 ///< TP28017 max TFT height

#define TP28017_NOP 0x00     ///< No-op register
#define TP28017_SWRESET 0x01 ///< Software reset register
#define TP28017_RDDID 0x04   ///< Read display identification information
#define TP28017_RDDST 0x09   ///< Read Display Status

#define TP28017_SLPIN 0x10  ///< Enter Sleep Mode
#define TP28017_SLPOUT 0x11 ///< Sleep Out
#define TP28017_PTLON 0x12  ///< Partial Mode ON
#define TP28017_NORON 0x13  ///< Normal Display Mode ON

#define TP28017_RDMODE 0x0A     ///< Read Display Power Mode
#define TP28017_RDMADCTL 0x0B   ///< Read Display MADCTL
#define TP28017_RDPIXFMT 0x0C   ///< Read Display Pixel Format
#define TP28017_RDIMGFMT 0x0D   ///< Read Display Image Format
#define TP28017_RDSELFDIAG 0x0F ///< Read Display Self-Diagnostic Result

#define TP28017_INVOFF 0x20   ///< Display Inversion OFF
#define TP28017_INVON 0x21    ///< Display Inversion ON
#define TP28017_GAMMASET 0x26 ///< Gamma Set
#define TP28017_DISPOFF 0x28  ///< Display OFF
#define TP28017_DISPON 0x29   ///< Display ON

#define TP28017_CASET 0x2A ///< Column Address Set
#define TP28017_PASET 0x2B ///< Page Address Set
#define TP28017_RAMWR 0x2C ///< Memory Write
#define TP28017_RAMRD 0x2E ///< Memory Read

#define TP28017_PTLAR 0x30    ///< Partial Area
#define TP28017_VSCRDEF 0x33  ///< Vertical Scrolling Definition
#define TP28017_MADCTL 0x36   ///< Memory Access Control
#define TP28017_VSCRSADD 0x37 ///< Vertical Scrolling Start Address
#define TP28017_PIXFMT 0x3A   ///< COLMOD: Pixel Format Set

#define TP28017_FRMCTR1                                                        \
  0xB1 ///< Frame Rate Control (In Normal Mode/Full Colors)
#define TP28017_FRMCTR2 0xB2 ///< Frame Rate Control (In Idle Mode/8 colors)
#define TP28017_FRMCTR3                                                        \
  0xB3 ///< Frame Rate control (In Partial Mode/Full Colors)
#define TP28017_INVCTR 0xB4  ///< Display Inversion Control
#define TP28017_DFUNCTR 0xB6 ///< Display Function Control

#define TP28017_PWCTR1 0xC0 ///< Power Control 1
#define TP28017_PWCTR2 0xC1 ///< Power Control 2
#define TP28017_PWCTR3 0xC2 ///< Power Control 3
#define TP28017_PWCTR4 0xC3 ///< Power Control 4
#define TP28017_PWCTR5 0xC4 ///< Power Control 5
#define TP28017_VMCTR1 0xC5 ///< VCOM Control 1
#define TP28017_VMCTR2 0xC7 ///< VCOM Control 2

#define TP28017_RDID1 0xDA ///< Read ID 1
#define TP28017_RDID2 0xDB ///< Read ID 2
#define TP28017_RDID3 0xDC ///< Read ID 3
#define TP28017_RDID4 0xDD ///< Read ID 4

#define TP28017_GMCTRP1 0xE0 ///< Positive Gamma Correction
#define TP28017_GMCTRN1 0xE1 ///< Negative Gamma Correction
// #define TP28017_PWCTR6     0xFC


class Tp28017 : public Display {
    public:
        Tp28017(const char *spi_dev, int cs, int dc, int rst = -1);
        void drawImage(libcamera::Span<uint8_t>& data) override;
        void drawPixel(int16_t x, int16_t y, uint16_t color) override;
        void fillWithColour(uint16_t colour) override;
        void displayOff() override;
    protected:
        void setAddrWindow(uint16_t x1, uint16_t y1, uint16_t w, uint16_t h) override;
};
