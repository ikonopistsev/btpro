#pragma once

#include "btpro/tcp/listener.hpp"
#include "btpro/socket.hpp"

namespace btpro {
namespace tcp {

class acceptor
{
public:
    typedef std::function<void(socket, ip::addr)> handler_t;
    typedef std::function<void(std::exception_ptr)> throw_t;

private:
    listener listener_{};
    handler_t handler_{};
    throw_t on_throw_{};

    template<class T>
    struct proxy
    {
        static void evcb(connlistener_handle_t, evutil_socket_t sock,
            sockaddr *sa, int salen, void *obj) noexcept
        {
            assert(obj);
            static_cast<T*>(obj)->dispatch(sock, sa, salen);
        }
    };

    void dispatch(evutil_socket_t sock, sockaddr *sa, int salen) noexcept
    {
        try
        {
            handler_(socket(sock), ip::addr::create(sa,
                static_cast<socklen_t>(salen)));
        }
        catch(...)
        {
            on_throw(std::current_exception());
        }
    }

    void on_throw(std::exception_ptr ep) noexcept
    {
        try
        {
            if (on_throw_)
                on_throw_(ep);
        }
        catch (...)
        {   }
    }

public:
    acceptor(queue_handle_t queue, unsigned int flags,
        const ip::addr& sa, int backlog, handler_t handler)
        : handler_(std::move(handler))
    {
        assert(queue && handler_);
        listener_.listen(queue, flags,
            backlog, sa, proxy<acceptor>::evcb, this);
    }

    acceptor& set(handler_t handler)
    {
        assert(handler);
        handler_ = std::move(handler);
        return *this;
    }

    acceptor& set(throw_t handler)
    {
        on_throw_ = std::move(handler);
        return *this;
    }

    void enable()
    {
        listener_.enable();
    }

    void disable()
    {
        listener_.disable();
    }
};

} // namespace tcp
} // namespace btpro
