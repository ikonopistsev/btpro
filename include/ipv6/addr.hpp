#pragma once

#include "btpro/ip/addr.hpp"
#include "btdef/text.hpp"

namespace btpro {
namespace ip {
namespace v6 {

class addr
    : public ip::addr
{
public:
    typedef decltype(sockaddr_in6::sin6_port) port_type;
    static const ev_socklen_t capacity = INET6_ADDRSTRLEN;

    static inline const sockaddr_in6& empty_sockaddr_in6() noexcept
    {
        static const auto res = create_sockaddr_in6();
        return res;
    }

    static inline sockaddr_in6 create_sockaddr_in6() noexcept
    {
        sockaddr_in6 res;
        std::memset(&res, 0, sizeof(res));
        return res;
    }

private:
    sockaddr_in6 sockaddr_in6_ = empty_sockaddr_in6();

    static inline std::string parse(const std::string& str, int& port)
    {
        if (str.empty())
            return str;

        // если начинается со скобки
        if (str.front() == '[')
        {
            // находим закрывающую скобку
            auto e = str.rfind(']');
            if (std::string::npos == e)
                throw std::runtime_error("parse: " + str);

            if (!port)
            {
                auto f = e + 2;
                if (f < str.size())
                {
                    // чтобы не копировать строку
                    auto port_str = std::string_view(str).substr(f);
                    if (!port_str.empty())
                        port = std::atoi(port_str.data());
                }
            }

            // пропускаем скобки
            return str.substr(1, e - 1);
        }

        return str;
    }

    sockaddr* selfaddr() noexcept
    {
        return reinterpret_cast<sockaddr*>(&sockaddr_in6_);
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

    explicit addr(const sockaddr_in6& sin6) noexcept
        : ip::addr(selfaddr())
    {
        assign(sin6);
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

    explicit addr(const in6_addr& in_addr, int port = 0) noexcept
        : ip::addr(selfaddr())
    {
        assign(in_addr, port);
    }

    void assign(const sockaddr_in6& sin6) noexcept
    {
        sockaddr_in6_ = sin6;
        set_socklen(capacity);
    }

    void assign(const sockaddr *sa, ev_socklen_t salen, int port = 0)
    {
        assert(sa && (salen > 0));
        assert((port >= 0) && (port < 65536));

        if ((capacity < salen) || (AF_INET6 != sa->sa_family))
            throw std::runtime_error("assign ipv6 sockaddr");

        std::memcpy(&sockaddr_in6_, sa, salen);

        if (port)
            sockaddr_in6_.sin6_port = htons(static_cast<port_type>(port));

        set_socklen(capacity);
    }

    void assign(const std::string& str, int port = 0)
    {
        assert((port >= 0) && (port < 65536));

        auto sin6 = empty_sockaddr_in6();
        sin6.sin6_family = AF_INET6;
        auto addr_str = parse(str, port);
        auto res = inet_pton(AF_INET6, addr_str.c_str(), &sin6.sin6_addr);
        if (code::fail == res)
            throw std::system_error(net::error_code(), addr_str);
        if (1 != res)
            throw std::runtime_error("inet_pton");
        if (port)
            sin6.sin6_port = htons(static_cast<port_type>(port));

        assign(sin6);
    }

    void assign(const in6_addr& in_addr, int port = 0) noexcept
    {
        assert((port >= 0) && (port < 65536));

        sockaddr_in6_.sin6_family = AF_INET6;
        sockaddr_in6_.sin6_addr = in_addr;
        if (port)
            sockaddr_in6_.sin6_port = htons(static_cast<port_type>(port));

        set_socklen(capacity);
    }

    sockaddr_in6& data() noexcept
    {
        return sockaddr_in6_;
    }

    const sockaddr_in6& data() const noexcept
    {
        return sockaddr_in6_;
    }

    sockaddr_in6* operator->() noexcept
    {
        return &sockaddr_in6_;
    }

    const sockaddr_in6* operator->() const noexcept
    {
        return &sockaddr_in6_;
    }

    int port() const noexcept
    {
        return static_cast<int>(ntohs(sockaddr_in6_.sin6_port));
    }

private:
    template<class T>
    struct print
    {
        T operator()(const addr& sa)
        {
            T str;
            str.reserve(128);
            auto res = inet_ntop(sa.family(), &sa->sin6_addr,
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
        auto text = to_text();
        return std::string(text.data(), text.size());
    }
};

static inline addr loopback() noexcept
{
#ifdef _WIN32    
    static const in6_addr in_addr = { { IN6ADDR_LOOPBACK_INIT } };
#else
    static const in6_addr in_addr = IN6ADDR_LOOPBACK_INIT;
#endif 
    static const auto res = addr(in_addr);
    return res;
}

static inline addr loopback(int port) noexcept
{
#ifdef _WIN32        
    static const in6_addr in_addr = { { IN6ADDR_LOOPBACK_INIT } };
#else
    static const in6_addr in_addr = IN6ADDR_LOOPBACK_INIT;
#endif 
    return addr(in_addr, port);
}

static inline addr any() noexcept
{
    static const in6_addr in_addr = IN6ADDR_ANY_INIT;
    static const auto res = addr(in_addr);
    return res;
}

static inline addr any(int port) noexcept
{
    static const in6_addr in_addr = IN6ADDR_ANY_INIT;
    return addr(in_addr, port);
}

} // namespace v6
} // namespace ipv4

namespace ipv6 = ip::v6;

} // namespace btpro

template<class T, class C>
std::basic_ostream<T, C>& operator<<(
    std::basic_ostream<T, C>& os, btpro::ipv6::addr ipv6_addr)
{
    auto port = ipv6_addr.port();
    if (port)
        os << '[' << ipv6_addr.to_text() << ']' << ':' << port;
    else
        os << ipv6_addr.to_text();
    return os;
}

