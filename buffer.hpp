#pragma once

#include "btpro/btpro.hpp"
#include "btpro/socket.hpp"
#include "event2/buffer.h"

#include <mutex>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>

namespace btpro {

typedef evbuffer* buffer_handle_t;

namespace detail {

template<class R>
struct buffer_create;

template<>
struct buffer_create<tag_ref>
{
    constexpr static inline buffer_handle_t create_handle() noexcept
    {
        return nullptr;
    }
};

template<>
struct buffer_create<tag_obj>
{
    static inline buffer_handle_t create_handle()
    {
        auto hbuf = evbuffer_new();
        if (!hbuf)
            throw std::runtime_error("evbuffer_new");
        return hbuf;
    }
};

template<class T>
struct buffer_destroy;

template<>
struct buffer_destroy<tag_ref>
{
    constexpr static inline void destroy_handle(buffer_handle_t) noexcept
    {   }
};

template<>
struct buffer_destroy<tag_obj>
{
    static inline void destroy_handle(buffer_handle_t hbuf) noexcept
    {
        evbuffer_free(hbuf);
    }
};

template<class tag_ref>
struct buffer_handle;

template<>
struct buffer_handle<tag_ref>
{
    static inline void check(buffer_handle_t hbuf)
    {
        if (nullptr == hbuf)
            throw std::runtime_error("buffer_ref empty");
    }
};

template<>
struct buffer_handle<tag_obj>
{
    constexpr static inline void check(buffer_handle_t) noexcept
    {   }
};

} // detail

template<class R>
class basic_buffer;

typedef basic_buffer<tag_ref> buffer_ref;
typedef basic_buffer<tag_obj> buffer;

template<class R>
class basic_buffer
{
public:
    typedef evbuffer* handle_t;
    constexpr static bool is_ref = R::is_ref;

private:
    handle_t hbuf_{ detail::buffer_create<R>::create_handle() };

    handle_t assert_handle() const noexcept
    {
        auto hbuf = handle();
        assert(hbuf);
        return hbuf;
    }

    basic_buffer& check_result(const char* what, int result)
    {
        assert(what);

        if (result == code::fail)
            throw std::runtime_error(what);

        return *this;
    }

    template<class T>
    std::size_t check_size(const char* what, T result) const
    {
        assert(what);

        if (result == static_cast<T>(code::fail))
            throw std::runtime_error(what);

        return static_cast<std::size_t>(result);
    }

public:
    basic_buffer()
        : hbuf_(detail::buffer_create<R>::create_handle())
    {   }

    ~basic_buffer() noexcept
    {
        detail::buffer_destroy<R>::destroy_handle(hbuf_);
    }

    basic_buffer(basic_buffer&& that) noexcept
    {
        std::swap(hbuf_, that.hbuf_);
    }

    basic_buffer& operator=(basic_buffer&& that) noexcept
    {
        std::swap(hbuf_, that.hbuf_);
        return *this;
    }

    explicit basic_buffer(handle_t hbuf) noexcept
        : hbuf_(hbuf)
    {
        assert(hbuf);
        static_assert(is_ref, "buffer_ref only");
    }

    buffer_ref& operator=(handle_t hbuf) noexcept
    {
        assert(hbuf);
        hbuf_ = hbuf;
        return *this;
    }

    template<class T>
    basic_buffer(const basic_buffer<T>& other) noexcept
        : basic_buffer(other.handle())
    {   }

    template<class T>
    buffer_ref& operator=(const basic_buffer<T>& other) noexcept
    {
        *this = other.hbuf_;
        return *this;
    }

    handle_t handle() const noexcept
    {
        return hbuf_;
    }

    operator handle_t() const noexcept
    {
        return handle();
    }

    template<std::size_t N>
    explicit basic_buffer(std::reference_wrapper<const char[N]> data_ref)
    {
        static_assert(!is_ref, "no buffer_ref");
        append(std::move(data_ref));
    }

    template<class T>
    explicit basic_buffer(std::reference_wrapper<const T> data_ref)
    {
        static_assert(!is_ref, "no buffer_ref");
        append(std::move(data_ref));
    }

    template<class T>
    explicit basic_buffer(const T& text)
    {
        static_assert(!is_ref, "no buffer_ref");
        append(text);
    }

    basic_buffer(const void *data, std::size_t len)
    {
        static_assert(!is_ref, "no buffer_ref");
        append(data, len);
    }

    void clear(buffer other = buffer())
    {
        evbuffer_add_buffer(assert_handle(), other);
    }

    basic_buffer& add_file(int fd, ev_off_t offset, ev_off_t length)
    {
        return check_result("evbuffer_add_file",
            evbuffer_add_file(assert_handle(), fd, offset, length));
    }

    basic_buffer& append_ref(const void *data, std::size_t len,
        evbuffer_ref_cleanup_cb cleanupfn, void *cleanupfn_arg)
    {
        assert(data && len);
        return check_result("evbuffer_add_reference",
            evbuffer_add_reference(assert_handle(), data, len,
                                   cleanupfn, cleanupfn_arg));
    }

    basic_buffer& append_ref(const void *data, std::size_t len)
    {
        return append_ref(data, len, nullptr, nullptr);
    }

    template<std::size_t N>
    basic_buffer& append_ref(std::reference_wrapper<const char[N]> data_ref)
    {
        return append_ref(data_ref, N - 1);
    }

    basic_buffer& append(buffer_ref) = delete;
    basic_buffer& append(buffer buf)
    {
        return check_result("evbuffer_add_buffer",
            evbuffer_add_buffer(assert_handle(), buf));
    }

    basic_buffer& append(const void *data, std::size_t len)
    {
        assert(data && len);
        return check_result("evbuffer_add",
            evbuffer_add(assert_handle(), data, len));
    }

    basic_buffer& append(const void *data, std::size_t len, bool ref)
    {
        return (ref) ?
            append_ref(data, len) : append(data, len);
    }

    template<std::size_t N>
    basic_buffer& append(const char(&data)[N])
    {
        return append(data, N - 1);
    }

    template<std::size_t N>
    basic_buffer& append(std::reference_wrapper<const char[N]> data_ref)
    {
        return append_ref(data_ref.get(), N - 1);
    }

    template<class T>
    basic_buffer& append(const T& str_buf)
    {
        return append(str_buf.data(), str_buf.size());
    }

    template<class T>
    basic_buffer& append(std::reference_wrapper<const T> data_ref)
    {
        const auto& str_buf = data_ref.get();
        append_ref(str_buf.data(), str_buf.size());
        return *this;
    }

    // Prepends data to the beginning of the evbuffer
    basic_buffer& prepend(buffer_ref) = delete;
    basic_buffer& prepend(buffer buf)
    {
        return check_result("evbuffer_prepend_buffer",
            evbuffer_prepend_buffer(assert_handle(), buf));
    }

    // Prepends data to the beginning of the evbuffer
    basic_buffer& prepend(const void *data, std::size_t len)
    {
        assert((nullptr != data) && len);
        return check_result("evbuffer_prepend",
            evbuffer_prepend(assert_handle(), data, len));
    }

    // Prepends data to the beginning of the evbuffer
    template<std::size_t N>
    basic_buffer& prepend(const char(&data)[N])
    {
        return prepend(data, N - 1);
    }

    // Prepends data to the beginning of the evbuffer
    template<class T>
    basic_buffer& prepend(const T& str_buf)
    {
        return prepend(str_buf.data(), str_buf.size());
    }

    // Remove a specified number of bytes data from the beginning of an evbuffer.
    std::size_t drain(std::size_t len)
    {
        check_result("evbuffer_drain",
            evbuffer_drain(assert_handle(), len));
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
    basic_buffer& expand(std::size_t size)
    {
        return check_result("evbuffer_expand",
            evbuffer_expand(assert_handle(), size));
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
            evbuffer_copyout(assert_handle(), out, len));
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
        // @return a pointer to the contiguous memory array, or NULL
        // if param size requested more data than is present in the buffer.
        assert(len <= static_cast<ev_ssize_t>(size()));

        unsigned char *result = evbuffer_pullup(assert_handle(), len);
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
        return evbuffer_get_length(assert_handle());
    }

    // Returns the number of contiguous available bytes in the first buffer chain.
    std::size_t contiguous_space() const noexcept
    {
        return evbuffer_get_contiguous_space(assert_handle());
    }

    basic_buffer& enable_locking()
    {
        return check_result("evbuffer_enable_locking",
            evbuffer_enable_locking(assert_handle(), nullptr));
    }

    void lock() const noexcept
    {
        evbuffer_lock(assert_handle());
    }

    void unlock() const noexcept
    {
        evbuffer_unlock(assert_handle());
    }

    template<typename T>
    void sync(T&& func)
    {
        std::lock_guard<basic_buffer> l(*this);
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
            evbuffer_remove(assert_handle(), out, len));
    }

    // Read data from an evbuffer and drain the bytes read.
    template<std::size_t N>
    std::size_t remove(char(&out)[N]) const
    {
        std::size_t result = remove(out, N - 1);
        out[result] = '\0';
        return result;
    }

    int read(evutil_socket_t fd, int howmuch)
    {
        return evbuffer_read(assert_handle(), fd, howmuch);
    }

    int read(socket sock, int howmuch)
    {
        return read(sock.fd(), howmuch);
    }

    int write(evutil_socket_t fd)
    {
        return evbuffer_write(assert_handle(), fd);
    }

    int write(socket sock)
    {
        return write(sock.fd());
    }

    int wirte(buffer buf)
    {
        auto size = buf.size();
        buf.write(std::move(*this));
        return size;
    }
};

} // namespace btpro
