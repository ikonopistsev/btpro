#pragma once

#include "btpro/tcp/bev.hpp"

#include <functional>

namespace btpro {
namespace tcp {

template<class T>
class bevfn
{
public:
    typedef void (T::*on_data_t)();
    typedef void (T::*on_event_t)(short);
    typedef bevfn<T> this_type;

private:
    T& self_;
    on_data_t send_fn_{};
    on_data_t recv_fn_{};
    on_event_t event_fn_{};
    on_data_t connect_fn_{};

    template<class A>
    struct proxy
    {
        static inline void evcb(bufferevent *, short what, void *self) noexcept
        {
            assert(self);
            static_cast<A*>(self)->do_evcb(what);
        }

        static inline void sendcb(bufferevent *, void *self) noexcept
        {
            assert(self);
            static_cast<A*>(self)->do_send();
        }

        static inline void recvcb(bufferevent *, void *self) noexcept
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

    bevfn(T& self, on_event_t fn) noexcept
        : self_(self)
        , event_fn_(fn)
    {
        assert(fn);
    }

    bevfn(T& self, on_event_t efn, on_data_t cfn,
          on_data_t sfn, on_data_t rfn) noexcept
        : self_(self)
        , send_fn_(sfn)
        , recv_fn_(rfn)
        , event_fn_(efn)
        , connect_fn_(cfn)
    {
        assert(efn);
        assert(cfn);
        assert(sfn);
        assert(rfn);
    }

    void on_connect(on_data_t fn) noexcept
    {
        assert(fn);
        connect_fn_ = fn;
    }

    void on_recv(on_data_t fn) noexcept
    {
        assert(fn);
        recv_fn_ = fn;
    }

    void on_send(on_data_t fn) noexcept
    {
        assert(fn);
        send_fn_ = fn;
    }

    void apply(bufferevent_handle_t hbev) noexcept
    {
        assert(hbev);
        bufferevent_setcb(hbev, &proxy<this_type>::recvcb,
            &proxy<this_type>::sendcb, &proxy<this_type>::evcb, this);
    }
};

} // namespace tcp
} // namespace evnet
