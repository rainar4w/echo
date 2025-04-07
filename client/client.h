#pragma once

#include <string>

#include <netinet/in.h>

#include <helpers/helpers.hpp>
#include <logger/logger.h>

class Client : public Socket
{
public:
    explicit Client(Logger &logger);

    bool connect(const char *server, int port);
    bool connect(int port);
    bool send(const char *msg);
    bool receive(std::string &msg, size_t expectedSize);

private:
    Logger &m_logger;
    bool m_connected;

    bool checkConnected() const;
    bool connect(const in_addr_t address, int port);
};

