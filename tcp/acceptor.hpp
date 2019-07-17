#pragma once

#include "btpro/tcp/listener.hpp"

namespace btpro {
namespace tcp {

class acceptor
{
public:
    typedef std::function<void(evutil_socket_t, sock_addr)> handler_t;
    typedef std::function<void(int, std::string)> error_t;
    typedef std::function<void(std::exception_ptr)> throw_t;

private:
    listener& listener_;
    handler_t handler_{};
    error_t error_{};
    throw_t throw_{};

    template<class T>
    struct proxy
    {
        static void call(listener::handle_t, evutil_socket_t sock,
            sockaddr *sa, int salen, void *obj) noexcept
        {
            assert(obj);
            static_cast<T*>(obj)->dispatch(sock, sa, salen);
        }
    };

    void enable()
    {
        listener_.set(&proxy<acceptor>::call, this);
        listener_.enable();
    }

    void dispatch(evutil_socket_t sock, sockaddr *sa, int salen) noexcept
    {
        try
        {
            handler_(sock, sock_addr(sa, salen));
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
            if (throw_)
                throw_(ep);
            else
            {
                if (error_)
                    std::rethrow_exception(ep);
            }
        }
        catch (const std::exception& e)
        {
            on_error(600, e.what());
        }
        catch (...)
        {
            on_error(600);
        }
    }

    void on_error(int code, std::string text = std::string()) noexcept
    {
        try
        {
            if (error_)
                error_(code, std::move(text));
        }
        catch (...)
        {   }
    }

public:
    explicit acceptor(listener& listener) noexcept
        : listener_(listener)
    {   }

    acceptor(listener& listener, handler_t handler)
        : listener_(listener)
        , handler_(std::move(handler))
    {
        enable();
    }

    void set(handler_t handler)
    {
        if (handler)
        {
            handler_ = std::move(handler);
            enable();
        }
        else
            clear();
    }

    void clear()
    {
        listener_.disable();
        listener_.clear();
    }

    acceptor& on_throw(throw_t handler)
    {
        throw_ = std::move(handler);
        return *this;
    }

    acceptor& on_error(error_t handler)
    {
        error_ = std::move(handler);
        return *this;
    }

    const throw_t& throw_handler() const noexcept
    {
        return throw_;
    }

    const error_t& error_handler() const noexcept
    {
        return error_;
    }

    ~acceptor()
    {
        clear();
    }
};

} // namespace tcp
} // namespace btpro
