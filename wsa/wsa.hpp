#pragma once

#include "btpro/wsa/error_code.hpp"
#include <Ws2tcpip.h>

#ifndef BTPRO_INVALID_SOCKET
#define BTPRO_INVALID_SOCKET -1
#endif // BTPRO_INVALID_SOCKET

typedef ULONG in_addr_t;

namespace btpro {
namespace wsa {

struct launch
{
    launch(unsigned char h, unsigned char l)
    {
        WSADATA w;
        auto err = ::WSAStartup(MAKEWORD(h, l), &w);
        if (code::sucsess != err)
            throw std::system_error(error_code(), "::WSAStartup");
    }

    ~launch() noexcept
    {
        ::WSACleanup();
    }
};

typedef evutil_socket_t socket_t;

#define BTPRO_SOCK_ERROR(e) WSA ## e
constexpr static auto invalid = int{ BTPRO_INVALID_SOCKET };
constexpr static auto eacces = int{ BTPRO_SOCK_ERROR(EACCES) };
constexpr static auto eaddrnoavail = int{ BTPRO_SOCK_ERROR(EADDRNOTAVAIL) };
constexpr static auto eisconn = int{ BTPRO_SOCK_ERROR(EISCONN) };
constexpr static auto enotconn = int{ BTPRO_SOCK_ERROR(ENOTCONN) };
constexpr static auto ealready = int{ BTPRO_SOCK_ERROR(EALREADY) };
constexpr static auto ewouldblock = int{ BTPRO_SOCK_ERROR(EWOULDBLOCK) };
constexpr static auto einprogress = int{ BTPRO_SOCK_ERROR(EINPROGRESS) };
constexpr static auto econnrefused = int{ BTPRO_SOCK_ERROR(ECONNREFUSED) };
constexpr static auto econnaborted = int{ BTPRO_SOCK_ERROR(ECONNABORTED) };
constexpr static auto econnreset = int{ BTPRO_SOCK_ERROR(ECONNRESET) };
constexpr static auto ebadf = int{ BTPRO_SOCK_ERROR(EBADF) };
constexpr static auto efault = int{ BTPRO_SOCK_ERROR(EFAULT) };
constexpr static auto ehostunreach = int{ BTPRO_SOCK_ERROR(EHOSTUNREACH) };
constexpr static auto eintr = int{ BTPRO_SOCK_ERROR(EINTR) };
constexpr static auto einval = int{ BTPRO_SOCK_ERROR(EINVAL) };
constexpr static auto emsgsize = int{ BTPRO_SOCK_ERROR(EMSGSIZE) };
constexpr static auto enetdown = int{ BTPRO_SOCK_ERROR(ENETDOWN) };
constexpr static auto enetreset = int{ BTPRO_SOCK_ERROR(ENETRESET) };
constexpr static auto enetunreach = int{ BTPRO_SOCK_ERROR(ENETUNREACH) };
constexpr static auto emfile = int{ BTPRO_SOCK_ERROR(EMFILE) };
constexpr static auto enobufs = int{ BTPRO_SOCK_ERROR(ENOBUFS) };
constexpr static auto eshutdown = int{ BTPRO_SOCK_ERROR(ESHUTDOWN) };
constexpr static auto etimedout = int{ BTPRO_SOCK_ERROR(ETIMEDOUT) };
constexpr static auto eperm = int{ EPERM };
constexpr static auto eagain = int{ EAGAIN };
constexpr static auto epipe = int{ EPIPE };
constexpr static auto enomem = int{ ENOMEM };
#undef BTPRO_SOCK_ERROR

} // namespace wsa

namespace net = wsa;

} // namespace btpro
