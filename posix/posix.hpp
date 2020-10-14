#pragma once

#include "btpro/posix/error_code.hpp"

#include "event2/util.h"

#include <netinet/tcp.h>
#include <arpa/inet.h>

#ifndef BTPRO_INVALID_SOCKET
#define BTPRO_INVALID_SOCKET -1
#endif // BTPRO_INVALID_SOCKET

namespace btpro {

struct code
{
    enum {
        sucsess = 0,
        fail = -1
    };
};

namespace posix {

struct launch
{
    launch(unsigned char, unsigned char)
    {   }
};

typedef evutil_socket_t socket_t;

#define BTPRO_SOCK_ERROR(e) e
constexpr static auto invalid = socket_t{ BTPRO_INVALID_SOCKET };
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

} // namespace posix

namespace net = posix;
namespace sys = posix;

} // namespace btpro
