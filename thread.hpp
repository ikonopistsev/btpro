#pragma once

#include "btpro/btpro.hpp"

#include "event2/thread.h"

namespace btpro {

#ifdef _WIN32
static inline void use_threads()
{
    auto res = evthread_use_windows_threads();
    if (code::sucsess != res)
        throw std::runtime_error("evthread_use_windows_threads");
}
#else
static inline void use_threads()
{
    auto res = evthread_use_pthreads();
    if (code::sucsess != res)
        throw std::runtime_error("evthread_use_pthreads");
}
#endif // _WIN32

} // namespace btpro

