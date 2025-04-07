#pragma once

#include <algorithm>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>
#include <vector>

#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

class FD
{
public:
    explicit FD(const int fd) : m_fd(fd)
    {
        if (m_fd < 0)
            throw std::runtime_error("Invalid fd");
    }

    FD(const FD &fd) = delete;
    const FD &operator=(const FD &fd) = delete;
    FD(FD &&fd) = delete;
    const FD &operator=(FD &&fd) = delete;

    virtual ~FD()
    {
        release();
    }

    operator int() const
    {
        return m_fd;
    }

private:
    constexpr static int INVALID_FD = -1;
    int m_fd;

    void release()
    {
        if (m_fd > 0)
            close(m_fd);
        m_fd = FD::INVALID_FD;
    }
};

class Socket : public FD
{
public:
    Socket(const int domain, const int type, const int protocol)
        : FD(socket(domain, type, protocol)) {}
};

class Epoll : public FD
{
public:
    Epoll() : FD(epoll_create(1)) {}

    bool addNonblocking(std::shared_ptr<FD> fd, const uint32_t events)
    {
        struct epoll_event e{};
        e.data.fd = *fd;
        e.events = events;

        if (-1 == fcntl(*fd, F_SETFL, fcntl(*fd, F_GETFL, 0) | O_NONBLOCK))
            return false;

        if (-1 == epoll_ctl(*this, EPOLL_CTL_ADD, *fd, &e))
            return false;

        {
            std::lock_guard<std::mutex> lock(m_fdsMutex);
            m_fds.push_back(fd);
        }

        return true;
    }

    bool rearm(const int fd, const uint32_t events)
    {
        struct epoll_event e{};
        e.data.fd = fd;
        e.events = events;

        return  (-1 != epoll_ctl(*this, EPOLL_CTL_MOD, fd, &e));
    }

    bool remove(const int fd)
    {
        if (-1 == epoll_ctl(*this, EPOLL_CTL_DEL, fd, nullptr))
            return false;

        {
            std::lock_guard<std::mutex> lock(m_fdsMutex);
            auto it = std::remove_if(m_fds.begin(), m_fds.end(), [fd](const auto &elem)
                                     { return *elem == fd; });
            m_fds.erase(it, m_fds.end());
        }

        return true;
    }

private:
    std::mutex m_fdsMutex;
    std::vector<std::shared_ptr<FD>> m_fds;
};