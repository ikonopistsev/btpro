#pragma once

#include "btpro/ip/addr.hpp"
#include "btdef/string.hpp"
#include "btdef/text.hpp"

namespace btpro {
namespace ip {
namespace v4 {

class addr
    : public ip::addr
{
public:
    typedef decltype(sockaddr_in::sin_port) port_type;
    static const ev_socklen_t capacity = INET_ADDRSTRLEN;

    static inline const sockaddr_in& empty_sockaddr_in() noexcept
    {
        static const auto res = create_sockaddr_in();
        return res;
    }

    static inline sockaddr_in create_sockaddr_in() noexcept
    {
        sockaddr_in res;
        std::memset(&res, 0, sizeof(res));
        return res;
    }

private:
    sockaddr_in sockaddr_in_ = empty_sockaddr_in();

    static inline std::string parse(const std::string& str, int& port)
    {
        if (str.empty())
            return str;

        auto f = str.rfind(':');
        if (f != std::string::npos)
        {
            if (!port)
            {
                // чтобы не копировать строку
                auto port_str = std::string_view(str).substr(f + 1);
                if (!port_str.empty())
                    port = std::atoi(port_str.data());
            }

            return str.substr(0, f);
        }

        return str;
    }

    sockaddr* selfaddr() noexcept
    {
        return reinterpret_cast<sockaddr*>(&sockaddr_in_);
    }

public:
    addr() noexcept
        : ip::addr(selfaddr())
    {   }

    addr(const addr&) = default;
    addr& operator=(const addr&) = default;

    addr(const ip::addr& ip, int port = 0)
        : ip::addr(selfaddr())
    {
        assign(ip.sa(), ip.size(), port);
    }

    addr& operator=(const ip::addr& ip)
    {
        assign(ip.sa(), ip.size());
        return *this;
    }

    explicit addr(const sockaddr_in& sin) noexcept
        : ip::addr(selfaddr())
    {
        assign(sin);
    }

    addr(const sockaddr *sa, ev_socklen_t salen, int port = 0)
        : ip::addr(selfaddr())
    {
        assign(sa, salen, port);
    }

    explicit addr(const std::string& addr_str, int port = 0)
        : ip::addr(selfaddr())
    {
        assign(addr_str, port);
    }

    explicit addr(in_addr_t in_addr, int port = 0) noexcept
        : ip::addr(selfaddr())
    {
        assign(in_addr, port);
    }

    void assign(const sockaddr_in& sin) noexcept
    {
        sockaddr_in_ = sin;

        set_socklen(capacity);
    }

    void assign(const sockaddr *sa, ev_socklen_t salen, int port = 0)
    {
        assert(sa && (salen > 0));
        assert((port >= 0) && (port < 65536));

        if ((capacity < salen) || (AF_INET != sa->sa_family))
            throw std::runtime_error("assign ipv4 sockaddr");

        std::memcpy(&sockaddr_in_, sa, salen);

        if (port)
            sockaddr_in_.sin_port = htons(static_cast<port_type>(port));

        set_socklen(salen);
    }

    void assign(const std::string& str, int port = 0)
    {
        assert((port >= 0) && (port < 65536));

        auto sin = empty_sockaddr_in();
        sin.sin_family = AF_INET;
        auto addr_str = parse(str, port);
        auto res = inet_pton(AF_INET, addr_str.c_str(), &sin.sin_addr);
        if (code::fail == res)
            throw std::system_error(net::error_code(), addr_str);
        if (1 != res)
            throw std::runtime_error("inet_pton");
        if (port)
            sin.sin_port = htons(static_cast<port_type>(port));

        assign(sin);
    }

    void assign(in_addr_t in_addr, int port = 0) noexcept
    {
        assert((port >= 0) && (port < 65536));

        sockaddr_in_.sin_family = AF_INET;
        sockaddr_in_.sin_addr.s_addr = htonl(in_addr);
        if (port)
            sockaddr_in_.sin_port = htons(static_cast<port_type>(port));

        set_socklen(capacity);
    }

    sockaddr_in& data() noexcept
    {
        return sockaddr_in_;
    }

    const sockaddr_in& data() const noexcept
    {
        return sockaddr_in_;
    }

    sockaddr_in* operator->() noexcept
    {
        return &sockaddr_in_;
    }

    const sockaddr_in* operator->() const noexcept
    {
        return &sockaddr_in_;
    }

    int port() const noexcept
    {
        return static_cast<int>(ntohs(sockaddr_in_.sin_port));
    }

private:
    template<class T>
    struct print
    {
        T operator()(const addr& sa)
        {
            T str;
            str.reserve(32);
            auto res = inet_ntop(sa.family(), &sa->sin_addr,
                str.data(), static_cast<ev_socklen_t>(str.capacity()));
            if (res)
                str.resize(std::strlen(res));
            return str;
        }
    };

public:
    btdef::util::text to_text() const noexcept
    {
        print<btdef::util::text> p;
        return p(*this);
    }

    std::string to_string() const
    {
        btdef::util::text text = to_text();
        return std::string(text.data(), text.size());
    }
};

static inline addr loopback() noexcept
{
    static const auto res = addr(INADDR_LOOPBACK);
    return res;
}

static inline addr loopback(int port) noexcept
{
    return addr(INADDR_LOOPBACK, port);
}

static inline addr any() noexcept
{
    static const auto res = addr(INADDR_ANY);
    return res;
}

static inline addr any(int port) noexcept
{
    return addr(INADDR_ANY, port);
}

} // namespace v4
} // namespace ip

namespace ipv4 = ip::v4;

} // namespace btpro

template<class T, class C>
std::basic_ostream<T, C>& operator<<(
    std::basic_ostream<T, C>& os, btpro::ipv4::addr ipv4_addr)
{
    os << ipv4_addr.to_text();
    auto port = ipv4_addr.port();
    if (port)
        os << ':' << port;
    return os;
}

