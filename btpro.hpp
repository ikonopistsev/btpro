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

typedef short event_flag_t;
typedef event* event_handle_t;
typedef event_base* queue_handle_t;

template<class Rep, class Period>
static timeval make_timeval(std::chrono::duration<Rep, Period> timeout) noexcept
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

struct tag_ref
{
    constexpr static bool is_ref = true;
};

struct tag_obj
{
    constexpr static bool is_ref = false;
};

} // namespace btpro

namespace be = btpro;
