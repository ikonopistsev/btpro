#pragma once

#include "btpro/sock_addr.hpp"
#include "btpro/sock_opt.hpp"

namespace btpro {

class socket
{
    evutil_socket_t socket_{ net::invalid };

    void do_close() noexcept
    {
        evutil_closesocket(socket_);
        socket_ = net::invalid;
    }

#ifndef SOCK_NONBLOCK
    void make_socket_nonblocking()
    {
        auto res = evutil_make_socket_nonblocking(fd());
        if (code::fail == res)
            throw std::system_error(net::error_code(), "SOCK_NONBLOCK");
    }
#endif // SOCK_NONBLOCK

public:
    socket() = default;

    explicit socket(evutil_socket_t fd) noexcept
        : socket_(fd)
    {   }

    evutil_socket_t fd() const noexcept
    {
        return socket_;
    }

    bool good() const noexcept
    {
        return fd() != net::invalid;
    }

    void bind(const sockaddr *sa, ev_socklen_t len)
    {
        auto res = ::bind(socket_, sa, len);
        if (code::fail == res)
            throw std::system_error(net::error_code(), "::bind");
    }

    void bind(const ip::addr& addr)
    {
        bind(addr.sa(), addr.size());
    }

    void listen(int backlog)
    {
        auto res = ::listen(socket_, backlog);
        if (code::fail == res)
            throw std::system_error(net::error_code(), "::listen");
    }

    template<class V, int L, int I>
    void set(const sock_basic_option<V, L, I>& opt)
    {
        opt.apply(fd());
    }

    template<class V, int L, int I, class... T>
    void set(const sock_basic_option<V, L, I>& opt, const T& ...opts)
    {
        set(opt);
        set(opts...);
    }

    // create socket only
    // if you need blocking socket use attach( blocking_sock )
    void create(int domain, int type)
    {
        assert(!good());
        socket_ = ::socket(domain, type|sock_nonblock, 0);
        if (code::fail == socket_)
            throw std::system_error(net::error_code(), "::socket");

#ifndef SOCK_NONBLOCK
        make_socket_nonblocking();
#endif // SOCK_NONBLOCK
    }

    // create and bind socket addr
    void create(const ip::addr& addr, int type)
    {
        create(addr.family(), type);
        bind(addr);
    }

    template<class... T>
    void create(int domain, int type, const T& ...opts)
    {
        create(domain, type);
        set(opts...);
    }

    // create and bind socket addr
    template<class... T>
    void create(const ip::addr& addr, int type, const T& ...opts)
    {
        create(addr.family(), type, opts...);
        bind(addr);
    }

    void attach(evutil_socket_t socket) noexcept
    {
        socket_ = socket;
    }

    void detach()
    {
        socket_ = net::invalid;
    }

    void close() noexcept
    {
        if (good())
            do_close();
    }

    int error() const noexcept
    {
        return evutil_socket_geterror(socket_);
    }

    static inline bool wouldblock() noexcept
    {
        auto err = net::error();
        return (btpro::net::ewouldblock == err) || (btpro::net::eagain == err);
    }

    static inline bool inprogress() noexcept
    {
        auto err = net::error();
        return btpro::net::einprogress == err;
    }

    ev_ssize_t send(const char *buf, std::size_t len, int flags = 0) noexcept
    {
        return ::send(socket_, buf, static_cast<int>(len), flags);
    }

    ev_ssize_t recv(char *buf, std::size_t len, int flags = 0) noexcept
    {
        return ::recv(socket_, buf, static_cast<int>(len), flags);
    }

    ev_ssize_t sendto(const ip::addr& addr,
        const char *buf, std::size_t len, int flags = 0) noexcept
    {
        return ::sendto(socket_, buf, static_cast<int>(len),
                        flags, addr.sa(), addr.size());
    }

    ev_ssize_t recvfrom(sock_addr& sa,
        char *buf, std::size_t len, int flags = 0) noexcept
    {
        ev_socklen_t salen = sock_addr::capacity;
        auto res = ::recvfrom(socket_, buf, static_cast<int>(len),
                              flags, sa.sa(), &salen);
        if (res != code::fail)
            sa.resize(salen);

        return res;
    }

    class holder
    {
        socket& socket_;

    public:
        holder(socket& socket) noexcept
            : socket_(socket)
        {   }

        ~holder() noexcept
        {
            socket_.close();
        }
    };
};

} // namespace btpro
