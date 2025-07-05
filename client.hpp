#include <cstdint>
#include <string>


class Client {
    private:
        int m_socket_fd;
    public:
        Client(const char *host, uint16_t port);
        void sendImage(uint8_t *buffer, uint32_t length, uint16_t sequence);
};
