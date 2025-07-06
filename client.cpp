#include "client.hpp"
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>

Client::Client(const char *host, uint16_t port)
{
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    m_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = inet_addr(host);
    bzero(&serverAddress.sin_zero, 8);

    // sending connection request
    connect(m_socket_fd, (struct sockaddr*)&serverAddress,
            sizeof(serverAddress));

}

void Client::sendImage(uint8_t *buffer, uint32_t length, uint16_t sequence)
{
    uint8_t header[6];
    header[0] = sequence >> 8;
    header[1] = sequence & 0xFF;
    header[2] = length >> 24;
    header[3] = length >> 16;
    header[4] = length >> 8;
    header[5] = length & 0xFF;
    send(m_socket_fd, header, 6, 0);
    uint32_t bytes_sent = 0;
    while (bytes_sent != length)
    {
        bytes_sent += send(m_socket_fd, &buffer[bytes_sent], length-bytes_sent, 0);
    }
}