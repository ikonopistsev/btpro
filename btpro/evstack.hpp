#pragma once

#include "btpro/queue.hpp"
#include "btpro/socket.hpp"
#include "event2/event_struct.h"

namespace btpro {

// стек эвент
class evstack
{
private:
    static inline event create_event() noexcept
    {
        event ev;
        std::memset(&ev, 0, sizeof(ev));
        return ev;
    }

    static inline const event& empty_event() noexcept
    {
        static const event ev = create_event();
        return ev;
    }

    event event_{ empty_event() };

public:
    evstack() = default;

    // создание объекта
    void create(queue::handle_t queue, evutil_socket_t fd,
        event_flag_t ef, event_callback_fn fn, void *arg)
    {
        // без метода не получится
        assert(queue && fn && empty());

        auto res = event_assign(&event_, queue, fd, ef, fn, arg);
        if (code::fail == res)
            throw std::runtime_error("event_assign");
    }

    void create(queue& queue, be::socket sock,
        event_flag_t ef, event_callback_fn fn, void *arg)
    {
        create(queue.handle(), sock.fd(), ef, fn, arg);
    }

    void create(queue& queue, event_flag_t ef, event_callback_fn fn, void *arg)
    {
        create(queue.handle(), -1, ef, fn, arg);
    }

    void destroy() noexcept
    {
        if (!empty())
        {
            event_del(&event_);
            event_ = empty_event();
        }
    }

    event_handle_t handle() const noexcept
    {
        return const_cast<event_handle_t>(&event_);
    }

    bool empty() const noexcept
    {
        return !event_initialized(&event_);
    }
};

} // namespace btpro
