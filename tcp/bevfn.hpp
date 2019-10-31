#pragma once

#include "btpro/tcp/bev.hpp"

#include <functional>

namespace btpro {
namespace tcp {

template<class T>
class bevfn
{
public:
    typedef void (T::*on_connect_t)();
    typedef void (T::*on_send_t)();
    typedef void (T::*on_recv_t)();
    typedef void (T::*on_event_t)(short);
    typedef bevfn<T> this_type;

private:
    T& self_;
    on_send_t send_fn_{};
    on_recv_t recv_fn_{};
    on_event_t event_fn_{};
    on_connect_t connect_fn_{};
    bev bev_;

    template<class A>
    struct proxy
    {
        static inline void evcb(bufferevent *, short what, void *self) noexcept
        {
            assert(self);
            static_cast<A*>(self)->do_evcb(what);
        }

        static inline void send(bufferevent *, void *self) noexcept
        {
            assert(self);
            static_cast<A*>(self)->do_send();
        }

        static inline void recv(bufferevent *, void *self) noexcept
        {
            assert(self);
            static_cast<A*>(self)->do_recv();
        }
    };

    void do_evcb(short what) noexcept
    {
        if (what == BEV_EVENT_CONNECTED)
        {
            assert(connect_fn_);
            (self_.*connect_fn_)();
        }
        else
            (self_.*event_fn_)(what);
    }

    void do_send() noexcept
    {
        assert(send_fn_);
        (self_.*send_fn_)();
    }

    void do_recv() noexcept
    {
        assert(recv_fn_);
        (self_.*recv_fn_)();
    }

public:
    bevfn(bevfn&) = delete;
    bevfn& operator=(bevfn&) = delete;

    explicit bevfn(T& self, queue& queue, int opt, on_event_t fn)
        : self_(self)
        , bev_(queue, opt)
        , event_fn_(fn)
    {
        assert(fn);
        bev_.set(proxy<this_type>::send, proxy<this_type>::recv,
                 proxy<this_type>::evcb, this);
    }

    explicit bevfn(T& self, queue& queue,
        be::socket sock, int opt, on_event_t fn)
        : self_(self)
        , bev_(queue, sock, opt)
        , event_fn_(fn)
    {
        assert(fn);
        bev_.set(proxy<this_type>::send, proxy<this_type>::recv,
                 proxy<this_type>::evcb, this);
    }

    void set(on_connect_t fn)
    {
        assert(fn);
        connect_fn_ = fn;
    }

    void connect(dns& dns, const std::string& hostname, int port)
    {
        bev_.connect(dns, hostname, port);
    }

    void connect(dns& dns, const std::string& hostname,
        int port, on_connect_t fn)
    {
        set(fn);
        connect(dns, hostname, port);
    }
};

} // namespace tcp
} // namespace evnet
