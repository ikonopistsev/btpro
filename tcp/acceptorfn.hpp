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
    typedef void (T::*on_throw_t)(std::exception_ptr);
    typedef acceptorfn<T> this_type;
    typedef listener::handle_t handle_t;

private:
    T& self_;
    callback_fn fn_{ nullptr };
    on_throw_t on_throw_{ nullptr };
    listener listener_{};

    template<class A>
    struct proxy
    {
        static inline void evcb(handle_t, evutil_socket_t sock,
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
            if (on_throw_)
                (self_.*on_throw_)(ep);
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

    acceptorfn& listen(be::queue& queue,
        unsigned int flags, const ip::addr& sa, int backlog)
    {
        listener_.listen(queue, flags, backlog,
            sa, proxy<this_type>::evcb, this);

        return *this;
    }

    acceptorfn& listen(be::queue& queue,
        unsigned int flags, const ip::addr& sa)
    {
        return listen(queue, flags, sa, -1);
    }

    acceptorfn& listen(be::queue& queue, const ip::addr& sa)
    {
        return listen(queue, 0, sa, -1);
    }

    acceptorfn& set(callback_fn fn) const noexcept
    {
        fn_ = fn;
    }

    acceptorfn& set(on_throw_t fn) const noexcept
    {
        on_throw_ = fn;
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
