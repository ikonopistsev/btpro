#pragma once

#include "btpro/btpro.hpp"

namespace btpro {

#ifdef SOCK_NONBLOCK
#define BTPRO_SOCK_NONBLOCK SOCK_NONBLOCK
#else
#define BTPRO_SOCK_NONBLOCK 0
#endif // SOCK_NONBLOCK

static constexpr auto sock_nonblock = int{ BTPRO_SOCK_NONBLOCK };
static constexpr auto sock_dgram = int{ SOCK_DGRAM|sock_nonblock };
static constexpr auto sock_stream = int{ SOCK_STREAM|sock_nonblock };

template<class T, int SO_LEVEL, int SO_OPTID>
class sock_basic_option
{
protected:
    T val_{};
    const char *name_{nullptr};

public:
    sock_basic_option(T val, const char *name) noexcept
        : val_(std::move(val))
        , name_(name)
    {   }

    void apply(evutil_socket_t fd) const
    {
        auto res = ::setsockopt(fd, SO_LEVEL, SO_OPTID, 
            reinterpret_cast<const char*>(&val_), sizeof(val_));
        if (code::fail == res)
            throw std::system_error(net::error_code(), name_);
    }
};

#ifdef SO_REUSEADDR
#ifdef _WIN32
namespace exclusive_addr_use {

namespace detail {
    static const char name[] = "SO_EXCLUSIVEADDRUSE";
    typedef sock_basic_option<int, SOL_SOCKET, SO_EXCLUSIVEADDRUSE> type;
} // namespace detail

static inline detail::type on() noexcept
{
    return detail::type(1, detail::name);
}

static inline detail::type off() noexcept
{
    return detail::type(0, detail::name);
}

} // namespace exclusive_addr_use
#endif // WIN32

namespace reuse_addr {

namespace detail {
    static const char name[] = "SO_REUSEADDR";
#ifdef _WIN32
    class reuse_impl
        : public sock_basic_option<int, SOL_SOCKET, SO_REUSEADDR>
    {
    public:
        reuse_impl(int value, const char *name) noexcept
            : sock_basic_option(value, name)
        {   }

        void apply(evutil_socket_t fd) const
        {
            if (val_)
            {
                exclusive_addr_use::off().apply(fd);
                sock_basic_option::apply(fd);
            }
            else
            {
                sock_basic_option::apply(fd);
                exclusive_addr_use::on().apply(fd);
            }
        }
    };
    typedef reuse_impl type;
#else
    typedef sock_basic_option<int, SOL_SOCKET, SO_REUSEADDR> type;
#endif // _WIN32
} // namespace detail

static inline detail::type on() noexcept
{
    return detail::type(1, detail::name);
}

static inline detail::type off() noexcept
{
    return detail::type(0, detail::name);
}

} // namespace reuse_addr
#endif // SO_REUSEADDR

#ifdef SO_REUSEPORT
namespace reuse_port {
namespace detail {
    static const char name[] = "SO_REUSEPORT";
    typedef sock_basic_option<int, SOL_SOCKET, SO_REUSEPORT> type;
} // namespace detail
    static inline detail::type on() noexcept
    {
        return detail::type(1, detail::name);
    }
    static inline detail::type off() noexcept
    {
        return detail::type(0, detail::name);
    }
} // namespace reuse_port
#endif // SO_REUSEPORT


namespace sndbuf {
namespace detail {
    static const char name[] = "SO_SNDBUF";
    typedef sock_basic_option<int, SOL_SOCKET, SO_SNDBUF> type;
} // namespace detail

static inline detail::type size(int value) noexcept
{
    return detail::type(value, detail::name);
}
} // namespace sndbuf

namespace rcvbuf {
namespace detail {
    static const char name[] = "SO_RCVBUF";
    typedef sock_basic_option<int, SOL_SOCKET, SO_RCVBUF> type;
} // namespace detail

static inline detail::type size(int value) noexcept
{
    return detail::type(value, detail::name);
}
} // namespace sndbuf

} // namespace btpro
