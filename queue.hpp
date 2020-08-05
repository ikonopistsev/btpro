#pragma once

#include "btpro/config.hpp"
#include <functional>
#include <type_traits>

namespace btpro {

//typedef void (*queue_callback_fn)(short, evutil_socket_t);
typedef std::function<void(short, evutil_socket_t)> queue_fn;

namespace detail {

template<class R>
struct queue_destroy;

template<>
struct queue_destroy<tag_ref>
{
    constexpr static inline void destroy_handle(queue_handle_t) noexcept
    {   }
};

template<>
struct queue_destroy<tag_obj>
{
    static inline void destroy_handle(queue_handle_t hqueue) noexcept
    {
        if (nullptr != hqueue)
            event_base_free(hqueue);
    }
};

} // detail

template<class R>
class basic_queue;

typedef basic_queue<tag_ref> queue_ref;
typedef basic_queue<tag_obj> queue;

template<class R>
class basic_queue
{
public:
    typedef queue_handle_t handle_t;
    constexpr static bool is_ref = R::is_ref;

private:
    handle_t hqueue_{ nullptr };

    handle_t assert_handle() const noexcept
    {
        auto hqueue = handle();
        assert(hqueue);
        return hqueue;
    }

public:
    basic_queue() = default;

    ~basic_queue() noexcept
    {
        detail::queue_destroy<R>::destroy_handle(hqueue_);
    }

    basic_queue(basic_queue&& that) noexcept
    {
        std::swap(hqueue_, that.hqueue_);
    }

    basic_queue(handle_t hqueue) noexcept
        : hqueue_(hqueue)
    {
        assert(hqueue);
        static_assert(is_ref, "queue_ref only");
    }

    basic_queue(const queue& other) noexcept
        : basic_queue(other.handle())
    {   }

    basic_queue(const queue_ref& other) noexcept
        : basic_queue(other.handle())
    {   }

    basic_queue& operator=(basic_queue&& that) noexcept
    {
        std::swap(hqueue_, that.hqueue_);
        return *this;
    }

    void assign(handle_t hqueue) noexcept
    {
        assert(hqueue);
        hqueue_ = hqueue;
    }

    queue_ref& operator=(handle_t hqueue) noexcept
    {
        assign(hqueue);
        return *this;
    }

    queue_ref& operator=(const queue& other) noexcept
    {
        assign(other.handle());
        return *this;
    }

    queue_ref& operator=(const queue_ref& other) noexcept
    {
        assign(other.handle());
        return *this;
    }

    handle_t handle() const noexcept
    {
        return hqueue_;
    }

    operator handle_t() const noexcept
    {
        return handle();
    }

    bool empty() const noexcept
    {
        return nullptr == handle();
    }

    void create()
    {
        static_assert(!is_ref, "no queue_ref");

        assert(empty());

        auto hqueue = event_base_new();
        if (!hqueue)
            throw std::runtime_error("event_base_new");

        hqueue_ = hqueue;
    }

    void create(const config& conf)
    {
        static_assert(!is_ref, "no queue_ref");

        assert(empty());

        auto hqueue = event_base_new_with_config(conf);
        if (!hqueue)
            throw std::runtime_error("event_base_new");

        hqueue_ = hqueue;
    }

    void destroy() noexcept
    {
        detail::queue_destroy<R>::destroy_handle(hqueue_);
        hqueue_ = nullptr;
    }

    template<class T>
    struct proxy
    {
        static inline void make_once(basic_queue& queue,
        evutil_socket_t fd, short ef, timeval tv, std::reference_wrapper<T> fn)
        {
            queue.once(fd, ef, tv, call, &fn.get());
        }

        template<class F>
        static inline void make_once(basic_queue& queue,
            evutil_socket_t fd, short ef, timeval tv, F&& f)
        {
            queue.once(fd, ef, tv, callfun, new T(std::forward<F>(f)));
        }

    private:

        static inline void call(evutil_socket_t sock, short ef, void *arg)
        {
            assert(arg);
            (*static_cast<T*>(arg))(sock, ef);
        }

        static inline void callfun(evutil_socket_t sock, short ef, void* arg)
        {
            assert(arg);
            auto fn = static_cast<T*>(arg);

            try
            {
                (*fn)(ef, sock);
            }
            catch (...)
            {   }

            delete fn;
        }
    };

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

    operator timeval() const
    {
        return gettimeofday_cached();
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
    void once(evutil_socket_t fd, short ef, timeval tv,
              std::reference_wrapper<F> fn)
    {
        proxy<F>::make_once(*this, fd, ef, tv, std::move(fn));
    }

    template<class F>
    void once(evutil_socket_t fd, short ef, timeval tv, F fun)
    {
        proxy<queue_fn>::make_once(*this, fd, ef, tv, std::move(fun));
    }

    template<class Rep, class Period, class F>
    void once(evutil_socket_t fd, short ef,
        std::chrono::duration<Rep, Period> timeout, F fun)
    {
        once(fd, ef, make_timeval(timeout), std::move(fun));
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
    void once(short ef, timeval tv, F fun)
    {
        once(-1, ef|EV_TIMEOUT, tv, std::move(fun));
    }

    template<class Rep, class Period, class F>
    void once(short ef, std::chrono::duration<Rep, Period> timeout, F fun)
    {
        once(-1, ef|EV_TIMEOUT, make_timeval(timeout), std::move(fun));
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
    void once(timeval tv, F fun)
    {
        once(-1, EV_TIMEOUT, tv, std::move(fun));
    }


    template<class Rep, class Period, class F>
    void once(std::chrono::duration<Rep, Period> timeout, F fun)
    {
        once(-1, EV_TIMEOUT, make_timeval(timeout), std::move(fun));
    }

// ------
    void once(event_callback_fn fn, void *arg)
    {
        once(-1, EV_TIMEOUT, timeval{0, 0}, fn, arg);
    }

    template<class F>
    void once(F fun)
    {
        once(-1, EV_TIMEOUT, timeval{0, 0},  std::move(fun));
    }
};

} // namespace btpro
