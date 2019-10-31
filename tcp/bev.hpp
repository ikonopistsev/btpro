#pragma once

#include "btpro/dns.hpp"
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

    typedef std::unique_ptr<bufferevent,
        decltype(&bufferevent_free)> value_type;

private:

    value_type handle_{ nullptr, bufferevent_free };

    void check_result(const char *what, int result)
    {
        assert(what);
        assert(what[0] != '\0');
        if (result == code::fail)
            throw std::runtime_error(what);
    }

    buffer::handle_t output_handle() const noexcept
    {
        return bufferevent_get_output(handle());
    }

    buffer::handle_t input_handle() const noexcept
    {
        return bufferevent_get_input(handle());
    }

public:
    bev() = default;

    bev(bev&&) = default;
    bev& operator=(bev&&) = default;

    explicit bev(value_type ptr) noexcept
        : handle_(std::move(ptr))
    {   }

    void assign(handle_t handle) noexcept
    {
        assert(handle);
        handle_.reset(handle);
    }

    std::size_t output_length() const noexcept
    {
        return evbuffer_get_length(output_handle());
    }

    std::size_t input_length() const noexcept
    {
        return evbuffer_get_length(input_handle());
    }

    bool output_empty() const noexcept
    {
        return output_length() == 0;
    }

    bool input_empty() const noexcept
    {
        return input_length() == 0;
    }

    buffer input() const noexcept
    {
        return buffer(input_handle());
    }

    buffer output() const noexcept
    {
        return buffer(output_handle());
    }

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

    void connect(dns& dns, int af, const std::string& hostname, int port)
    {
        check_result("bufferevent_socket_connect_hostname",
            bufferevent_socket_connect_hostname(handle(), dns.handle(),
                af, hostname.c_str(), port));
    }

    void connect(dns& dns, const std::string& hostname, int port)
    {
        connect(dns, AF_UNSPEC, hostname, port);
    }

    void connect4(dns& dns, const std::string& hostname, int port)
    {
        connect(dns, AF_INET, hostname, port);
    }

    void connect6(dns& dns, const std::string& hostname, int port)
    {
        connect(dns, AF_INET6, hostname, port);
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
            evbuffer_add_reference(output_handle(), data, size,
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
            evbuffer_add_reference(output_handle(), ptr, size,
                ref_buffer<std::function<void()>>::clean_fn_all, fn_ptr));
    }

    void write(buffer data)
    {
        check_result("bufferevent_write_buffer",
            bufferevent_write_buffer(handle(), data.handle()));
    }

    // assign a bufferevent to a specific event_base.
    // NOTE that only socket bufferevents support this function.
    void set(queue& queue)
    {
        check_result("bufferevent_base_set",
            bufferevent_base_set(queue.handle(), handle()));
    }

    void set(bufferevent_data_cb rdfn, bufferevent_data_cb wrfn,
        bufferevent_event_cb evfn, void *arg) noexcept
    {
        bufferevent_setcb(handle(), rdfn, wrfn, evfn, arg);
    }
};

} // namespace tcp
} // namespace evnet
