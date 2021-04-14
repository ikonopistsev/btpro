#pragma once

#include "btpro/buffer.hpp"
#include "btpro/curl/info.hpp"
#include "btpro/curl/io/buffer.hpp"
#include "btpro/curl/header/parser.hpp"

#include "btpro/wslay/context.hpp"

#include "btpro/ssl/base64.hpp"
#include "btpro/ssl/rand.hpp"
#include "btpro/ssl/sha.hpp"

#include <string_view>

namespace btpro {
namespace curl {

// resp
class resp
{
    easy_handle_t easy_{};
    CURLcode error_{};
    btpro::buffer_ref buffer_{};

public:
    using fn_type = std::function<void(resp)>;

    resp(easy_handle_t easy, CURLcode error) noexcept
        : easy_(easy)
        , error_(error)
    {
        assert(easy);
    }

    resp(easy_handle_t easy, CURLcode error,
         btpro::buffer_ref buffer, header::store_ref) noexcept
        : easy_(easy)
        , error_(error)
        , buffer_(std::move(buffer))
    {
        assert(easy);
    }

    CURLcode error() const noexcept
    {
        return error_;
    }

    btpro::buffer_ref& buffer() noexcept
    {
        return buffer_;
    }

    easy_handle_t handle() const noexcept
    {
        return easy_;
    }

    operator easy_handle_t() const noexcept
    {
        return handle();
    }
};


// resp_ext
class resp_ext
    : public resp
{
    header::store_ref hdr_;

public:
    using fn_type = std::function<void(resp_ext)>;

    resp_ext(easy_handle_t easy, CURLcode code,
        btpro::buffer_ref buffer, header::store_ref hdr) noexcept
        : resp(easy, code, std::move(buffer), hdr)
        , hdr_(hdr)
    {   }

    auto begin() const noexcept
    {
        return hdr_.begin();
    }

    auto end() const noexcept
    {
        return hdr_.end();
    }

    auto find(std::string_view key)
    {
        return hdr_.find(key);
    }

    auto get(std::string_view key)
    {
        return hdr_.get(key);
    }
};


// detail
namespace detail {

class base_resp
{
public:
    using error_fn_type = std::function<void(std::exception_ptr)>;

protected:
    error_fn_type error_fn_{};

public:
    virtual void set(error_fn_type fn)
    {
        error_fn_ = std::move(fn);
    }

    virtual void assign(easy_handle_t) = 0;

    virtual void done(easy_handle_t, CURLcode) noexcept = 0;

    virtual void error(std::exception_ptr ex) noexcept
    {
        if (error_fn_)
            error_fn_(ex);
    }

    // является ли вебсокетом
    virtual bool stream() noexcept
    {
        return false;
    }
};

template<class T>
class get_resp
    : public base_resp
{
public:
    using result_type = T;
    using fn_type = typename T::fn_type;
    using this_type = get_resp<T>;
    using inbuf_type = io::buffer<this_type, io::append>;

    using header_fn_type = std::function<bool(header::store_cref)>;

private:
    fn_type done_{};

    header::store hdr_{};
    header::parser<this_type> hparse_{*this, hdr_};
    header_fn_type header_fn_{};

    inbuf_type inbuf_{*this};

    virtual void done(easy_handle_t easy, CURLcode code) noexcept override
    {
        try
        {
            done_(result_type(easy, code, inbuf_.data(), hdr_));
        }
        catch (...)
        {
            error(std::current_exception());
        }
    }

public:

    get_resp(fn_type fn)
        : done_(std::move(fn))
    {   }

    void assign(easy_handle_t easy) override
    {
        assert(easy);
        hparse_.assign(easy);
        inbuf_.assign(easy);
    }

    void set(fn_type fn)
    {
        done_ = std::move(fn);
    }

    void set(header_fn_type fn)
    {
        header_fn_ = std::move(fn);
    }

    void call(std::exception_ptr ex)
    {
        error(ex);
    }

    bool call(header::store_ref hdr)
    {
        return (header_fn_) ?
            header_fn_(hdr) : true;
    }
};


class websocket
    : public base_resp
{


    // стал ли запрос вебсокетом
    bool stream_{false};

    virtual void assign(easy_handle_t) override
    {

    }

    virtual void done(easy_handle_t, CURLcode) noexcept override
    {

    }

    virtual void error(std::exception_ptr) noexcept override
    {

    }

    // является ли вебсокетом
    virtual bool stream() noexcept
    {
        return stream_;
    }
};

} // namespace resp
} // namespace curl
} // namespace btpro
