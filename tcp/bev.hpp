#pragma once

#include "btpro/queue.hpp"
#include "btpro/buffer.hpp"
#include "btpro/socket.hpp"

#include "event2/bufferevent.h"
//#include "event2/bufferevent_struct.h"

#include <functional>

namespace btpro {
namespace tcp {

class bev
{
public:
    typedef bufferevent* handle_t;

private:
    static inline void destroy(handle_t handle) noexcept
    {
        assert(handle);
        bufferevent_free(handle);
    }

    static inline void leave(handle_t) noexcept
    {   }

    std::unique_ptr<bufferevent, decltype(&leave)>
        handle_{ nullptr, leave };

    buffer::handle_t output() const noexcept
    {
        return bufferevent_get_output(handle());
    }

    std::size_t output_length() const noexcept
    {
        return evbuffer_get_length(output());
    }

    std::size_t input_length() const noexcept
    {
        return evbuffer_get_length(bufferevent_get_input(handle()));
    }

    bool output_empty() const noexcept
    {
        return output_length() == 0;
    }

    bool input_empty() const noexcept
    {
        return input_length() == 0;
    }

    void check_result(const char *what, int result)
    {
        assert(what);
        assert(what[0] != '\0');
        if (result == code::fail)
            throw std::runtime_error(what);
    }

    // create a new socket bufferevent over an existing socket
    static inline handle_t create_bufferevent(queue& queue, int opt)
    {
        auto handle = bufferevent_socket_new(queue.handle(), -1, opt);
        if (!handle)
            throw std::runtime_error("bufferevent_socket_new");
        return handle;
    }

    // create a new socket bufferevent over an existing socket
    static inline handle_t create_bufferevent(queue& queue,
        socket sock, int opt)
    {
        auto handle = bufferevent_socket_new(queue.handle(), sock.fd(), opt);
        if (!handle)
            throw std::runtime_error("bufferevent_socket_new");
        return handle;
    }

public:
    bev() = default;
    bev(bev&&) = default;
    bev& operator=(bev&&) = default;

    // create a new socket bufferevent
    explicit bev(queue& queue, int opt)
        : handle_(create_bufferevent(queue, opt), &destroy)
    {   }

    // create a new socket bufferevent over an existing socket
    explicit bev(queue& queue, be::socket sock, int opt)
        : handle_(create_bufferevent(queue, sock, opt), &destroy)
    {   }

    void connect(const sockaddr* sa, ev_socklen_t len)
    {
        assert(sa);
        check_result("bufferevent_socket_connect",
            bufferevent_socket_connect(handle(), sa, static_cast<int>(len)));
    }

    void connect(const ip::addr& addr)
    {
        connect(addr.sa(), addr.size());
    }

    handle_t handle() const noexcept
    {
        auto result = handle_.get();
        assert(result);
        return result;
    }

    be::socket socket() const noexcept
    {
        return be::socket(bufferevent_getfd(handle()));
    }

    void enable(short event)
    {
        check_result("bufferevent_enable",
            bufferevent_enable(handle(), event));
    }

    void disable(short event)
    {
        check_result("bufferevent_disable",
            bufferevent_disable(handle(), event));
    }

    ev_ssize_t get_max_to_read() const noexcept
    {
        return bufferevent_get_max_to_read(handle());
    }

    ev_ssize_t get_max_to_write() const noexcept
    {
        return bufferevent_get_max_to_write(handle());
    }

    void lock() const noexcept
    {
        bufferevent_lock(handle());
    }

    void unlock() const noexcept
    {
        bufferevent_unlock(handle());
    }

    void write(const void *data, std::size_t size)
    {
        check_result("bufferevent_write",
            bufferevent_write(handle(), data, size));
    }

private:
    template<class F>
    struct ref_buffer
    {
        static void clean_fn(const void*, size_t, void* extra) noexcept
        {
            assert(extra);
            
            auto fn = static_cast<F*>(extra);

            try
            {
                (*fn)();
            }
            catch (...)
            {    }

            delete fn;
        }

        static void clean_fn_all(const void* data, size_t, void* extra) noexcept
        {
            assert(data);
            assert(extra);

            free(const_cast<void*>(data));

            auto fn = static_cast<F*>(extra);

            try
            {
                (*fn)();
            }
            catch (...)
            {   }

            delete fn;
        }
    };


public:
    template<class F>
    void write_ref(const void* data, std::size_t size, F fn)
    {
        assert(data);

        // копируем каллбек
        auto fn_ptr = new std::function<void()>(std::forward<F>(fn));
        // выделяем память под буфер

        check_result("evbuffer_add_reference",
            evbuffer_add_reference(output(), data, size,
                ref_buffer<std::function<void()>>::clean_fn, fn_ptr));
    }

    template<class F>
    void write(const void* data, std::size_t size, F fn)
    {
        assert(data);

        // копируем каллбек
        auto fn_ptr = new std::function<void()>(std::forward<F>(fn));
        // выделяем память под буфер
        auto ptr = malloc(size);
        check_result("write mem allock",
            ptr != nullptr ? code::sucsess : code::fail);

        // копируем память
        memcpy(ptr, data, size);

        check_result("evbuffer_add_reference",
            evbuffer_add_reference(output(), ptr, size, 
                ref_buffer<std::function<void()>>::clean_fn_all, fn_ptr));
    }


    void write(buffer data)
    {
        check_result("bufferevent_write",
            bufferevent_write_buffer(handle(), data.handle()));
    }

    // assign a bufferevent to a specific event_base.
    // NOTE that only socket bufferevents support this function.
    void set(queue& queue)
    {
        check_result("bufferevent_base_set",
            bufferevent_base_set(queue.handle(), handle()));
    }
};

} // namespace tcp
} // namespace evnet
