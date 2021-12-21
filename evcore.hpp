#pragma once

#include "btpro/evheap.hpp"
#include "btpro/evstack.hpp"
#include "btpro/queue.hpp"

namespace btpro {

template<class T>
class evcore;

typedef evcore<evheap> evh;
typedef evcore<evstack> evs;

template<class T>
class evcore
{
public:
    typedef T value_type;

private:
    template<class F>
    struct proxy
    {
        static inline void evcb(evutil_socket_t fd, short ef, void* arg)
        {
            assert(arg);
            (*static_cast<F*>(arg))(fd, ef);
        }

        static inline void evsfncb(evutil_socket_t fd, short ef, void* arg)
        {
            assert(arg);
            static_cast<evsfn<F>*>(arg)->call(fd, ef);
        }

        static inline void evtfncb(evutil_socket_t, short, void* arg)
        {
            assert(arg);
            static_cast<evtfn<F>*>(arg)->call();
        }
    };

    event_handle_t assert_handle() const noexcept
    {
        assert(!empty());
        return handle();
    }

    value_type event_{};

public:
    evcore() = default;

    evcore(queue_handle_t queue, evutil_socket_t fd, event_flag_t ef,
        event_callback_fn fn, void *arg)
    {
        create(queue, fd, ef, fn, arg);
    }

    template<class P>
    evcore(queue_handle_t queue, event_flag_t ef, evtfn<P>& ev)
    {
        create(queue, -1, ef, ev);
    }

    template<class P>
    evcore(queue_handle_t queue, evutil_socket_t fd,
                event_flag_t ef, evsfn<P>& ev)
    {
        create(queue, fd, ef, ev);
    }

    ~evcore() noexcept
    {
        event_.deallocate();
    }

    void create(queue_handle_t queue, evutil_socket_t fd, event_flag_t ef,
        event_callback_fn fn, void *arg)
    {
        event_.create(queue, fd, ef, fn, arg);
    }

    void create(queue_handle_t queue, be::socket sock, event_flag_t ef,
        event_callback_fn fn, void *arg)
    {
        create(queue, sock.fd(), ef, fn, arg);
    }

    template<class P>
    void create(queue_handle_t queue, evutil_socket_t fd,
                event_flag_t ef, evsfn<P>& ev)
    {
        create(queue, fd, ef, proxy<evsfn<P>>::evsfncb, &ev);
    }

    template<class P>
    void create(queue_handle_t queue, evutil_socket_t fd,
                event_flag_t ef, evtfn<P>& ev)
    {
        create(queue, fd, ef, proxy<evtfn<P>>::evtfncb, &ev);
    }

    template<class F>
    void create_timeout(queue_handle_t queue, F& ev)
    {
        create(queue, -1, EV_TIMEOUT, ev);
    }

    template<class F>
    void create_interval(queue_handle_t queue, F& ev)
    {
        create(queue, -1, EV_TIMEOUT|EV_PERSIST, ev);
    }

    void create(queue_handle_t queue, event_flag_t ef,
                event_callback_fn fn, void *arg)
    {
        create(queue, -1, ef, fn, arg);
    }

    // конструктор для лямбды
    // можно использовать с сигналами
    template<class F>
    void create(queue_handle_t queue, evutil_socket_t fd,
                event_flag_t ef, F& fn)
    {
        event_.create(queue, fd, ef, proxy<F>::evcb, &fn);
    }

    template<class F>
    void create(queue_handle_t queue, be::socket sock,
                event_flag_t ef, F& fn)
    {
        event_.create(queue, sock, ef, proxy<F>::evcb, &fn);
    }

    // конструктор для лямбды
    template<class F>
    void create(queue_handle_t queue, event_flag_t ef, F& fn)
    {
        event_.create(queue, ef, proxy<F>::evcb, &fn);
    }


    template<class F>
    void create_then_add(queue_handle_t queue, evutil_socket_t fd,
                         event_flag_t ef, timeval* tv, F& fn)
    {
        create(queue, fd, ef, proxy<F>::evcb, &fn);
        add(tv);
    }

    template<class F>
    void create_then_add(queue_handle_t queue, be::socket sock,
                         event_flag_t ef, timeval* tv, F& fn)
    {
        create(queue, sock, ef, proxy<F>::evcb, &fn);
        add(tv);
    }

    template<class F, class Rep, class Period>
    void create_then_add(queue_handle_t queue, evutil_socket_t fd,
        event_flag_t ef, std::chrono::duration<Rep, Period> timeout, F& fn)
    {
        create(queue, fd, ef, proxy<F>::evcb, &fn);
        add(timeout);
    }

    template<class F, class Rep, class Period>
    void create_then_add(queue_handle_t queue, be::socket sock,
        event_flag_t ef, std::chrono::duration<Rep, Period> timeout, F& fn)
    {
        create(queue, sock, ef, proxy<F>::evcb, &fn);
        add(timeout);
    }

    // !!! это не выполнить на следующем цикле очереди
    // это добавить без таймаута
    // допустим вечное ожидание EV_READ или сигнала
    // конструктор для лямбды
    template<class F>
    void create_then_add(queue_handle_t queue, event_flag_t ef,
                         timeval tv, F& fn)
    {
        create(queue, ef, proxy<F>::evcb, &fn);
        add(tv);
    }

    // конструктор для лямбды
    template<class F>
    void create_then_add(queue_handle_t queue, event_flag_t ef, F& fn)
    {
        create_then_add(queue, ef, nullptr, fn);
    }

    template<class F, class Rep, class Period>
    void create_then_add(queue_handle_t queue, event_flag_t ef,
                         std::chrono::duration<Rep, Period> timeout, F& fn)
    {
        create_then_add(queue, ef, make_timeval(timeout), fn);
    }

    bool empty() const noexcept
    {
        return event_.empty();
    }

    event_handle_t handle() const noexcept
    {
        return event_.handle();
    }

    operator event_handle_t() const noexcept
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

    // Warning: This function is only useful for distinguishing a a zeroed-out
    //   piece of memory from an initialized event, it can easily be confused by
    //   uninitialized memory.  Thus, it should ONLY be used to distinguish an
    //   initialized event from zero.
    bool initialized() const noexcept
    {
        return !event_.empty();
    }

    //    Checks if a specific event is pending or scheduled
    //    @param tv if this field is not NULL, and the event has a timeout,
    //           this field is set to hold the time at which the timeout will
    //       expire.
    bool pending(timeval& tv, event_flag_t events) const noexcept
    {
        return event_pending(assert_handle(), events, &tv) != 0;
    }

    // Checks if a specific event is pending or scheduled
    bool pending(event_flag_t events) const noexcept
    {
        return event_pending(assert_handle(), events, nullptr) != 0;
    }

    event_flag_t events() const noexcept
    {
        return event_get_events(assert_handle());
    }

    evutil_socket_t fd() const noexcept
    {
        return event_get_fd(assert_handle());
    }

    // хэндл очереди
    queue_handle_t queue_handle() const noexcept
    {
        return event_get_base(assert_handle());
    }

    operator queue_handle_t() const noexcept
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
    void exec(event_flag_t ef)
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
