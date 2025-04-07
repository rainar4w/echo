#include <array>
#include <memory>
#include <string>
#include <sstream>
#include <thread>

#include <errno.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/epoll.h>

#include <helpers/helpers.hpp>
#include <logger/logger.h>

#include "server_config.h"

using namespace std;

#define LOG_DEBUG(msg) getLogger().log(Logger::Level::DEBUG, msg)
#define LOG_INFO(msg) getLogger().log(Logger::Level::INFO, msg)
#define LOG_ERROR(msg) getLogger().log(Logger::Level::ERROR, msg)

#define _count_of(a) (sizeof(a) / sizeof(*a))

#define SERVER_EVENTS (EPOLLIN | EPOLLET)
// need EPOLLONESHOT to avoid being triggered by multiple writes while processing echo in separate thread
#define CLIENT_EVENTS (EPOLLIN | EPOLLET | EPOLLHUP | EPOLLRDHUP | EPOLLONESHOT)

namespace
{
    Logger &getLogger()
    {
        static std::unique_ptr<Logger> l{LoggerFactory::getFileLogger(LOG_FILE, LOG_LEVEL)};
        //static std::unique_ptr<Logger> l{LoggerFactory::getConsoleLogger(LOG_LEVEL)};
        return *l;
    }

    shared_ptr<Socket> createSocket()
    {
        shared_ptr<Socket> s{make_shared<Socket>(AF_INET, SOCK_STREAM, 0)};

        struct sockaddr_in a{};
        a.sin_family = AF_INET;
        a.sin_addr.s_addr = INADDR_ANY;
        a.sin_port = htons(PORT);

        const int fd = *s;
        if (-1 == bind(fd, reinterpret_cast<struct sockaddr *>(&a), sizeof(a)))
        {
            LOG_ERROR("Failed to bind");
            return nullptr;
        }

        if (-1 == listen(fd, MAX_CONN))
        {
            LOG_ERROR("Failed to listen");
            return nullptr;
        }

        return s;
    }

    shared_ptr<Socket> &getServerSocket()
    {
        static shared_ptr<Socket> s = createSocket();
        return s;
    }

    Epoll &getEpoll()
    {
        static Epoll ep;
        return ep;
    }

    bool handleServerEvent(const int32_t events)
    {
        LOG_DEBUG("Handling server event");

        if (events & ~EPOLLIN)
        {
            // do not expect event other than EPOLLIN
            LOG_ERROR("Unexpected event");
            return false;
        }

        while (1)
        {
            const int result = accept(*getServerSocket(), nullptr, nullptr);
            if (-1 == result)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    // all connections are processed
                    break;
                }

                // continue to process other connections
                LOG_ERROR("Failed to accept connection");
                continue;
            }

            std::shared_ptr<FD> client{std::make_shared<FD>(result)};
            if (getEpoll().addNonblocking(client, CLIENT_EVENTS))
                LOG_DEBUG("Client connection opened");
        }

        return true;
    }

    void handleEcho(FD &&socket)
    {
        // data is supposed to be human-readable string
        std::vector<char> msg;
        char buffer[SERVER_BUFFEER_SIZE] = {0};
        ssize_t num;
        while (0 < (num = read(socket, static_cast<void *>(buffer), sizeof(buffer))))
        {
            msg.insert(msg.end(), buffer, buffer + num);
        }

        if (-1 == num && errno != EAGAIN && errno != EWOULDBLOCK)
        {
            LOG_ERROR("Failed to read from socket");
            return;
        }

        LOG_DEBUG("Data read");
        if (-1 == write(socket, static_cast<const void *>(msg.data()), msg.size()))
        {
            LOG_ERROR("Failed to write to socket");
            return;
        }

        LOG_DEBUG("Data sent");
        LOG_INFO(string(msg.begin(), msg.end()).c_str());
    }

    void handleClientEvent(const int fd, const uint32_t events)
    {
        LOG_DEBUG("Handling client event");

        bool isAlive = true;
        uint32_t leftover = events;

        if (events & (EPOLLRDHUP | EPOLLHUP))
        {
            // connection closed
            LOG_DEBUG("Client connection closed");
            getEpoll().remove(fd);
            isAlive = false;

            leftover &= ~(EPOLLRDHUP | EPOLLHUP);
        }

        if (events & EPOLLERR)
        {
            // error happened
            LOG_ERROR("Error happened on client connection");
            getEpoll().remove(fd);
            isAlive = false;

            leftover &= ~(EPOLLERR);
        }

        if (isAlive && (events & EPOLLIN))
        {
            // incoming data
            LOG_DEBUG("Incoming client data event");

            // handle as separate thread to avoid block on read/write of big data
            // number of threads created is limited by maximum number of connections
            thread t([fd]
                     {
                         handleEcho(FD(dup(fd)));
                         if (!getEpoll().rearm(fd, CLIENT_EVENTS))
                         {
                             LOG_ERROR("Failed to rearm client socket, removing it");
                             getEpoll().remove(fd);
                         }
                     });
            t.detach();

            leftover &= ~(EPOLLIN);
        }
        else if (events & EPOLLIN)
        {
            leftover &= ~(EPOLLIN);
        }

        if (leftover)
        {
            LOG_ERROR("Unexpected event");
        }
    }

    [[maybe_unused]] string eventsToString(uint32_t events)
    {
        constexpr static std::array<std::pair<EPOLL_EVENTS, const char *>, 7> array = {{
            {EPOLLIN, "EPOLLIN"},
            {EPOLLOUT, "EPOLLOUT"},
            {EPOLLRDHUP, "EPOLLRDHUP"},
            {EPOLLPRI, "EPOLLPRI"},
            {EPOLLERR, "EPOLLERR"},
            {EPOLLHUP, "EPOLLHUP"},
            {EPOLLONESHOT, "EPOLLONESHOT"},
        }};

        ostringstream oss;
        for (const auto &e : array)
        {
            if (events & e.first)
                oss << e.second << " ";
        }

        return oss.str();
    }
} // namespace

int main()
{
    LOG_DEBUG("Server starting");

    if (!getEpoll().addNonblocking(getServerSocket(), SERVER_EVENTS))
        return -1;

    struct epoll_event events[MAX_EVENTS] = {0};
    while (1)
    {
        const int num = epoll_wait(getEpoll(), events, _count_of(events), -1);
        if (-1 == num)
        {
            LOG_ERROR("Failed to wait");
            break;
        }

        for (int i = 0; i < num; ++i)
        {
            const struct epoll_event &e = events[i];
            if (*getServerSocket() == e.data.fd)
            {
                // server socket event
                if (!handleServerEvent(e.events))
                    break;
            }
            else
            {
                // client connection event
                handleClientEvent(e.data.fd, e.events);
            }
        }
    }

    LOG_DEBUG("Server finished");
    return 0;
}
