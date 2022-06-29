#pragma once

#include "btpro/tcp/listener.hpp"
#include "btpro/socket.hpp"

namespace btpro {
namespace tcp {

template<class T>
class acceptorfn
{
public:
    typedef void (T::*callback_fn)(socket, ip::addr);
    typedef void (T::*throw_fn)(std::exception_ptr);
    typedef acceptorfn<T> this_type;

private:
    T& self_;
    callback_fn fn_{ nullptr };
    throw_fn throw_fn_{ nullptr };
    listener listener_{};

    template<class A>
    struct proxy
    {
        static inline void evcb(connlistener_handle_t, evutil_socket_t sock,
            sockaddr *sa, int salen, void *self) noexcept
        {
            assert(self);
            static_cast<A*>(self)->do_accept(sock, sa, salen);
        }
    };

    void on_throw(std::exception_ptr ep) noexcept
    {
        try
        {
            if (throw_fn_)
                (self_.*throw_fn_)(ep);
        }
        catch (...)
        {   }
    }

    void do_accept(evutil_socket_t fd, sockaddr *sa, int salen) noexcept
    {
        try
        {
            (self_.*fn_)(be::socket(fd),
                ip::addr::create(sa, static_cast<socklen_t>(salen)));
        }
        catch (...)
        {
            on_throw(std::current_exception());
        }
    }

public:
    acceptorfn(acceptorfn&) = delete;
    acceptorfn& operator=(acceptorfn&) = delete;

    acceptorfn(T& self, callback_fn fn) noexcept
        : self_(self)
        , fn_(fn)
    {   
        assert(fn);
    }

    acceptorfn(T& self, callback_fn c, throw_fn t) noexcept
        : self_(self)
        , fn_(c)
        , throw_fn_(t)
    {
        assert(c);
    }

    void close()
    {
        listener_.close();
    }

    acceptorfn& listen(queue_handle_t queue,
        unsigned int flags, const ip::addr& sa, int backlog)
    {
        assert(queue);
        listener_.listen(queue, flags, backlog,
            sa, proxy<this_type>::evcb, this);

        return *this;
    }

    acceptorfn& listen(queue_handle_t queue,
        unsigned int flags, const ip::addr& sa)
    {
        assert(queue);
        return listen(queue, flags, sa, -1);
    }

    acceptorfn& listen(queue_handle_t queue, const ip::addr& sa)
    {
        assert(queue);
        return listen(queue, 0, sa, -1);
    }

    acceptorfn& set(callback_fn fn) noexcept
    {
        assert(fn);
        fn_ = fn;
        return *this;
    }

    acceptorfn& set(throw_fn fn) noexcept
    {
        throw_fn_ = fn;
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
