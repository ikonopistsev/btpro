#pragma once

#include "btpro/ssl/bevsock.hpp"

namespace btpro {
namespace http {

template<class T>
class connfn
{
public:
    typedef void (T::*on_data_t)();
    typedef void (T::*on_event_t)(short);
    typedef connfn<T> this_type;

private:
private:
    T& self_;
    on_event_t event_fn_{};
    on_data_t connect_fn_{};

    btpro::tcp::bev bev_;

    btpro::dns_ref dns_;
    btpro::queue_ref queue_;
    btpro::ssl::context_ref ssl_;
    btpro::ssl::bevtls bevtls_{bev_};

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
    connfn(bevfn&) = delete;
    connfn& operator=(bevfn&) = delete;

    connfn(T& self, on_event_t fn) noexcept
        : self_(self)
        , event_fn_(fn)
    {
        assert(fn);
    }

    connfn(T& self, on_event_t efn, on_data_t cfn,
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
};


} // namepsace ssl
} // namespace btpro
