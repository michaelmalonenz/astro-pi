#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <spidev_lib++.h>

static spi_config_t spi_config;
static uint8_t tx_buffer[32];
static uint8_t rx_buffer[32];

namespace astro_pi {
    void init_device()
    {
        SPI *mySPI = NULL;
        spi_config.mode = 0;
        spi_config.speed = 1000000;
        spi_config.delay = 0;
        spi_config.bits_per_word = 8;
        mySPI = new SPI("/dev/spidev0.0", &spi_config);
        if (mySPI->begin())
        {
            sprintf((char *)tx_buffer, "hello world");
            printf("sending %s, to spidev2.0 in full duplex \n ", (char *)tx_buffer);
            mySPI->xfer(tx_buffer, strlen((char *)tx_buffer), rx_buffer, strlen((char *)tx_buffer));
            printf("rx_buffer=%s\n", (char *)rx_buffer);
            // mySPI->end();
            delete mySPI;
        }
    }
}
