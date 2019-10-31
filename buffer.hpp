#pragma once

#include "btpro/btpro.hpp"

#include "event2/buffer.h"

#include <mutex>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>

namespace btpro {

class buffer
{
public:
    typedef evbuffer* handle_t;

private:
    static inline void destroy(handle_t handle) noexcept
    {
        assert(handle);
        evbuffer_free(handle);
    }

    static inline void leave(handle_t) noexcept
    {   }

    std::unique_ptr<evbuffer, decltype(&leave)>
        handle_{ nullptr, leave };

    static inline handle_t create_buffer()
    {
        auto handle = evbuffer_new();
        if (!handle)
            throw std::runtime_error("evbuffer_new");
        return handle;
    }

    buffer& check_result(const char *what, int result)
    {
        assert(what);
        if (result == code::fail)
            throw std::runtime_error(what);
        return *this;
    }

    template<typename T>
    std::size_t check_size(const char *what, T result) const
    {
        assert(what);
        if (result == static_cast<T>(code::fail))
            throw std::runtime_error(what);
        return static_cast<std::size_t>(result);
    }

public:
    buffer()
        : handle_(create_buffer(), destroy)
    {   }

    buffer(buffer&) = delete;
    buffer& operator=(buffer& buffer) = delete;

    buffer(buffer&&) = default;
    buffer& operator=(buffer&&) = default;

    explicit buffer(handle_t handle)
        : handle_(handle, leave)
    {   }

    template<std::size_t N>
    explicit buffer(std::reference_wrapper<const char[N]> data_ref)
        : handle_(create_buffer(), &destroy)
    {
        append(std::move(data_ref));
    }

    template<class T>
    explicit buffer(std::reference_wrapper<const T> data_ref)
        : handle_(create_buffer(), &destroy)
    {
        append(std::move(data_ref));
    }

    template<class T>
    explicit buffer(const T& text)
        : handle_(create_buffer(), &destroy)
    {
        append(text);
    }

    buffer(const void *data, std::size_t len)
        : handle_(create_buffer(), &destroy)
    {
        append(data, len);
    }

    handle_t handle() const noexcept
    {
        return handle_.get();
    }

    void clear(buffer e = buffer())
    {
        evbuffer_add_buffer(e.handle(), handle());
    }

    buffer& add_file(int fd, ev_off_t offset, ev_off_t length)
    {
        return check_result("evbuffer_add_file",
            evbuffer_add_file(handle(), fd, offset, length));
    }

    buffer& append_ref(const void *data, std::size_t len,
        evbuffer_ref_cleanup_cb cleanupfn, void *cleanupfn_arg)
    {
        assert(data && len);
        return check_result("evbuffer_add_reference",
            evbuffer_add_reference(handle(), data, len,
                cleanupfn, cleanupfn_arg));
    }

    buffer& append_ref(const void *data, std::size_t len)
    {
        assert(data && len);
        return append_ref(data, len, nullptr, nullptr);
    }

    buffer& append(buffer buf)
    {
        return check_result("evbuffer_add_buffer",
            evbuffer_add_buffer(handle(), buf.handle()));
    }

    buffer& append(const void *data, std::size_t len)
    {
        assert(data && len);
        return check_result("evbuffer_add",
            evbuffer_add(handle(), data, len));
    }

    buffer& append(const void *data, std::size_t len, bool ref)
    {
        return (ref) ? append_ref(data, len) : append(data, len);
    }

    template<std::size_t N>
    buffer& append(const char(&data)[N])
    {
        return append(data, N - 1);
    }

    template<std::size_t N>
    buffer& append(std::reference_wrapper<const char[N]> data_ref)
    {
        return append_ref(data_ref.get(), N - 1);
    }

    template<class T>
    buffer& append(const T& str_buf)
    {
        return append(str_buf.data(), str_buf.size());
    }

    template<class T>
    buffer& append(std::reference_wrapper<const T> data_ref)
    {
        const auto& str_buf = data_ref.get();
        append_ref(str_buf.data(), str_buf.size());
        return *this;
    }

    // Prepends data to the beginning of the evbuffer
    buffer& prepend(buffer other)
    {
        return check_result("evbuffer_prepend_buffer",
            evbuffer_prepend_buffer(handle(), other.handle()));
    }

    // Prepends data to the beginning of the evbuffer
    buffer& prepend(const void *data, std::size_t len)
    {
        assert((data != nullptr) && len);
        return check_result("evbuffer_prepend",
            evbuffer_prepend(handle(), data, len));
    }

    // Prepends data to the beginning of the evbuffer
    template<std::size_t N>
    buffer& prepend(const char(&data)[N])
    {
        return prepend(data, N - 1);
    }

    // Prepends data to the beginning of the evbuffer
    template<class T>
    buffer& prepend(const T& str_buf)
    {
        return prepend(str_buf.data(), str_buf.size());
    }

    // Remove a specified number of bytes data from the beginning of an evbuffer.
    std::size_t drain(std::size_t len)
    {
        check_result("evbuffer_drain",
            evbuffer_drain(handle(), len));
        return size();
    }

    // Remove a specified number of bytes data from the beginning of an evbuffer.
    std::size_t drain(std::string& text, std::size_t len)
    {
        std::size_t sz = (std::min)(size(), len);
        if (sz)
        {
            text.assign(reinterpret_cast<char*>(pullup(sz)), sz);
            return drain(sz);
        }
        return size();
    }

    // Expands the available space in the evbuffer to at least datlen,
    // so that appending datlen additional bytes
    // will not require any new allocations
    buffer& expand(std::size_t size)
    {
        return check_result("evbuffer_expand",
            evbuffer_expand(handle(), size));
    }

    std::size_t drain(std::string& text)
    {
        return drain(text, size());
    }

    // Read data from an evbuffer, and leave the buffer unchanged.
    std::size_t copyout(void *out, std::size_t len) const
    {
        assert(out && len);
        return check_size("evbuffer_copyout",
            evbuffer_copyout(handle(), out, len));
    }

    // Read data from an evbuffer, and leave the buffer unchanged.
    template<std::size_t N>
    std::size_t copyout(char(&out)[N]) const
    {
        std::size_t result = copyout(out, N - 1);
        out[result] = '\0';
        return result;
    }

    // Makes the data at the beginning of an evbuffer contiguous.
    unsigned char* pullup(ev_ssize_t len)
    {
        assert(static_cast<ev_ssize_t>(size()) <= len);
        unsigned char *result = evbuffer_pullup(handle(), len);
        if (!result)
        {
            if (empty())
            {
                static unsigned char nil;
                result = &nil;
            }
            else
                throw std::runtime_error("evbuffer_pullup");
        }
        return result;
    }

    // Returns the total number of bytes stored in the evbuffer
    std::size_t size() const noexcept
    {
        return evbuffer_get_length(handle());
    }

    // Returns the number of contiguous available bytes in the first buffer chain.
    std::size_t contiguous_space() const noexcept
    {
        return evbuffer_get_contiguous_space(handle());
    }

    buffer& enable_locking()
    {
        return check_result("evbuffer_enable_locking",
            evbuffer_enable_locking(handle(), nullptr));
    }

    void lock() const noexcept
    {
        evbuffer_lock(handle());
    }

    void unlock() const noexcept
    {
        evbuffer_unlock(handle());
    }

    template<typename T>
    void sync(T&& func)
    {
        std::lock_guard<buffer> l(*this);
        func(*this);
    }

    std::vector<char> vector() const
    {
        std::vector<char> vec;
        std::size_t len = size();
        if (len)
        {
            vec.resize(len);
            copyout(vec.data(), len);
        }
        return vec;
    }

    std::string str() const
    {
        std::vector<char> vec = vector();
        return { vec.data(), vec.size() };
    }

    bool empty() const noexcept
    {
        return size() == 0;
    }

    // Read data from an evbuffer and drain the bytes read.
    std::size_t remove(void *out, std::size_t len) const
    {
        assert(out && len);
        return check_size("evbuffer_remove",
            evbuffer_remove(handle(), out, len));
    }

    // Read data from an evbuffer and drain the bytes read.
    template<std::size_t N>
    std::size_t remove(char(&out)[N]) const
    {
        std::size_t result = remove(out, N - 1);
        out[result] = '\0';
        return result;
    }
};

} // namespace btpro
