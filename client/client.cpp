#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>

#include "config.h"
#include "client.h"

Client::Client(Logger &logger) : Socket(AF_INET, SOCK_STREAM, 0), m_logger(logger), m_connected(false)
{
}

bool Client::connect(const char *server, int port)
{
    in_addr_t address{};
    if (1 != inet_pton(AF_INET, server, static_cast<void *>(&address)))
    {
        m_logger.log(Logger::Level::ERROR, "Failed to convert address from string");
        return false;
    }

    return connect(address, port);
}

bool Client::connect(int port)
{
    return connect(INADDR_ANY, port);
}

bool Client::send(const char *msg)
{
    if (!checkConnected())
        return false;

    // send terminating 0 as well to be able to send empty string
    ssize_t num = write(*this, static_cast<const void *>(msg), strlen(msg) + 1);
    if (-1 == num)
    {
        m_logger.log(Logger::Level::ERROR, "Failed to write to socket");
        return false;
    }

    return true;
}

bool Client::receive(std::string &msg, size_t expectedSize)
{
    if (!checkConnected())
        return false;

    // account for terminating 0
    ++expectedSize;
    std::vector<char> data;
    data.reserve(expectedSize);

    char buffer[CLIENT_BUFFER_SIZE] = {0};
    for (ssize_t num = 0; 0 != expectedSize; expectedSize -= num)
    {
        num = read(*this, buffer, sizeof(buffer));
        if (-1 == num)
        {
            m_logger.log(Logger::Level::ERROR, "Failed to read from socket");
            return false;
        }

        if (expectedSize < static_cast<size_t>(num))
        {
            m_logger.log(Logger::Level::ERROR, "Read more than expected");
            return false;
        }

        data.insert(data.end(), buffer, buffer + num);
    }

    msg.assign(data.begin(), data.end() - 1);
    return true;
}

bool Client::checkConnected() const
{
    if (!m_connected)
    {
        m_logger.log(Logger::Level::ERROR, "Client is not connected");
        return false;
    }

    return true;
}

bool Client::connect(const in_addr_t address, int port)
{

    struct sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = address;
    a.sin_port = htons(port);

    if (-1 == ::connect(*this, reinterpret_cast<struct sockaddr *>(&a), sizeof(a)))
    {
        m_logger.log(Logger::Level::ERROR, "Failed to connect to the server");
        return false;
    }

    m_connected = true;

    return true;
}
