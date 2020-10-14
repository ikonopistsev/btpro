#pragma once

#include "btpro/tcp/tcp.hpp"
#include "btpro/sock_addr.hpp"
#include "btpro/socket.hpp"

#include "event2/listener.h"

namespace btpro {
namespace tcp {

typedef evconnlistener* connlistener_handle_t;

class listener
{
public:
    typedef connlistener_handle_t handle_t;

private:
    std::unique_ptr<evconnlistener, decltype(&evconnlistener_free)>
        handle_{ nullptr, evconnlistener_free };

    handle_t assert_handle() const noexcept
    {
        auto handle = handle_.get();
        assert(handle);
        return handle;
    }

    constexpr static auto lev_opt = unsigned{
        LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE
    };

    static inline handle_t create(int backlog, unsigned int flags,
        queue_handle_t queue, const sockaddr *sa, ev_socklen_t salen,
        evconnlistener_cb cb, void *arg)
    {
        assert(queue && sa && (salen > 0));

        auto result = evconnlistener_new_bind(queue, cb, arg,
            flags, backlog, sa, static_cast<int>(salen));
        if (!result)
            throw std::runtime_error("evconnlistener_new_bind");
        return result;
    }

    static inline handle_t create(int backlog, unsigned int flags,
        queue_handle_t queue, const ip::addr& addr, evconnlistener_cb cb, void *arg)
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

    void listen(queue_handle_t queue, unsigned int flags, int backlog,
        const ip::addr& sa, evconnlistener_cb cb, void *arg)
    {
        handle_.reset(create(backlog, flags|lev_opt, queue, sa, cb, arg));
    }

    void listen(queue_handle_t queue, unsigned int flags,
        const ip::addr& sa, evconnlistener_cb cb, void *arg)
    {
        listen(queue, flags, -1, sa, cb, arg);
    }

    void listen(queue_handle_t queue, const ip::addr& sa,
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
        return handle_.get();
    }

    operator handle_t() const noexcept
    {
        return handle();
    }

    void set(evconnlistener_cb cb, void *arg)
    {
        assert(cb);
        evconnlistener_set_cb(assert_handle(), cb, arg);
    }

    void clear()
    {
        evconnlistener_set_cb(assert_handle(), nullptr, nullptr);
    }

    evutil_socket_t fd() const noexcept
    {
        return evconnlistener_get_fd(assert_handle());
    }

    // хэндл очереди
    queue_handle_t queue_handle() const noexcept
    {
        return evconnlistener_get_base(assert_handle());
    }

    operator queue_handle_t() const noexcept
    {
        return queue_handle();
    }

    void close()
    {
        handle_.reset(nullptr);
    }

    void enable()
    {
        int result = evconnlistener_enable(assert_handle());
        if (result == code::fail)
            throw std::runtime_error("evconnlistener_enable");
    }

    void disable()
    {
        int result = evconnlistener_disable(assert_handle());
        if (result == code::fail)
            throw std::runtime_error("evconnlistener_disable");
    }
};

} // namespace tcp
} // namespace btpro
