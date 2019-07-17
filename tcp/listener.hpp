#pragma once

#include "btpro/tcp/tcp.hpp"
#include "btpro/queue.hpp"
#include "btpro/sock_addr.hpp"

#include "event2/listener.h"

namespace btpro {
namespace tcp {

class listener
{
public:
    typedef evconnlistener* handle_t;

private:
    std::unique_ptr<evconnlistener, decltype(&evconnlistener_free)>
        handle_{ nullptr, evconnlistener_free };

    static constexpr auto lev_opt = unsigned{
        LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE
    };

    static inline handle_t create(int backlog, unsigned int flags,
        be::queue& queue, const sockaddr *sa, ev_socklen_t salen,
        evconnlistener_cb cb, void *arg)
    {
        assert(sa && (salen > 0));

        auto result = evconnlistener_new_bind(queue.handle(), cb, arg,
            flags, backlog, sa, salen);
        if (!result)
            throw std::runtime_error("evconnlistener_new_bind");
        return result;
    }

    static inline handle_t create(int backlog, unsigned int flags,
        be::queue& queue, const ip::addr& addr, evconnlistener_cb cb, void *arg)
    {
        return create(backlog, flags, queue, addr.sa(), addr.size(), cb, arg);
    }

public:
    listener() = default;

    listener(listener&) = delete;
    listener& operator=(listener&) = delete;

    listener(listener&& other)
    {
        swap(other);
    }

    listener& operator=(listener&& other)
    {
        swap(other);
        return *this;
    }

    void listen(be::queue& queue, unsigned int flags, int backlog,
        const ip::addr& sa, evconnlistener_cb cb, void *arg)
    {
        handle_.reset(create(backlog, flags|lev_opt, queue, sa, cb, arg));
    }

    void listen(be::queue& queue, unsigned int flags,
        const ip::addr& sa, evconnlistener_cb cb, void *arg)
    {
        listen(queue, flags, -1, sa, cb, arg);
    }

    void listen(be::queue& queue, const ip::addr& sa,
        evconnlistener_cb cb, void *arg)
    {
        listen(queue, 0, -1, sa, cb, arg);
    }

    void swap(listener& other) noexcept
    {
        assert(this != &other);
        handle_.swap(other.handle_);
    }

    bool running() const noexcept
    {
        return handle_.get() != nullptr;
    }

    handle_t handle() const noexcept
    {
        auto handle = handle_.get();
        assert(handle);
        return handle;
    }

    void set(evconnlistener_cb cb, void *arg)
    {
        assert(cb);
        evconnlistener_set_cb(handle(), cb, arg);
    }

    void clear()
    {
        evconnlistener_set_cb(handle(), nullptr, nullptr);
    }

    evutil_socket_t socket() const noexcept
    {
        return evconnlistener_get_fd(handle());
    }

    void close()
    {
        handle_.reset(nullptr);
    }

    void enable()
    {
        int result = evconnlistener_enable(handle());
        if (result == code::fail)
            throw std::runtime_error("evconnlistener_enable");
    }

    void disable()
    {
        int result = evconnlistener_disable(handle());
        if (result == code::fail)
            throw std::runtime_error("evconnlistener_disable");
    }
};

} // namespace tcp
} // namespace btpro
