#pragma once

#include "btpro/socket.hpp"
#include <functional>

namespace btpro {

class queue;

template<class T>
struct timer_fn
{
    using fn_type = void (T::*)();
    using self_type = T;

    fn_type fn_{};
    T& self_;

    void exec() noexcept
    {
        assert(fn_);
        try {
            (self_.*fn_)();
        } 
        catch (...)
        {   }
    }
};

template<class T>
struct signal_fn
{
    using fn_type = void (T::*)(event_flag);
    using self_type = T;

    fn_type fn_{};
    T& self_;

    void call(event_flag ef) noexcept
    {
        assert(fn_);
        try {
            (self_.*fn_)(ef);
        } 
        catch (...)
        {   }
    }
};

template<class T>
struct socket_fn
{
    using fn_type = void (T::*)(btpro::socket, event_flag);
    using self_type = T;

    fn_type fn_{};
    T& self_;

    void call(btpro::socket sock, event_flag ef)
    {
        assert(fn_);
        try {
            (self_.*fn_)(sock, ef);
        } 
        catch (...)
        {   }
    }
};

using timer_fun = std::function<void()>;
using signal_fun = std::function<void(short)>;
using socket_fun = std::function<void(btpro::socket, event_flag)>;

template<class T>
struct proxy;

template<class T>
struct proxy<timer_fn<T>>
{
    static inline void call(evutil_socket_t, 
        event_flag, void *arg) noexcept
    {
        assert(arg);
        static_cast<timer_fn<T>*>(arg)->exec();
    } 
};


template<class T>
struct proxy<signal_fn<T>>
{
    static inline void call(evutil_socket_t,
        event_flag ef, void *arg) noexcept
    {
        assert(arg);
        static_cast<signal_fn<T>*>(arg)->call(ef);
    }
};


template<class T>
struct proxy<socket_fn<T>>
{
    static inline void call(evutil_socket_t fd, event_flag ef, void *arg)
    {
        assert(arg);
        static_cast<socket_fn<T>*>(arg)->call(btpro::socket(fd), ef);
    }  
};

template<>
struct proxy<timer_fun>
{
    static inline void call(evutil_socket_t, event_flag, void* arg) noexcept
    {
        assert(arg);
        auto fn = static_cast<timer_fun*>(arg);

        try {
            (*fn)();
        }
        catch (...)
        {   }

        delete fn;
    }
};

template<>
struct proxy<std::reference_wrapper<timer_fun>>
{
    static inline void call(evutil_socket_t, event_flag, void *arg)
    {
        assert(arg);
        (*static_cast<timer_fun*>(arg))();
    }
};

template<>
struct proxy<signal_fun>
{
    static inline void call(evutil_socket_t, event_flag ef, void* arg) noexcept
    {
        assert(arg);
        auto fn = static_cast<signal_fun*>(arg);

        try {
            (*fn)(ef);
        }
        catch (...)
        {   }

        delete fn;
    }
};

template<>
struct proxy<std::reference_wrapper<signal_fun>>
{
    static inline void call(evutil_socket_t, event_flag ef, void *arg)
    {
        assert(arg);
        (*static_cast<signal_fun*>(arg))(ef);
    }
};

template<>
struct proxy<socket_fun>
{
    static inline void call(evutil_socket_t fd, event_flag ef, void* arg) noexcept
    {
        assert(arg);
        auto fn = static_cast<socket_fun*>(arg);

        try {
            (*fn)(btpro::socket(fd), ef);
        }
        catch (...)
        {   }

        delete fn;
    }
};

template<>
struct proxy<std::reference_wrapper<socket_fun>>
{
    static inline void call(evutil_socket_t fd, event_flag ef, void *arg)
    {
        assert(arg);
        (*static_cast<socket_fun*>(arg))(btpro::socket(fd), ef);
    }
};

} // namespace btpro