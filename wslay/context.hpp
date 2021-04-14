#pragma once

#include "btpro/ssl/rand.hpp"
#include "wslay/wslay.h"

namespace btpro {
namespace wslay {

template<class T>
class handler
{
public:
    typedef ssize_t (T::*receive_fn_type)(
        uint8_t *data, size_t len);

    typedef void (T::*message_fn_type)(
        const wslay_event_on_msg_recv_arg* msg);

private:
    T& consumer_;
    receive_fn_type receive_fn_{nullptr};
    message_fn_type message_fn_{nullptr};

public:
    handler(T& consumer) noexcept
        : consumer_(consumer)
    {   }

    handler(T& consumer,
            receive_fn_type receive_fn, message_fn_type message_fn) noexcept
        : consumer_(consumer)
        , receive_fn_(receive_fn)
        , message_fn_(message_fn)
    {
        assert(receive_fn);
        assert(message_fn);
    }

    void set(receive_fn_type receive_fn) noexcept
    {
        assert(receive_fn);
        receive_fn_ = receive_fn;
    }

    void set(message_fn_type message_fn) noexcept
    {
        assert(message_fn);
        message_fn_ = message_fn;
    }

    // интерфейсные методы handler'a
    ssize_t wslay_receive(uint8_t *data, size_t len)
    {
        assert(receive_fn_);
        return (consumer_.*receive_fn_)(data, len);
    }

    void wslay_message(const wslay_event_on_msg_recv_arg* msg)
    {
        assert(receive_fn_);
        return (consumer_.*message_fn_)(msg);
    }
};

using handle_t = wslay_event_context_ptr;

template<class H>
class context
{
    H& handler_;
    handle_t wslay_{nullptr};

    template<class F>
    struct proxy
    {
        static ssize_t recvcb(wslay_event_context_ptr,
            uint8_t *data, size_t len, int, void *self)
        {
            assert(self);
            return static_cast<F*>(self)->call_receive(data, len);
        }

        static int genmask(wslay_event_context_ptr ctx,
            uint8_t *buf, size_t len, void*)
        {
            ssl::rand rnd;
            auto rc = rnd(buf, len);
            if (rc == -1)
            {
                wslay_event_set_error(ctx, WSLAY_ERR_CALLBACK_FAILURE);
                return rc;
            }

            return 0;
        }

        static void msgcb(wslay_event_context_ptr,
             const wslay_event_on_msg_recv_arg *msg, void *self)
        {
            assert(self);
            static_cast<F*>(self)->call_message(msg);
        }
    };

    handle_t assert_handle() const noexcept
    {
        auto wslay = handle();
        assert(wslay);
        return wslay;
    }

    ssize_t call_receive(uint8_t *data, size_t len)
    {
        return handler_.wslay_receive(data, len);
    }

    void call_message(const wslay_event_on_msg_recv_arg* msg)
    {
        return handler_.wslay_message(msg);
    }

public:
    context(H& handler) noexcept
        : handler_(handler)
    {   }

    ~context() noexcept
    {   }

    handle_t handle() const noexcept
    {
        return wslay_;
    }

    operator handle_t() const noexcept
    {
        return handle();
    }

    bool empty() const noexcept
    {
        return nullptr == handle();
    }

    void client()
    {
        wslay_event_callbacks wslaycb = {
            proxy<context>::recvcb,
            nullptr,
            proxy<context>::genmask,
            nullptr, nullptr, nullptr,
            proxy<context>::msgcb
        };

        if (wslay_event_context_client_init(&wslay_, &wslaycb, this) != 0)
            throw std::runtime_error("wslay_event_context_client_init");
    }

    void server()
    {
        wslay_event_callbacks wslaycb = {
            proxy<context>::recvcb,
            nullptr,
            proxy<context>::genmask,
            nullptr, nullptr, nullptr,
            proxy<context>::msgcb
        };

        if (wslay_event_context_server_init(&wslay_, &wslaycb, this) != 0)
            throw std::runtime_error("wslay_event_context_server_init");
    }

    void set_buffering(int val)
    {
        // обратные называния опций только деграданты делают
        wslay_event_config_set_no_buffering(assert_handle(), !val);
    }

    void set_max_recv_msg_length(uint64_t val)
    {
        wslay_event_config_set_max_recv_msg_length(assert_handle(), val);
    }

    void destroy() noexcept
    {
        if (wslay_)
        {
            wslay_event_context_free(wslay_);
            wslay_ = nullptr;
        }
    }

    void queue_msg(const wslay_event_msg& msg)
    {
        if (wslay_event_queue_msg(assert_handle(), &msg) != 0)
            throw std::runtime_error("wslay_event_queue_msg");
    }

    void queue_text(std::string_view text)
    {
        wslay_event_msg msg = {
            WSLAY_TEXT_FRAME,
            reinterpret_cast<const uint8_t*>(text.data()), text.size()
        };

        queue_msg(msg);
    }

    void queue_binary(const void* data, size_t len)
    {
        wslay_event_msg msg = {
            WSLAY_BINARY_FRAME,
            reinterpret_cast<const uint8_t*>(data), len
        };

        queue_msg(msg);
    }

    void queue_close(wslay_status_code status, std::string_view text)
    {
        wslay_event_queue_close(assert_handle(), status,
            reinterpret_cast<const uint8_t*>(text.data()), text.size());
    }

    int want_write() const noexcept
    {
        return wslay_event_want_write(assert_handle());
    }

    int want_read() const noexcept
    {
        return wslay_event_want_read(assert_handle());
    }

    ssize_t write(uint8_t *buf, size_t buflen) noexcept
    {
        return wslay_event_write(assert_handle(), buf, buflen);
    }

    void receive()
    {
        if (wslay_event_recv(assert_handle()))
            throw std::runtime_error("wslay_event_recv");
    }

    using this_type = context<H>;

    class holder
    {
        this_type& context_;

    public:
        holder(this_type& handler) noexcept
            : context_(handler)
        {   }

        ~holder() noexcept
        {
            context_.destroy();
        }
    };
};

} // namespace wslay
} // namespace btpro
