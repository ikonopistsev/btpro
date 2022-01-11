#pragma once

#include "btpro/evtype.hpp"

namespace btpro {

template<class T>
class evcore
{
public:
    using event_type = T;

private:
    event_type event_{};

    auto assert_handle() const noexcept
    {
        auto h = event_.handle();
        assert(h);
        return h;
    }

public:
    evcore() = default;

// generic
    evcore(queue_pointer queue, evutil_socket_t fd, event_flag ef,
        event_callback_fn fn, void *arg)
        : event_{queue, fd, ef, fn, arg}
    {   }

    evcore(queue_pointer queue, evutil_socket_t fd, event_flag ef,
        timeval tv, event_callback_fn fn, void *arg)
        : event_{queue, fd, ef, fn, arg}
    {   
        add(tv);
    }

// socket
    template<class F>
    evcore(queue_pointer queue, btpro::socket sock, event_flag ef, socket_fn<F>& fn)
        : evcore{queue, sock.fd(), ef, proxy<socket_fn<F>>::call, &fn}
    {   }

    template<class F>
    evcore(queue_pointer queue, btpro::socket sock, event_flag ef, timeval tv, socket_fn<F>& fn)
        : evcore{queue, sock.fd(), ef, tv, proxy<socket_fn<F>>::call, &fn}
    {   }

    template<class F, class Rep, class Period>
    evcore(queue_pointer queue, btpro::socket sock, event_flag ef, 
        std::chrono::duration<Rep, Period> timeout, socket_fn<F>& fn)
        : evcore{queue, sock, ef, make_timeval(timeout), fn}
    {   }

    evcore(queue_pointer queue, btpro::socket sock, event_flag ef, socket_fun fn)
        : evcore{queue, sock.fd(), ef, proxy<decltype(fn)>::call, 
            new socket_fun{std::move(fn)}}
    {   }

    evcore(queue_pointer queue, btpro::socket sock, 
        event_flag ef, std::reference_wrapper<socket_fun> fn)
        : evcore{queue, sock.fd(), ef, proxy<socket_fun>::call, &fn.get()}
    {   }

    template<class Rep, class Period>
    evcore(queue_pointer queue, btpro::socket sock, event_flag ef, 
        std::chrono::duration<Rep, Period> timeout, socket_fun fn)
        : evcore{queue, sock.fd(), ef, make_timeval(timeout), 
            proxy<decltype(fn)>::call, new socket_fun{std::move(fn)}}
    {   }

    template<class Rep, class Period>
    evcore(queue_pointer queue, btpro::socket sock, event_flag ef, 
        std::chrono::duration<Rep, Period> timeout, 
        std::reference_wrapper<socket_fun> fn)
        : evcore{queue, sock.fd(), ef, make_timeval(timeout), 
            proxy<decltype(fn)>::call, &fn.get()}
    {   }

// timer
    template<class F>
    evcore(queue_pointer queue, event_flag ef, timer_fn<F>& fn)
        : evcore{queue, -1, ef, proxy<timer_fn<F>>::call, &fn}
    {   }

    template<class F>
    evcore(queue_pointer queue, event_flag ef, timeval tv, timer_fn<F>& fn)
        : evcore{queue, -1, ef, tv, proxy<timer_fn<F>>::call, &fn}
    {   }

    template<class F, class Rep, class Period>
    evcore(queue_pointer queue, event_flag ef, 
        std::chrono::duration<Rep, Period> timeout, timer_fn<F>& fn)
        : evcore{queue, ef, make_timeval(timeout), fn}
    {   }

    evcore(queue_pointer queue, event_flag ef, timer_fun fn)
        : evcore{queue, -1, ef, proxy<decltype(fn)>::call, 
            new timer_fun{std::move(fn)}}
    {   }

    evcore(queue_pointer queue, event_flag ef, std::reference_wrapper<timer_fun> fn)
        : evcore{queue, -1, ef, proxy<decltype(fn)>::call, &fn.get()}
    {   }

    template<class Rep, class Period>
    evcore(queue_pointer queue, event_flag ef, 
        std::chrono::duration<Rep, Period> timeout, timer_fun fn)
        : evcore{queue, -1, ef, make_timeval(timeout), 
            proxy<decltype(fn)>::call, new timer_fun{std::move(fn)}}
    {   }

    template<class Rep, class Period>
    evcore(queue_pointer queue, event_flag ef, 
        std::chrono::duration<Rep, Period> timeout, 
        std::reference_wrapper<timer_fun> fn)
        : evcore{queue, -1, ef, make_timeval(timeout), 
            proxy<decltype(fn)>::call, &fn.get()}
    {   }

// generic
    void create(queue_pointer queue, evutil_socket_t fd, event_flag ef,
        event_callback_fn fn, void *arg)
    {
        event_.create(queue, fd, ef, fn, arg);
    }
    
// socket
    template<class F>
    void create(queue_pointer queue, btpro::socket sock, event_flag ef, socket_fn<F>& fn)
    {
        create(queue, sock.fd(), ef, proxy<socket_fn<F>>::call, &fn);
    }

    void create(queue_pointer queue, btpro::socket sock, event_flag ef, socket_fun fn)
    {
        create(queue, sock.fd(), ef, proxy<decltype(fn)>::call, 
            new socket_fun(std::move(fn)));
    }

    void create(queue_pointer queue, btpro::socket sock, 
        event_flag ef, std::reference_wrapper<socket_fun> fn)
    {
        create(queue, sock.fd(), ef, proxy<socket_fun>::call, &fn.get()); 
    }

    void create(queue& queue, queue_pointer from_queue, btpro::socket sock, 
        event_flag ef, socket_fun fn)
    {
        create(from_queue, sock.fd(), ef, proxy<decltype(fn)>::call, 
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

// timer
    template<class F>
    void create(queue_pointer queue, event_flag ef, timer_fn<F>& fn)
    {
        create(queue, -1, ef, proxy<timer_fn<F>>::call, &fn);
    }

    void create(queue_pointer queue, event_flag ef, timer_fun fn)
    {
        create(queue, -1, ef, proxy<decltype(fn)>::call, 
            new timer_fun(std::move(fn)));
    }

    void create(queue& queue, queue_pointer from_queue, 
        event_flag ef, timer_fun fn)
    {
        create(from_queue, -1, ef, proxy<decltype(fn)>::call, 
            new timer_fun([&, the_fn = std::move(fn)]{
                try {
                    queue.once(std::move(the_fn));
                }
                catch(...)
                {   }            
            }));            
    }

// api
    bool empty() const noexcept
    {
        return event_.empty();
    }

    auto handle() const noexcept
    {
        return event_.handle();
    }

    operator event_pointer() const noexcept
    {
        return handle();
    }

    void destroy() noexcept
    {
        event_.destroy();
    }

    // !!! это не выполнить на следующем цикле очереди
    // это добавить без таймаута
    // допустим вечное ожидание EV_READ или сигнала
    void add(timeval* tv = nullptr)
    {
        detail::check_result("event_add",
            event_add(assert_handle(), tv));
    }

    void add(timeval tv)
    {
        add(&tv);
    }

    template<class Rep, class Period>
    void add(std::chrono::duration<Rep, Period> timeout)
    {
        add(make_timeval(timeout));
    }

    void remove()
    {
        detail::check_result("event_del",
            event_del(assert_handle()));
    }

    // метод запускает эвент напрямую
    // те может привести к бесконечному вызову калбеков activate
    void active(int res) noexcept
    {
        event_active(assert_handle(), res, 0);
    }

    evcore& set_priority(int priority)
    {
        detail::check_result("event_priority_set",
            event_priority_set(assert_handle(), priority));
        return *this;
    }

    //    Checks if a specific event is pending or scheduled
    //    @param tv if this field is not NULL, and the event has a timeout,
    //           this field is set to hold the time at which the timeout will
    //       expire.
    bool pending(timeval& tv, event_flag events) const noexcept
    {
        return event_pending(assert_handle(), events, &tv) != 0;
    }

    // Checks if a specific event is pending or scheduled
    bool pending(event_flag events) const noexcept
    {
        return event_pending(assert_handle(), events, nullptr) != 0;
    }

    event_flag events() const noexcept
    {
        return event_get_events(assert_handle());
    }

    evutil_socket_t fd() const noexcept
    {
        return event_get_fd(assert_handle());
    }

    // хэндл очереди
    queue_pointer queue_handle() const noexcept
    {
        return event_get_base(assert_handle());
    }

    operator queue_pointer() const noexcept
    {
        return queue_handle();
    }

    event_callback_fn callback() const noexcept
    {
        return event_get_callback(assert_handle());
    }

    void* callback_arg() const noexcept
    {
        return event_get_callback_arg(assert_handle());
    }

    // выполнить обратный вызов напрямую
    void exec(event_flag ef)
    {
        auto handle = assert_handle();
        auto fn = event_get_callback(handle);
        // должен быть каллбек
        assert(fn);
        // вызываем обработчик
        (*fn)(event_get_fd(handle), ef, event_get_callback_arg(handle));
    }
};

} // namespace btpro
