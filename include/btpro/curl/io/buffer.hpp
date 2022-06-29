#pragma once

#include "btpro/curl/curl.hpp"
#include "btpro/buffer.hpp"

namespace btpro {
namespace curl {
namespace io {

struct append
{
    template<class F>
    struct proxy
    {
        static size_t writecb(const char* data,
            size_t size, size_t nitems, void *that) noexcept
        {
            assert(that);

            try
            {
                size *= nitems;
                return static_cast<F*>(that)->append(data, size);
            }
            catch (...)
            {
                static_cast<F*>(that)->call(std::current_exception());
            }

            return 0;
        }
    };

    template<template<class,class> class T, class H>
    static void assign(T<H, append>& buf, easy_handle_t handle)
    {
        assert(handle);

        // This callback function gets called by libcurl as soon
        // as there is data received that needs to be saved.

        set_opt(handle, CURLOPT_WRITEDATA, &buf);
        set_opt(handle, CURLOPT_WRITEFUNCTION,
            proxy<T<H, append>>::writecb);
    }
};

struct copyout
{
    template<class F>
    struct proxy
    {
        static size_t readcb(char* data,
            size_t size, size_t nitems, void *that) noexcept
        {
            assert(that);

            try
            {
                size *= nitems;
                return static_cast<F*>(that)->copyout(data, size);
            }
            catch (...)
            {
                static_cast<F*>(that)->call(std::current_exception());
            }

            return 0;
        }
    };

    template<template<class,class> class T, class H>
    static void assign(T<H, copyout>& buf, easy_handle_t easy)
    {
        assert(easy);

        // This callback function gets called by libcurl as soon as
        // it needs to read data in order to send it to the peer -
        // like if you ask it to upload or post data to the server.        
        set_opt(easy, CURLOPT_READDATA, &buf);
        set_opt(easy, CURLOPT_READFUNCTION,
                proxy<T<H, copyout>>::readcb);

        auto size = buf.size();
        if (size)
        {
            set_opt(easy, CURLOPT_POSTFIELDSIZE_LARGE,
                static_cast<curl_off_t>(size));
        }
    }
};

template<class T, class IO>
class buffer
{
    T& handler_;
    btpro::buffer data_{};

public:
    buffer(T& handler)
        : handler_(handler)
    {   }

    void append(btpro::buffer other)
    {
        data_.append(std::move(other));
    }

    std::size_t append(const char *data, std::size_t size)
    {
        data_.append(data, size);

        return size;
    }

    void copyout(buffer_ref other)
    {
        other.append(std::move(data_));
    }

    std::size_t copyout(void *data, std::size_t size)
    {
        assert(data);

        size = data_.copyout(data, size);
        data_.drain(size);

        return size;
    }

    btpro::buffer_ref data() noexcept
    {
        return btpro::buffer_ref(data_);
    }

    void call(std::exception_ptr ep)
    {
        handler_.call(ep);
    }

    bool empty() const noexcept
    {
        return data_.empty();
    }

    std::size_t size() const noexcept
    {
        return data_.size();
    }

    void assign(easy_handle_t easy)
    {
        IO::assign(*this, easy);
    }
};

} // namespace io
} // namespace curl
} // namespace btpro
