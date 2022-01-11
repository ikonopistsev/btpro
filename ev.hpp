#pragma once

#include "btpro/evcore.hpp"
#include "btpro/functional.hpp"

namespace btpro {

template<class H, class E>
class basic_ev;

template<class H, class E>
class basic_ev
{
    H handler_{};
    E ev_{};

public:
    basic_ev() = default;

    basic_ev(queue_pointer queue, event_flag ef, timeval tv, H handler)
        : handler_{std::move(handler)}
        , ev_{queue, ef, tv, handler_}
    {   }

    template<class Rep, class Period>
    basic_ev(queue_pointer queue, event_flag ef, 
        std::chrono::duration<Rep, Period> timeout, H handler)
        : handler_{std::move(handler)}
        , ev_{queue, ef, timeout, handler_}
    {   }
};

using ev_heap = evcore<heap_event>;
using ev_stack = evcore<stack_event>;

namespace detail {

template<class H, class E>
using timer_fn = basic_ev<btpro::timer_fn<H>, E>;

template<class T>
using timer = basic_ev<btpro::timer_fun, T>;

} // namespace ev

namespace evs {

template<class T>
using timer_fn = detail::timer_fn<T, ev_stack>;

using timer = detail::timer<ev_stack>;

} // namespace evs

namespace evh {

template<class T>
using timer_fn = detail::timer_fn<T, ev_heap>;

using timer = detail::timer<ev_heap>;

} // namespace evh
} // namespace btpro
