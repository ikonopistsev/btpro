#pragma once

#ifdef _WIN32
#include "btpro/win/win.hpp"
#include "btpro/wsa/wsa.hpp"
#else
#include "btpro/posix/posix.hpp"
#endif // _WIN32

#include "event2/event.h"

#include <memory>
#include <chrono>

#include <cstdint>
#include <cstdlib>
#include <cassert>
#include <cstring>

namespace btpro {

using event_flag = short;
using event_pointer = event*;
using queue_pointer = event_base*;

template<class Rep, class Period>
timeval make_timeval(std::chrono::duration<Rep, Period> timeout) noexcept
{
    const auto sec = std::chrono::duration_cast<
        std::chrono::seconds>(timeout);
    const auto usec = std::chrono::duration_cast<
        std::chrono::microseconds>(timeout) - sec;

    // FIX: decltype fix for clang macosx
    return {
        static_cast<decltype(timeval::tv_sec)>(sec.count()),
        static_cast<decltype(timeval::tv_usec)>(usec.count())
    };
}

static inline void startup(unsigned char h = 2, unsigned char l = 2)
{
    static const net::launch launch(h, l);
}

namespace detail {

static inline void check_result(const char* what, int result)
{
    assert(what);
    if (code::fail == result)
        throw std::runtime_error(what);
}

template<class P>
P* check_pointer(const char* what, P* value)
{
    assert(what);
    if (!value)
        throw std::runtime_error(what);
    return value;
}

template<class T>
std::size_t check_size(const char* what, T result)
{
    assert(what);
    if (static_cast<T>(code::fail) == result)
        throw std::runtime_error(what);
    return static_cast<std::size_t>(result);
}

} // namespace detail
} // namespace btpro

namespace be = btpro;
