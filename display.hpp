#include <cstdint>
#include <memory>
#include <libcamera/libcamera.h>
#include <spidevpp/spi.h>

class Display {
    protected:
        int m_cs;
        int m_dc;
        int m_rst;
        std::unique_ptr<spidevpp::Spi> m_spi;
    public:
        Display(const char *spi_dev, int spi_speed, int cs, int dc, int rst = -1);
        virtual void drawImage(libcamera::Span<uint8_t>& data) = 0;
        virtual void drawPixel(int16_t x, int16_t y, uint16_t color) = 0;
        virtual void fillWithColour(uint16_t colour) = 0;
        virtual void displayOff() = 0;
        ~Display();
    protected:
        void sendCommand(uint8_t cmd, uint8_t *buffer, int bufferLen);
        void sendCommand(uint8_t byte);
        void sendCommand(uint8_t byte, uint8_t arg1);
        void sendCommand(uint8_t byte, uint8_t arg1, uint8_t arg2);
        void sendCommand(uint8_t byte, uint8_t arg1, uint8_t arg2, uint8_t arg3);
        void sendCommand(uint8_t byte, uint8_t arg1, uint8_t arg2, uint8_t arg3, uint8_t arg4);
        void sendCommand(uint8_t byte, uint8_t arg1, uint8_t arg2, uint8_t arg3, uint8_t arg4, uint8_t arg5);
        void sendData(uint8_t *buffer, int bufferLen);
        void setChipSelect(bool asserted);
        virtual void setAddrWindow(uint16_t x1, uint16_t y1, uint16_t w, uint16_t h) = 0;
        void reset();
};
