#pragma once

#include "btpro/queue.hpp"
#include "btpro/buffer.hpp"
#include "btpro/socket.hpp"

#include "event2/bufferevent.h"
#include "event2/bufferevent_struct.h"

#include <functional>

namespace evnet {
namespace tcp {

class bev
{
public:
    typedef bufferevent* handle_t;

private:
    static inline void destroy(handle_t handle) noexcept
    {
        assert(handle);
        bufferevent_free(handle);
    }

    static inline void leave(handle_t) noexcept
    {   }

    std::unique_ptr<bufferevent, decltype(&leave)>
        handle_{ nullptr, leave };

    template<typename T>
    struct inner
    {
        static void sign(handle_t, short ev, void *obj) noexcept
        {
            assert(obj);
            static_cast<T*>(obj)->do_event(ev);
        }

        static void send(handle_t, void *obj) noexcept
        {
            assert(obj);
            static_cast<T*>(obj)->do_send();
        }

        static void recv(handle_t, void *obj) noexcept
        {
            assert(obj);
            static_cast<T*>(obj)->do_recv();
        }

        static void sended(const void *, size_t, void *obj) noexcept
        {
            assert(obj);
            T *sh = static_cast<T*>(obj);
            sh->call();
            delete sh;
        }
    };

    std::size_t output_length() const noexcept
    {
        return evbuffer_get_length(bufferevent_get_output(handle()));
    }

    std::size_t input_length() const noexcept
    {
        return evbuffer_get_length(bufferevent_get_input(handle()));
    }

    bool output_empty() const noexcept
    {
        return output_length() == 0;
    }

    bool input_empty() const noexcept
    {
        return input_length() == 0;
    }


    void do_event(short ev) noexcept
    {
        try
        {
            // коннект будет 0
            if (ev & BEV_EVENT_CONNECTED)
                ev = 0;

            // таймаут сбрасывает фалги
            if (ev & BEV_EVENT_TIMEOUT)
            {
                if (ev & BEV_EVENT_READING)
                {
                    if (recv_)
                        enable(EV_READ);
                }
                if (ev & BEV_EVENT_WRITING)
                {
                    if (!is_output_empty())
                        enable(EV_WRITE);
                }
            }

            handler_noexcept(event_, ev);

            if (ev & BEV_EVENT_ERROR)
                handler_noexcept(error_, socket_error(), error());
        }
        catch (const std::exception& e)
        {
            error_noexept(500, e.what());
        }
        catch (...)
        {
            error_noexept(500, "do_event");
        }
    }

    be::buffer::handle_t output_handle() const
    {
        return bufferevent_get_output(handle());
    }

    be::buffer::handle_t input_handle() const
    {
        return bufferevent_get_input(handle());
    }

    be::buffer output() const
    {
        return be::buffer(output_handle());
    }

    be::buffer input() const
    {
        return be::buffer(input_handle());
    }

    bev& check_result(const char *what, int result)
    {
        assert(what);
        assert(what[0] != '\0');
        if (result == result::fail)
            throw std::runtime_error(what);
        return *this;
    }

    template<typename T>
    void error_noexept(int code, const T& what) const noexcept
    {
        try
        {
            if (error_)
                error_(code, what);
        }
        catch (...)
        {   }
    }

    template<typename T, typename... A>
    void handler_noexcept(T&& func, A&&... arg) const noexcept
    {
        try
        {
            if (func)
                func(std::forward<A>(arg)...);
        }
        catch (const std::exception& e)
        {
            error_noexept(600, e.what());
        }
        catch (...)
        {   }
    }

public:
    bev() = default;
    bev(bev&) = delete;
    bev& operator=(bev& buffer) = delete;

    static constexpr auto bev_opt = int{
        LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE
    };

    explicit bev(queue& queue, be::socket sock, int opt);

    bev& attach(queue& queue, be::socket sock)
    {
        bufferevent_socket_new()
        return *this;
    }

    handle_t handle() const noexcept
    {
        auto result = handle_.get();
        assert(result);
        return result;
    }

    be::socket socket() const noexcept
    {
        return be::socket(bufferevent_getfd(handle()));
    }

    bev& enable(short event)
    {
        return check_result("bufferevent_enable",
            bufferevent_enable(handle(), event));
    }

    bev& disable(short event)
    {
        return check_result("bufferevent_disable",
            bufferevent_disable(handle(), event));
    }

    ev_ssize_t get_max_to_read() const noexcept
    {
        return bufferevent_get_max_to_read(handle());
    }

    ev_ssize_t get_max_to_write() const noexcept
    {
        return bufferevent_get_max_to_write(handle());
    }

    void lock() const noexcept
    {
        bufferevent_lock(handle());
    }

    void unlock() const noexcept
    {
        bufferevent_unlock(handle());
    }

    bev& send(buffer buf)
    {
        if (!buf.empty())
        {
            auto outbuf = bufferevent_get_output(handle());
            check_result("evbuffer_add_buffer",
                evbuffer_add_buffer(outbuf, buf.handle()));
            return enable(EV_WRITE);
        }
        return *this;
    }

    bev& set(queue& queue)
    {
        return check_result("bufferevent_base_set", 
            bufferevent_base_set(queue.handle(), handle()));
    }

    bev& set(short events, std::size_t lo, std::size_t hi) noexcept
    {
        bufferevent_setwatermark(handle(), events, lo, hi);
        return *this;
    }

    bev& set(short events, const timeval& tv)
    {
        const timeval *rtv = (events & EV_READ) ? &tv : nullptr;
        const timeval *wtv = (events & EV_WRITE) ? &tv : nullptr;

        check_result("bufferevent_set_timeouts",
            bufferevent_set_timeouts(handle(), rtv, wtv));

        return enable(events);
    }

    template<class Rep, class Per>
    bev& set(short events, std::chrono::duration<Rep, Per>& timeout)
    {
        return set(make_timeval(timeout));
    }
};

} // namespace tcp
} // namespace evnet
