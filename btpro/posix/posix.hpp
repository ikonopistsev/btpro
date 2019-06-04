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
static constexpr auto invalid = socket_t{ BTPRO_INVALID_SOCKET };
static constexpr auto eacces = int{ BTPRO_SOCK_ERROR(EACCES) };
static constexpr auto eaddrnoavail = int{ BTPRO_SOCK_ERROR(EADDRNOTAVAIL) };
static constexpr auto eisconn = int{ BTPRO_SOCK_ERROR(EISCONN) };
static constexpr auto enotconn = int{ BTPRO_SOCK_ERROR(ENOTCONN) };
static constexpr auto ealready = int{ BTPRO_SOCK_ERROR(EALREADY) };
static constexpr auto ewouldblock = int{ BTPRO_SOCK_ERROR(EWOULDBLOCK) };
static constexpr auto einprogress = int{ BTPRO_SOCK_ERROR(EINPROGRESS) };
static constexpr auto econnrefused = int{ BTPRO_SOCK_ERROR(ECONNREFUSED) };
static constexpr auto econnaborted = int{ BTPRO_SOCK_ERROR(ECONNABORTED) };
static constexpr auto econnreset = int{ BTPRO_SOCK_ERROR(ECONNRESET) };
static constexpr auto ebadf = int{ BTPRO_SOCK_ERROR(EBADF) };
static constexpr auto efault = int{ BTPRO_SOCK_ERROR(EFAULT) };
static constexpr auto ehostunreach = int{ BTPRO_SOCK_ERROR(EHOSTUNREACH) };
static constexpr auto eintr = int{ BTPRO_SOCK_ERROR(EINTR) };
static constexpr auto einval = int{ BTPRO_SOCK_ERROR(EINVAL) };
static constexpr auto emsgsize = int{ BTPRO_SOCK_ERROR(EMSGSIZE) };
static constexpr auto enetdown = int{ BTPRO_SOCK_ERROR(ENETDOWN) };
static constexpr auto enetreset = int{ BTPRO_SOCK_ERROR(ENETRESET) };
static constexpr auto enetunreach = int{ BTPRO_SOCK_ERROR(ENETUNREACH) };
static constexpr auto emfile = int{ BTPRO_SOCK_ERROR(EMFILE) };
static constexpr auto enobufs = int{ BTPRO_SOCK_ERROR(ENOBUFS) };
static constexpr auto eshutdown = int{ BTPRO_SOCK_ERROR(ESHUTDOWN) };
static constexpr auto etimedout = int{ BTPRO_SOCK_ERROR(ETIMEDOUT) };
static constexpr auto eperm = int{ EPERM };
static constexpr auto eagain = int{ EAGAIN };
static constexpr auto epipe = int{ EPIPE };
static constexpr auto enomem = int{ ENOMEM };
#undef BTPRO_SOCK_ERROR

} // namespace posix

namespace net = posix;
namespace sys = posix;

} // namespace btpro
