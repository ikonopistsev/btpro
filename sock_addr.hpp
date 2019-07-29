#pragma once

#include "btpro/ipv4/addr.hpp"
#include "btpro/ipv6/addr.hpp"

namespace btpro {

class sock_addr
    : public ip::addr
{
public:
    static const ev_socklen_t capacity = sizeof(sockaddr_storage);

    static inline sockaddr_storage create_sockaddr_storage() noexcept
    {
        sockaddr_storage res;
        std::memset(&res, 0, capacity);
        return res;
    }

    static inline sockaddr_storage create_sockaddr_storage(
        const sockaddr *sa, ev_socklen_t salen)
    {
        assert(sa && (salen > 0));

        if (salen > capacity)
            throw std::runtime_error("invalid sockaddr size");

        sockaddr_storage s;
        std::memcpy(&s, sa, salen);
        return s;
    }

    static inline sockaddr_storage create_sockaddr_storage(const ip::addr& addr)
    {
        return create_sockaddr_storage(addr.sa(), addr.size());
    }

    static inline const sockaddr_storage& empty_sockaddr_storage() noexcept
    {
        static const auto res = create_sockaddr_storage();
        return res;
    }

private:
    sockaddr_storage sockaddr_storage_ = empty_sockaddr_storage();

    sockaddr* selfaddr() noexcept
    {
        return reinterpret_cast<sockaddr*>(&sockaddr_storage_);
    }

public:
    sock_addr()
        : ip::addr(selfaddr())
    {   }

    sock_addr(const sock_addr& other) noexcept
        : ip::addr(selfaddr(), other.size())
    {
        sockaddr_storage_ = other.sockaddr_storage_;
    }

    sock_addr& operator=(const sock_addr& other) noexcept
    {
        sockaddr_storage_ = other.sockaddr_storage_;
        set_socklen(other.size());
        return *this;
    }

    explicit sock_addr(const sockaddr *sa, ev_socklen_t socklen)
        : ip::addr(selfaddr(), socklen)
        , sockaddr_storage_(create_sockaddr_storage(sa, socklen))
    {   }

    sock_addr(const ip::addr& ip)
        : ip::addr(selfaddr(), ip.size())
        , sockaddr_storage_(create_sockaddr_storage(ip.sa(), ip.size()))
    {   }

    explicit sock_addr(const std::string& str)
        : ip::addr(selfaddr())
    {
        assign(str);
    }

    sock_addr& operator=(const ip::addr& ip)
    {
        assign(ip.sa(), ip.size());
        return *this;
    }

    void assign(const sockaddr *sa, ev_socklen_t salen)
    {
        sockaddr_storage_ = create_sockaddr_storage(sa, salen);

        set_socklen(salen);
    }

    void assign(const ip::addr& ip)
    {
        assign(ip.sa(), ip.size());
    }

    void assign(const char *str, std::size_t size)
    {
        assert(str && (size > 0));

        static const char localhost[] = "localhost";
        static constexpr auto localhost_size = sizeof(localhost) - 1;
        // ':' and port number
        static constexpr auto size_with_port = localhost_size + 1;
        if (evutil_ascii_strncasecmp(str, localhost, localhost_size) == 0)
        {
            if ((size > size_with_port) && (str[localhost_size] == ':'))
                assign(ipv4::loopback(std::atoi(str + size_with_port)));
            else
                throw std::runtime_error("assign: " + std::string(str, size));
        }
        else
        {
            int len = capacity;
            auto res = evutil_parse_sockaddr_port(str, sa(), &len);
            if (code::fail == res)
                throw std::runtime_error("evutil_parse_sockaddr_port");

            set_socklen(static_cast<ev_socklen_t>(len));
        }
    }

    void assign(const std::string& text)
    {
        assign(text.c_str(), text.size());
    }

    void resize(ev_socklen_t salen) noexcept
    {
        set_socklen(salen);
    }

    bool empty() const noexcept
    {
        return size() == 0;
    }

    void clear() noexcept
    {
        sockaddr_storage_ = empty_sockaddr_storage();
        set_socklen(0);
    }

    btdef::util::text to_text() const
    {
        auto fm = family();

        if (fm == AF_INET)
            return ipv4::addr(*this).to_text();
        else if (fm == AF_INET6)
            return ipv6::addr(*this).to_text();

        return btdef::util::text();
    }

    btdef::util::string string() const noexcept
    {
        auto fm = family();

        if (fm == AF_INET)
            return ipv4::addr(*this).string();
        else if (fm == AF_INET6)
            return ipv6::addr(*this).string();

        return btdef::util::string();
    }

    std::string to_string() const
    {
        auto fm = family();

        if (fm == AF_INET)
            return ipv4::addr(*this).to_string();
        else if (fm == AF_INET6)
            return ipv6::addr(*this).to_string();

        return std::string();
    }
};

} // namespace btpro

template<class T, class C>
std::basic_ostream<T, C>& operator<<(
    std::basic_ostream<T, C>& os, const btpro::sock_addr& saddr)
{
    auto fm = saddr.family();

    if (fm == AF_INET)
        os << btpro::ipv4::addr(saddr);
    else if (fm == AF_INET6)
        os << btpro::ipv6::addr(saddr);

    return os;
}
