#pragma once

#include "btpro/config.hpp"
#include "btpro/functional.hpp"
#include <type_traits>
#include <string_view>

namespace btpro {

class queue
{
public:
    using handle_type = queue_pointer;

private:
    handle_type hqueue_{ 
        detail::check_pointer("event_base_new", event_base_new()) };

    static inline void destroy_handle(handle_type hqueue) noexcept
    {
        if (nullptr != hqueue)
            event_base_free(hqueue);
    }

    handle_type assert_handle() const noexcept
    {
        auto hqueue = handle();
        assert(hqueue);
        return hqueue;
    }

public:
    queue() = default;

    queue(const queue& other) = delete;
    queue& operator=(const queue& other) = delete;

    queue(queue&& that) noexcept
    {
        assert(this != &that);
        std::swap(hqueue_, that.hqueue_);
    }

    queue& operator=(queue&& that) noexcept
    {
        assert(this != &that);
        std::swap(hqueue_, that.hqueue_);
        return *this;
    }

    explicit queue(handle_type hqueue) noexcept
        : hqueue_(hqueue)
    {
        assert(hqueue);
    }

    explicit queue(const config& conf) 
        : queue(detail::check_pointer("event_base_new_with_config",
            event_base_new_with_config(conf)))
    {   }

    ~queue() noexcept
    {
        destroy_handle(hqueue_);
    }

    void assign(handle_type hqueue) noexcept
    {
        assert(hqueue);
        hqueue_ = hqueue;
    }

    handle_type handle() const noexcept
    {
        return hqueue_;
    }

    operator handle_type() const noexcept
    {
        return handle();
    }

    bool empty() const noexcept
    {
        return nullptr == handle();
    }

    void destroy() noexcept
    {
        destroy_handle(hqueue_);
        hqueue_ = nullptr;
    }

    std::string_view method() const noexcept
    {
        auto res = event_base_get_method(assert_handle());
        return res ? std::string_view(res) : std::string_view();
    }

    static inline std::string_view version() noexcept
    {
        auto res = event_get_version();
        return (res) ? std::string_view(res) : std::string_view();
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
        detail::check_result("event_base_loopexit",
            event_base_loopexit(assert_handle(), &tv));
    }

    template<class Rep, class Period>
    void loopexit(std::chrono::duration<Rep, Period> timeout)
    {
        loopexit(make_timeval(timeout));
    }

    void loop_break()
    {
        detail::check_result("event_base_loopbreak",
            event_base_loopbreak(assert_handle()));
    }

    bool stopped() const noexcept
    {
        return event_base_got_break(assert_handle()) != 0;
    }

#ifdef EVENT_MAX_PRIORITIES
    void priority_init(int level)
    {
        detail::check_result("event_base_priority_init",
            event_base_priority_init(assert_handle(), level));
    }
#endif // EVENT_MAX_PRIORITIES

    timeval gettimeofday_cached() const
    {
        timeval tv;
        detail::check_result("event_base_gettimeofday_cached",
            event_base_gettimeofday_cached(assert_handle(), &tv));
        return tv;
    }

    void update_cache_time() const
    {
        detail::check_result("event_base_update_cache_time",
            event_base_update_cache_time(assert_handle()));
    }

    operator timeval() const
    {
        return gettimeofday_cached();
    }

//  generic wratpper
    void once(evutil_socket_t fd, event_flag ef, timeval tv,
        event_callback_fn fn, void *arg)
    {
        detail::check_result("event_base_once",
            event_base_once(assert_handle(), fd, ef, fn, arg, &tv));
    }

// socket
    template<class T>
    void once(btpro::socket sock, event_flag ef, timeval tv, socket_fn<T>& fn)
    {
        once(sock.fd(), ef, tv, &btpro::proxy<socket_fn<T>>::call, &fn);
    }

    template<class Rep, class Period, class T>
    void once(btpro::socket sock, event_flag ef,
        std::chrono::duration<Rep, Period> timeout, socket_fn<T>& fn)
    {
        once(sock, ef, make_timeval(timeout), fn);
    }

    template<class T>
    void once(btpro::socket sock, event_flag ef, socket_fn<T>& fn)
    {
        once(sock, ef, timeval{0, 0}, fn);
    }

    void once(btpro::socket sock, event_flag ef, timeval tv, socket_fun fn)
    {
        once(sock.fd(), ef, tv, &proxy<decltype(fn)>::call, 
            new socket_fun(std::move(fn)));
    }

    void once(btpro::socket sock, event_flag ef, timeval tv, 
        std::reference_wrapper<socket_fun> fn_ref)
    {
        once(sock.fd(), ef, tv, &proxy<decltype(fn_ref)>::call, &fn_ref.get());
    }

    template<class Rep, class Period>
    void once(btpro::socket sock, event_flag ef,
        std::chrono::duration<Rep, Period> timeout, socket_fun fn)
    {
        once(sock, ef, make_timeval(timeout), std::move(fn));
    }

    template<class Rep, class Period>
    void once(btpro::socket sock, event_flag ef,
        std::chrono::duration<Rep, Period> timeout, 
        std::reference_wrapper<socket_fun> fn_ref)
    {
        once(sock, ef, make_timeval(timeout), std::move(fn_ref));
    }

    void once(btpro::socket sock, event_flag ef, socket_fun fn)
    {
        once(sock, ef, timeval{0, 0}, std::move(fn));
    }

    void once(btpro::socket sock, event_flag ef, 
        std::reference_wrapper<socket_fun> fn_ref)
    {
        once(sock, ef, timeval{0, 0}, std::move(fn_ref));
    }

    void once(queue& queue, btpro::socket sock, 
        event_flag ef, timeval tv, socket_fun fn)
    {
        once(sock.fd(), ef, tv, proxy<socket_fun>::call, 
            new socket_fun([&, the_fn = std::move(fn)](btpro::socket s, event_flag e) {
                try {
                    queue.once([f = std::move(the_fn), s, e]{
                        try {
                            f(s, e);
                        } 
                        catch(...) 
                        {   }            
                    });
                } 
                catch(...) 
                {   }            
            }));
    }

    template<class Rep, class Period>
    void once(queue& queue, btpro::socket sock, event_flag ef,
        std::chrono::duration<Rep, Period> timeout, socket_fun fn)
    {
        once(queue, sock, ef, make_timeval(timeout), std::move(fn));
    }

    void once(queue& queue, btpro::socket sock, event_flag ef, socket_fun fn)
    {
        once(queue, sock, ef, timeval{0, 0}, std::move(fn));
    }

// --- timer
    template<class T>
    void once(timeval tv, timer_fn<T>& fn)
    {
        once(-1, EV_TIMEOUT, tv, btpro::proxy<timer_fn<T>>::call, &fn);
    }

    template<class Rep, class Period, class T>
    void once(std::chrono::duration<Rep, Period> timeout, timer_fn<T>& fn)
    {
        once(make_timeval(timeout), fn);
    }    

    template<class T>
    void once(timer_fn<T>& fn)
    {
        once(timeval{0, 0}, fn);
    }

    void once(timeval tv, timer_fun fn)
    {
        once(-1, EV_TIMEOUT, tv, proxy<decltype(fn)>::call, 
            new timer_fun(std::move(fn)));
    }

    void once(timeval tv, std::reference_wrapper<timer_fun> fn_ref)
    {
        once(-1, EV_TIMEOUT, tv, proxy<decltype(fn_ref)>::call, &fn_ref.get());
    }

    template<class Rep, class Period>
    void once(std::chrono::duration<Rep, Period> timeout, timer_fun fn)
    {
        once(make_timeval(timeout), std::move(fn));
    }

    template<class Rep, class Period>
    void once(std::chrono::duration<Rep, Period> timeout, 
        std::reference_wrapper<timer_fun> fn_ref)
    {
        once(make_timeval(timeout), std::move(fn_ref));
    }  

    void once(timer_fun fn)
    {
        once(timeval{0, 0}, std::move(fn));
    }

    void once(std::reference_wrapper<timer_fun> fn_ref)
    {
        once(timeval{0, 0}, std::move(fn_ref));
    }

    void once(queue& queue, timeval tv, timer_fun fn)
    {
        once(-1, EV_TIMEOUT, tv, proxy<timer_fun>::call, 
            new timer_fun([&, the_fn = std::move(fn)]{
                try {
                    queue.once(std::move(the_fn));
                }
                catch(...)
                {   }            
            }));
    }

    void once(queue& queue, timeval tv, 
        std::reference_wrapper<timer_fun> fn_ref)
    {
        once(-1, EV_TIMEOUT, tv, proxy<timer_fun>::call, 
            new timer_fun([&, fn_ref]{
                try {
                    queue.once(fn_ref);
                }
                catch(...)
                {   }            
            }));
    }

    template<class Rep, class Period>
    void once(queue& queue, 
        std::chrono::duration<Rep, Period> timeout, timer_fun fn)
    {
        once(queue, make_timeval(timeout), std::move(fn));
    }

    template<class Rep, class Period>
    void once(queue& queue, 
        std::chrono::duration<Rep, Period> timeout, 
        std::reference_wrapper<timer_fun> fn_ref)
    {
        once(queue, make_timeval(timeout), std::move(fn_ref));
    } 

    void once(queue& queue, timer_fun fn)
    {
        once(queue, timeval{0, 0}, std::move(fn));
    }

    void once(queue& queue, std::reference_wrapper<timer_fun> fn_ref)
    {
        once(queue, timeval{0, 0}, std::move(fn_ref));
    }
};

} // namespace btpro
