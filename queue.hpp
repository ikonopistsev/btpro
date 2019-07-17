#pragma once

#include "btpro/config.hpp"

namespace btpro {

class queue
{
public:
    typedef event_base* handle_t;
    typedef void (*callback_fn)(evutil_socket_t, short);

private:
    static inline handle_t create()
    {
        auto res = event_base_new();
        if (!res)
            throw std::runtime_error("event_base_new");
        return res;
    }

    static inline handle_t create(const config& cfg)
    {
        auto res = event_base_new_with_config(cfg.handle());
        if (!res)
            throw std::runtime_error("event_base_new_with_config");
        return res;
    }

    handle_t assert_handle() const noexcept
    {
        auto res = handle_.get();
        assert(res);
        return res;
    }

    std::unique_ptr<event_base, decltype(&event_base_free)>
        handle_{create(), event_base_free};

public:
    template<class T>
    struct proxy
    {
        static inline void call(evutil_socket_t sock, short ef, void *arg)
        {
            (*static_cast<T*>(arg))(sock, ef);
        }
    };

    queue(const queue&) = delete;
    queue& operator=(const queue&) = delete;

    queue() = default;
    queue(queue&&) = default;
    queue& operator=(queue&&) = default;

    explicit queue(const config& cfg)
        : handle_{create(cfg), event_base_free}
    {   }

    handle_t handle() const noexcept
    {
        return handle_.get();
    }

    std::string method() const noexcept
    {
        auto res = event_base_get_method(assert_handle());
        return res ? std::string(res) : std::string();
    }

    static inline std::string version() noexcept
    {
        auto res = event_get_version();
        return (res) ? std::string(res) : std::string();
    }

    int features() const noexcept
    {
        return event_base_get_features(assert_handle());
    }

    /* true - has events */
    /* false - no events */
    bool dispatch()
    {
        auto res = event_base_dispatch(assert_handle());
        if (code::fail == res)
            throw std::runtime_error("event_base_dispatch");
        return code::sucsess == res;
    }

    template<class Rep, class Period>
    bool dispatch(std::chrono::duration<Rep, Period> timeout)
    {
        loopexit(timeout);
        return dispatch();
    }

    bool loop(int flags)
    {
        auto res = event_base_loop(assert_handle(), flags);
        if (code::fail == res)
            throw std::runtime_error("event_base_loop");
        return code::sucsess == res;
    }

    void loopexit(timeval tv)
    {
        auto res = event_base_loopexit(assert_handle(), &tv);
        if (code::fail == res)
            throw std::runtime_error("event_base_loopexit");
    }

    template<class Rep, class Period>
    void loopexit(std::chrono::duration<Rep, Period> timeout)
    {
        loopexit(make_timeval(timeout));
    }

    void loop_break()
    {
        auto res = event_base_loopbreak(assert_handle());
        if (code::fail == res)
            throw std::runtime_error("event_base_loopbreak");
    }

    bool stopped() const noexcept
    {
        return event_base_got_break(assert_handle()) != 0;
    }

#ifdef EVENT_MAX_PRIORITIES
    void priority_init(int level)
    {
        auto res = event_base_priority_init(assert_handle(), level);
        if (res == code::fail)
            throw std::runtime_error("event_base_priority_init");
    }
#endif // EVENT_MAX_PRIORITIES

    timeval gettimeofday_cached() const
    {
        timeval tv;
        auto res = event_base_gettimeofday_cached(assert_handle(), &tv);
        if (code::fail == res)
            throw std::runtime_error("event_base_gettimeofday_cached");
        return tv;
    }

// ------
    void once(evutil_socket_t fd, short ef, timeval tv,
        event_callback_fn fn, void *arg)
    {
        auto res = event_base_once(assert_handle(), fd, ef, fn, arg, &tv);
        if (code::fail == res)
            throw std::runtime_error("event_base_once");
    }

    template<class Rep, class Period>
    void once(evutil_socket_t fd, short ef,
        std::chrono::duration<Rep, Period> timeout,
        event_callback_fn fn, void *arg)
    {
        once(fd, ef, make_timeval(timeout), fn, arg);
    }

    template<class F>
    void once(evutil_socket_t fd, short ef, timeval tv,F& fn)
    {
        once(fd, ef, tv, proxy<F>::call, &fn);
    }

    template<class Rep, class Period, class F>
    void once(evutil_socket_t fd, short ef,
        std::chrono::duration<Rep, Period> timeout, F& fn)
    {
        once(fd, ef, make_timeval(timeout), proxy<F>::call, &fn);
    }

// ------
    void once(short ef, timeval tv, event_callback_fn fn, void *arg)
    {
        once(-1, ef|EV_TIMEOUT, tv, fn, arg);
    }

    template<class Rep, class Period>
    void once(short ef, std::chrono::duration<Rep, Period> timeout,
        event_callback_fn fn, void *arg)
    {
        once(-1, ef|EV_TIMEOUT, make_timeval(timeout), fn, arg);
    }

    template<class F>
    void once(short ef, timeval tv, F& fn)
    {
        once(-1, ef|EV_TIMEOUT, tv, proxy<F>::call, &fn);
    }

    template<class Rep, class Period, class F>
    void once(short ef, std::chrono::duration<Rep, Period> timeout, F& fn)
    {
        once(-1, ef|EV_TIMEOUT, make_timeval(timeout), proxy<F>::call, &fn);
    }

// ------
    void once(timeval tv, event_callback_fn fn, void *arg)
    {
        once(-1, EV_TIMEOUT, tv, fn, arg);
    }

    template<class Rep, class Period>
    void once(std::chrono::duration<Rep, Period> timeout,
        event_callback_fn fn, void *arg)
    {
        once(-1, EV_TIMEOUT, make_timeval(timeout), fn, arg);
    }

    template<class F>
    void once(timeval tv, F& fn)
    {
        once(-1, EV_TIMEOUT, tv, proxy<F>::call, &fn);
    }

    template<class Rep, class Period, class F>
    void once(std::chrono::duration<Rep, Period> timeout, F& fn)
    {
        once(-1, EV_TIMEOUT, make_timeval(timeout), proxy<F>::call, &fn);
    }

// ------
    void once(event_callback_fn fn, void *arg)
    {
        once(-1, EV_TIMEOUT, timeval{0, 1}, fn, arg);
    }

    template<class F>
    void once(F& fn)
    {
        once(-1, EV_TIMEOUT, timeval{0, 1}, proxy<F>::call, &fn);
    }
};

} // namespace btpro
