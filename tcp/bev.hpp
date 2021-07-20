#pragma once

#include "btpro/dns.hpp"
#include "btpro/buffer.hpp"
#include "btpro/socket.hpp"
#include "btpro/tcp/tcp.hpp"

#include "event2/bufferevent.h"

#include <functional>

namespace btpro {
namespace tcp {

typedef bufferevent* bufferevent_handle_t;

template<class T>
class bevfn;

class bev
{
public:
    typedef bufferevent_handle_t handle_t;

private:

    handle_t hbev_{ nullptr };

    handle_t assert_handle() const noexcept
    {
        auto hbev = handle();
        assert(hbev);
        return hbev;
    }

    void check_result(const char *what, int result)
    {
        assert(what && (what[0] != '\0'));
        if (result == code::fail)
            throw std::runtime_error(what);
    }

    buffer_handle_t output_handle() const noexcept
    {
        return bufferevent_get_output(assert_handle());
    }

    buffer_handle_t input_handle() const noexcept
    {
        return bufferevent_get_input(assert_handle());
    }

    // хэндл очереди
    queue_handle_t queue_handle() const noexcept
    {
        return bufferevent_get_base(assert_handle());
    }

public:
    bev() = default;

    bev(const bev&) = delete;
    bev& operator=(const bev&) = delete;

    bev(handle_t hbev) noexcept
        : hbev_(hbev)
    {
        assert(hbev);
    }

    void attach(handle_t hbev) noexcept
    {
        hbev_ = hbev;
    }

    void create(queue_handle_t queue,
        be::socket sock, int opt = BEV_OPT_CLOSE_ON_FREE)
    {
        assert(queue);
        auto hbev = bufferevent_socket_new(queue, sock.fd(), opt);
        check_result("bufferevent_socket_new",
            hbev != nullptr ? code::sucsess : code::fail);
        hbev_ = hbev;
    }

    void create(queue_handle_t queue, int opt = BEV_OPT_CLOSE_ON_FREE)
    {
        create(queue, be::socket(), opt);
    }

    void destroy() noexcept
    {
        auto hbev = handle();
        if (nullptr != hbev)
        {
            bufferevent_free(hbev);
            hbev_ = nullptr;
        }
    }

    ~bev() noexcept
    {
        auto hbev = handle();
        if (nullptr != hbev)
            bufferevent_free(hbev);
    }

    bev(bev&& other) noexcept
    {
        std::swap(hbev_, other.hbev_);
    }

    bev& operator=(bev&& other) noexcept
    {
        std::swap(hbev_, other.hbev_);
        return *this;
    }

    buffer_ref input() const noexcept
    {
        return buffer_ref(input_handle());
    }

    buffer_ref output() const noexcept
    {
        return buffer_ref(output_handle());
    }

    auto queue() const noexcept
    {
        return queue_handle();
    }

    int get_dns_error() const noexcept
    {
        return bufferevent_socket_get_dns_error(assert_handle());
    }

    void connect(const sockaddr* sa, ev_socklen_t len)
    {
        assert(sa);
        check_result("bufferevent_socket_connect",
            bufferevent_socket_connect(assert_handle(),
                const_cast<sockaddr*>(sa), static_cast<int>(len)));
    }

    void connect(const ip::addr& addr)
    {
        connect(addr.sa(), addr.size());
    }

    void connect(dns_handle_t dns, int af, const std::string& hostname, int port)
    {
        assert(dns);
        check_result("bufferevent_socket_connect_hostname",
            bufferevent_socket_connect_hostname(assert_handle(), dns,
                af, hostname.c_str(), port));
    }

    void connect(dns_handle_t dns, const std::string& hostname, int port)
    {
        connect(dns, AF_UNSPEC, hostname, port);
    }

    void connect4(dns_handle_t dns, const std::string& hostname, int port)
    {
        connect(dns, AF_INET, hostname, port);
    }

    void connect6(dns_handle_t dns, const std::string& hostname, int port)
    {
        connect(dns, AF_INET6, hostname, port);
    }

    handle_t handle() const noexcept
    {
        return hbev_;
    }

    operator handle_t() const noexcept
    {
        return  handle();
    }

    evutil_socket_t fd() const noexcept
    {
        return bufferevent_getfd(assert_handle());
    }

    be::socket socket() const noexcept
    {
        return be::socket(fd());
    }

//    evil
//    operator be::socket() const noexcept
//    {
//        return socket();
//    }

    void enable(short event)
    {
        check_result("bufferevent_enable",
            bufferevent_enable(assert_handle(), event));
    }

    void disable(short event)
    {
        check_result("bufferevent_disable",
            bufferevent_disable(assert_handle(), event));
    }

    void set_watermark(short events, std::size_t lowmark,
                       std::size_t highmark) noexcept
    {
        bufferevent_setwatermark(assert_handle(), events, lowmark, highmark);
    }

    ev_ssize_t get_max_to_read() const noexcept
    {
        return bufferevent_get_max_to_read(assert_handle());
    }

    ev_ssize_t get_max_to_write() const noexcept
    {
        return bufferevent_get_max_to_write(assert_handle());
    }

    void lock() const noexcept
    {
        bufferevent_lock(assert_handle());
    }

    void unlock() const noexcept
    {
        bufferevent_unlock(assert_handle());
    }

    template<typename T>
    void sync(T&& func)
    {
        std::lock_guard<bev> l(*this);
        func(*this);
    }

    void write(const void *data, std::size_t size)
    {
        check_result("bufferevent_write",
            bufferevent_write(assert_handle(), data, size));
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
        auto fn_ptr = new std::function<void()>(std::move(fn));
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
        auto fn_ptr = new std::function<void()>(std::move(fn));
        // выделяем память под буфер
        auto ptr = malloc(size);
        check_result("write malloc",
            ptr != nullptr ? code::sucsess : code::fail);

        // копируем память
        memcpy(ptr, data, size);

        check_result("evbuffer_add_reference",
            evbuffer_add_reference(output_handle(), ptr, size,
                ref_buffer<std::function<void()>>::clean_fn_all, fn_ptr));
    }

    template<class Ref>
    void write(basic_buffer<Ref> buf)
    {
        check_result("bufferevent_write_buffer",
            bufferevent_write_buffer(assert_handle(), buf));
    }

    buffer read()
    {
        buffer result;
        check_result("bufferevent_read_buffer",
            bufferevent_read_buffer(assert_handle(), result));
        return result;
    }

    // assign a bufferevent to a specific event_base.
    // NOTE that only socket bufferevents support this function.
    void set(queue_handle_t queue)
    {
        assert(queue);
        check_result("bufferevent_base_set",
            bufferevent_base_set(queue, assert_handle()));
    }

    void set(evutil_socket_t fd)
    {
        check_result("bufferevent_setfd",
            bufferevent_setfd(assert_handle(), fd));
    }

    void set(be::socket sock)
    {
        set(sock.fd());
    }

    void set(bufferevent_data_cb rdfn, bufferevent_data_cb wrfn,
        bufferevent_event_cb evfn, void *arg) noexcept
    {
        bufferevent_setcb(assert_handle(), rdfn, wrfn, evfn, arg);
    }

    void set_timeout(timeval *timeout_read, timeval *timeout_write)
    {
        bufferevent_set_timeouts(assert_handle(), timeout_read, timeout_write);
    }

    template<class T>
    void set(bevfn<T>& val)
    {
        val.apply(*this);
    }
};

} // namespace tcp
} // namespace evnet
