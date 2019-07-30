#pragma once

#include "btpro/evcore.hpp"
#include "btpro/socket.hpp"

#ifndef BTPRO_ACTIVATE
#define EV_ACTIVATE 0x1000
#define BTPRO_ACTIVATE EV_ACTIVATE
#endif // BTPRO_TIMEOUT_ACTIVATE

#ifndef BTPRO_ON_FREE
#define EV_ON_FREE 0x2000
#define BTPRO_ON_FREE EV_ON_FREE
#endif // BTPRO_CALL_ON_FREE

#ifndef BTPRO_EV_REMOVE
#define EV_REMOVE 0x100000
#define BTPRO_EV_REMOVE EV_REMOVE
#endif // BTPRO_EV_END


namespace btpro {
namespace detail {

} // namespace detail

// базовый класс эвента
// присутствует дополнительная логика
// EV_ON_FREE - вызов события при удалении эвента
// EV_ACTIVATE - повторная активация эвента
// EV_REMOVE - удаление из очереди из калбека

template<class T, class H>
class ev
{
public:
    typedef T event_core_t;
    typedef H event_handler_t;
    // калбек без сокета
    typedef int (*nosock_fn)(evcore_flag_t, void*);
    // кабек с сокетом
    typedef int (*socket_fn)(evcore_flag_t, be::socket, void*);
    // калбек с дескриптором
    typedef int (*fd_fn)(evcore_flag_t, evutil_socket_t, void*);

protected:
    // базовое событие
    event_core_t ev_{};
    event_handler_t handler_{};
    // сохраненный таймаут
    timeval timeout_{};

    // какой-то из клиентских калбеков
    union {
        nosock_fn nosock_fn_{};
        socket_fn socket_fn_;
        fd_fn fd_fn_;
    };

    // аргумент калбека клинета
    void *arg_{};

    // состояние, для обработки EV_ACTIVATE и EV_REMOVE
    long state_{EV_REMOVE};

    enum {
        mask = 0xffffu
    };

    evflag_t  (short ef)
    {
        // проверяем была ли принудительная активация
        if (state_ & EV_ACTIVATE)
        {
            // если была убираем лишние флаги
            ef = static_cast<short>(state_ & mask);
            // сбрасываем состояние
            state_ = 0;
            // перезаводим таймаут повторно
            ev_.add(timeout_);
        }

        return ef;
    }

    void call(evutil_socket_t fd, short ef)
    {
        if (!empty())
        {
            auto res = fn_(next_events(ef), be::socket(fd), arg_);
            // EV_ON_FREE вызывается только либой при удалении
            assert(!(res & EV_ON_FREE));
            // убираем on_free
            res &= ~EV_ON_FREE;
            dispatch(res);
        }
    }

    void call_fd(evutil_socket_t fd, short ef)
    {
        if (!empty())
        {
            auto res = fd_fn_(next_events(ef), fd, arg_);
            // EV_ON_FREE вызывается только либой при удалении
            assert(!(res & EV_ON_FREE));
            // убираем on_free
            res &= ~EV_ON_FREE;
            dispatch(res);
        }
    }

    void call_nosock(short ef)
    {
        if (!empty())
        {
            auto res = nosock_fn_(next_events(ef), arg_);
            // EV_ON_FREE вызывается только либой при удалении
            assert(!(res & EV_ON_FREE));
            // убираем on_free
            res &= ~EV_ON_FREE;
            dispatch(res);
        }
    }

    virtual void dispatch(int res) noexcept
    {
        if (res && !empty())
        {
            if (res & EV_REMOVE)
                remove();
            else
                add_next(res);
        }
    }

    template<class F>
    struct proxy
    {
        static inline void evcb(evutil_socket_t fd, short ef, void* self)
        {
            assert(self);
            static_cast<F*>(self)->call(fd, ef);
        }

        static inline void evcb_fd(evutil_socket_t fd, short ef, void* self)
        {
            assert(self);
            static_cast<F*>(self)->call_fd(fd, ef);
        }

        static inline void evcb_nosock(evutil_socket_t, short ef, void* self)
        {
            assert(self);
            static_cast<F*>(self)->call_nosock(ef);
        }

        static inline int fncb(short ef, evutil_socket_t fd, void* arg)
        {
            assert(arg);
            auto fn = static_cast<F*>(arg);
            auto res = (*fn)(ef, be::socket(fd));
            // !!! EV_ON_FREE вызывается только при удалении
            assert(!(res & EV_ON_FREE));
            // убираем on_free
            res &= ~EV_ON_FREE;
            return res;
        }
    };

    void destroy()
    {
        if (!empty())
        {
            // вызываем обработчик если эвенет не отрабатывал
            if ((ev_.events() & EV_ON_FREE) && ev_.pending(EV_TIMEOUT))
            {
                // вызываем обработчик
                ev_.exec(EV_ON_FREE);
            }

            remove();
        }
    }

    void remove()
    {
        ev_.remove();
        state_ |= EV_REMOVE;
    }

    void add_event()
    {
        ev_.add();
    }

    void add_event(timeval tv)
    {
        timeout_ = tv;
        ev_.add(tv);
   }

    void active_event(int res)
    {
        ev_.active(res);
    }

    void add_next(int res)
    {
        state_ = static_cast<long>((res|EV_ACTIVATE) & mask);
        ev_.add(timeval{0, 0});
    }

    void remove_current(queue::handle_t handle)
    {
        auto current = ev_.base();
        if (current && (current != handle))
            ev_.remove();
    }

public:
    ev() = default;

    ev(const ev&) = delete;
    ev& operator=(const ev&) = delete;
    ev(ev&&) = delete;
    ev& operator=(ev&&) = delete;

    virtual ~ev()
    {
        destroy();
    }

    ev(queue& queue, be::socket sock,
        evflag_t ef, callback_fn fn, void *arg)
    {
        assign(queue, sock, ef, fn, arg);
    }

    ev(queue& queue, be::socket sock,
        evflag_t ef, socket_fn fn, void *arg)
    {
        assign(queue, sock, ef, fn, arg);
    }

    ev(queue& queue, be::socket sock,
        evflag_t ef, nosock_fn fn, void *arg)
    {
        assign(queue, sock, ef, fn, arg);
    }

    template<class F>
    ev(queue& queue, be::socket sock, evflag_t ef, F& fn)
    {
        assign(queue, sock, ef, fn);
    }

    ev& assign(queue::handle_t handle, be::socket sock,
        evflag_t ef, callback_fn fn, void *arg)
    {
        assert(fn);
        assert(handle);

        remove_current(handle);
        ev_.assign(handle, sock.fd(), ef, proxy<ev>::evcb, this);

        fn_ = fn;
        arg_ = arg;

        state_ &= ~EV_REMOVE;

        return *this;
    }

    ev& assign(queue::handle_t handle, be::socket sock,
        evflag_t ef, socket_fn fn, void *arg)
    {
        assert(fn);
        assert(handle);

        remove_current(handle);

        ev_.assign(handle, sock.fd(), ef, proxy<ev>::evcb_fd, this);
        fd_fn_ = fn;
        arg_ = arg;

        state_ &= ~EV_REMOVE;

        return *this;
    }

    ev& assign(queue::handle_t handle, be::socket sock,
        evflag_t ef, nosock_fn fn, void *arg)
    {
        assert(fn);
        assert(handle);

        remove_current(handle);
        ev_.assign(handle, sock.fd(), ef, proxy<ev>::evcb_nosock, this);

        nosock_fn_ = fn;
        arg_ = arg;

        state_ &= ~EV_REMOVE;

        return *this;
    }

    ev& assign(queue& queue, be::socket sock,
        evflag_t ef, callback_fn fn, void *arg)
    {
        return assign(queue.handle(), sock, ef, fn, arg);
    }

    ev& assign(queue& queue, be::socket sock,
        evflag_t ef, socket_fn fn, void *arg)
    {
        return assign(queue.handle(), sock, ef, fn, arg);
    }

    ev& assign(queue& queue, be::socket sock,
        evflag_t ef, nosock_fn fn, void *arg)
    {
        return assign(queue.handle(), sock, ef, fn, arg);
    }

    template<class F>
    ev& assign(queue& queue, be::socket sock, evflag_t ef, F& fn)
    {
        return assign(queue, sock, ef, proxy<F>::fncb, &fn);
    }

    bool empty() const noexcept
    {
        return (state_ & EV_REMOVE) > 0;
    }

    void clear()
    {
        if (!empty())
            remove();
    }

    void add()
    {
        add_event();
    }

    void add(timeval tv)
    {
        add_event(tv);
    }

    template<class Rep, class Period>
    void add(std::chrono::duration<Rep, Period> timeout)
    {
        add(make_timeval(timeout));
    }

    void active(evflag_t res)
    {
        active_event(res);
    }

    void next(evflag_t res)
    {
        add_next(res);
    }

    evflag_t events() const noexcept
    {
        return ev_.events();
    }

    be::socket fd() const noexcept
    {
        return be::socket(ev_.fd());
    }

    queue::handle_t base() const noexcept
    {
        return ev_.base();
    }

    callback_fn callback() const noexcept
    {
        return fn_;
    }

    void* callback_arg() const noexcept
    {
        return arg_;
    }
};

} // namespace btpro
