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
                static_cast<F*>(that)->on_throw(std::current_exception());
            }

            return 0;
        }
    };

    static inline void assign(easy_handle_t handle, buffer& buf)
    {
        assert(handle);

        // This callback function gets called by libcurl as soon
        // as there is data received that needs to be saved.

        set_opt(handle, CURLOPT_WRITEDATA, &buf);
        set_opt(handle, CURLOPT_WRITEFUNCTION, proxy<buffer>::writecb);
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
                static_cast<F*>(that)->on_throw(std::current_exception());
            }

            return 0;
        }
    };

    static void assign(easy_handle_t handle, buffer& buf)
    {
        assert(handle);

        // This callback function gets called by libcurl as soon as
        // it needs to read data in order to send it to the peer -
        // like if you ask it to upload or post data to the server.

        set_opt(handle, CURLOPT_READDATA, &buf);
        set_opt(handle, CURLOPT_READFUNCTION, proxy<buffer>::readcb);
    }
};

template<class T>
class buffer
{
    on_throw_fn& on_throw_;
    buffer data_{};

public:
    buffer(on_throw_fn& fn)
        : on_throw_(fn)
    {   }

    void append(buffer_ref other)
    {
        data_.append(std::move(other));
    }

    std::size_t append(const char *data, std::size_t size)
    {
        return data_.append(data, size);
    }

    std::size_t copyout(char *data, std::size_t size)
    {
        assert(data);

        size = data_.copyout(data, size);
        data_.drain(size);

        return size;
    }

    void copyout(buffer_ref other)
    {
        other.append(std::move(data_));
    }

    void on_throw(std::exception_ptr ep)
    {
        if (on_throw_)
            on_throw_(ep);
    }

    void assign(easy_handle_t handle)
    {
        T::assign(handle, data_);
    }
};

} // namespace io
} // namespace curl
} // namespace btpro
