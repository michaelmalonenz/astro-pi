#include <wiringPi.h>
#include "tp28017.hpp"

#define SPI_SPEED 80000000

#define BYTES_PER_PIXEL 3
#define BUFFER_STRIDE 8

Tp28017::Tp28017(int cs, int rs, int rd, int wr, int rst)
    : m_cs(cs), m_cd(rs), m_rd(rd), m_wr(wr), m_rst(rst)
{
    // setup all the pins for output
    for (int i = 0; i < 8; i++) {
        pinMode(i, OUTPUT);
    }
    pinMode(m_cs, OUTPUT);
    pinMode(m_cd, OUTPUT);
    pinMode(m_rd, OUTPUT);
    pinMode(m_wr, OUTPUT);
    if (m_rst != -1)
        pinMode(m_rst, OUTPUT);

    digitalWrite(m_rd, HIGH);
    digitalWrite(m_wr, LOW);

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
    this->sendCommand(TP28017_VSCRSADD, 0x00);           // Vertical scroll zero
    this->sendCommand(TP28017_PIXFMT, 0x55);             // 18 bits per pixel
    this->sendCommand(TP28017_FRMCTR1, 0x00, 0x18);
    this->sendCommand(TP28017_DFUNCTR, 0x08, 0x82, 0x27); // Display Function Control
    this->sendCommand(0xF2, 0x00);                         // 3Gamma Function Disable
    this->sendCommand(TP28017_GAMMASET, 0x01);             // Gamma curve selected

    uint8_t buffer[15] = {0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, // Set Positive Gamma
    0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00};
    this->sendCommand(buffer, 15, TP28017_GMCTRP1);

    uint8_t buffer2[15] = {0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, // Set Negative Gamma
    0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F};
    this->sendCommand(buffer2, 15, TP28017_GMCTRN1);
    this->sendCommand(TP28017_SLPOUT, 0x80);                // Exit Sleep
    this->sendCommand(TP28017_DISPON, 0x80);                // Display on
}

void Tp28017::drawImage(libcamera::Span<uint8_t>& data)
{
    int buffer_size = TP28017_TFTHEIGHT * BUFFER_STRIDE * BYTES_PER_PIXEL;
    for (uint32_t y = 0; y < TP28017_TFTHEIGHT; y += 8)
    {
        this->setAddrWindow(0, y, TP28017_TFTWIDTH, 8);
        this->sendData(&data[y*TP28017_TFTHEIGHT*BYTES_PER_PIXEL], buffer_size);
    }
}

void Tp28017::drawPixel(int16_t x, int16_t y, uint32_t colour)
{
    uint8_t buffer[BYTES_PER_PIXEL] = {
        (uint8_t) (colour >> 16 & 0xFF),
        (uint8_t) (colour >>  8 & 0xFF),
        (uint8_t) (colour       & 0xFF),
    };
    this->setAddrWindow(x, y, 1, 1);
    this->sendData(buffer, BYTES_PER_PIXEL);
}

void Tp28017::fillWithColour(uint32_t colour)
{
    int buffer_size = TP28017_TFTHEIGHT * BUFFER_STRIDE * BYTES_PER_PIXEL;
    uint8_t buffer[buffer_size];
    for (uint32_t i = 0; i < buffer_size; i+=BYTES_PER_PIXEL)
    {
        buffer[i]   = (uint8_t)((colour >> 16) & 0xFF);
        buffer[i+1] = (uint8_t)((colour >>  8) & 0xFF);
        buffer[i+2] = (uint8_t)((colour      ) & 0xFF);
    }

    for (uint8_t x = 0; x < TP28017_TFTWIDTH; x += BUFFER_STRIDE)
    {
        this->setAddrWindow(x, 0, BUFFER_STRIDE, TP28017_TFTHEIGHT);
        this->sendData(buffer, buffer_size);
    }
}

void Tp28017::displayOff()
{
    this->sendCommand(TP28017_DISPOFF, 0x80);
}

void Tp28017::setAddrWindow(uint16_t x1, uint16_t y1, uint16_t w, uint16_t h)
{
    static uint16_t old_x1 = 0xffff, old_x2 = 0xffff;
    static uint16_t old_y1 = 0xffff, old_y2 = 0xffff;

    uint8_t buffer[2];
    uint16_t x2 = (x1 + w - 1), y2 = (y1 + h - 1);
    if (x1 != old_x1 || x2 != old_x2) {
        this->sendCommand(TP28017_CASET); // Column address set
        this->writeData16(x1);
        this->writeData16(x2);
        old_x1 = x1;
        old_x2 = x2;
    }
    if (y1 != old_y1 || y2 != old_y2) {
        this->sendCommand(TP28017_PASET); // Row address set
        this->writeData16(y1);
        this->writeData16(y2);
        old_y1 = y1;
        old_y2 = y2;
    }
    this->sendCommand(TP28017_RAMWR); // Write to RAM
}


//  // General macros.   IOCLR registers are 1 cycle when optimised.
// #define WR_STROBE { WR_ACTIVE; WR_IDLE; }       //PWLW=TWRL=50ns
// #define RD_STROBE RD_IDLE, RD_ACTIVE, RD_ACTIVE, RD_ACTIVE      //PWLR=TRDL=150ns, tDDR=100ns

// #if !defined(GPIO_INIT)
// #define GPIO_INIT()
// #endif
// #define CTL_INIT()   { GPIO_INIT(); RD_OUTPUT; WR_OUTPUT; CD_OUTPUT; CS_OUTPUT; RESET_OUTPUT; }
// #define WriteCmd(x)  { CD_COMMAND; write16(x); CD_DATA; }
// #define WriteData(x) { write16(x); }



void Tp28017::sendData(uint8_t *buffer, int bufferLen)
{
    for(int i = 0; i < bufferLen; i++)
    {
        digitalWriteByte(buffer[i]);
    }
}

void Tp28017::sendCommand(uint8_t *args, int argLen, int command)
{
    digitalWrite(this->m_cs, LOW);  // chip-select
    digitalWrite(this->m_cd, LOW);  // set command mode
    digitalWriteByte(command);      // write the command
    digitalWrite(this->m_cd, HIGH); // set data mode
    if (argLen > 0)
        sendData(args, argLen);     // write the data
    digitalWrite(this->m_cs, HIGH); // deselect the chip
}

void Tp28017::sendCommand(uint8_t cmd)
{
    sendCommand(nullptr, 0, cmd);
}

void Tp28017::sendCommand(uint8_t cmd, uint8_t arg1)
{
    sendCommand(&arg1, 1, cmd);
}

void Tp28017::sendCommand(uint8_t cmd, uint8_t arg1, uint8_t arg2)
{
    uint8_t args[2] = {arg1, arg2};
    sendCommand(args, 2, cmd);
}

void Tp28017::sendCommand(uint8_t cmd, uint8_t arg1, uint8_t arg2, uint8_t arg3)
{
    uint8_t args[3] = {arg1, arg2, arg3};
    sendCommand(args, 3, cmd);
}

void Tp28017::sendCommand(uint8_t cmd, uint8_t arg1, uint8_t arg2, uint8_t arg3, uint8_t arg4)
{
    uint8_t args[4] = {arg1, arg2, arg3, arg4};
    sendCommand(args, 4, cmd);
}

void Tp28017::sendCommand(uint8_t cmd, uint8_t arg1, uint8_t arg2, uint8_t arg3, uint8_t arg4, uint8_t arg5)
{
    uint8_t args[5] = {arg1, arg2, arg3, arg4, arg5};
    sendCommand(args, 5, cmd);
}

void Tp28017::writeData16(uint16_t data)
{
    digitalWriteByte((data >> 8));
    digitalWriteByte((data & 0xFF));
}

