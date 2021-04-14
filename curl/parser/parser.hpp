#pragma once

#include "btpro/curl/info.hpp"
#include "btpro/curl/header/store.hpp"

#include <list>
#include <functional>
#include <string_view>

namespace btpro {
namespace curl {
namespace header {

typedef void (*handler_callback_fn)(int, header_ref, void*);

class handler
{
public:
    virtual void call(int code, header_ref hdr) noexcept = 0;

    virtual void call_throw(std::exception_ptr ex) noexcept = 0;
};


template<class T>
class handler_fn final
    : public handler
{
public:
    typedef void (T::*callback_fn)(int, header_ref);
    typedef void (T::*throw_fn)(std::exception_ptr);
    typedef handler_fn<T> this_type;

private:
    T& self_;
    callback_fn fn_{ nullptr };
    throw_fn throw_fn_{ nullptr };

    virtual void call(int code, header_ref hdr) noexcept override
    {
        try
        {
            (self_.*fn_)(code, hdr);
        }
        catch(...)
        {
            call_throw(std::current_exception());
        }
    }

    virtual void call_throw(std::exception_ptr ex) noexcept override
    {
        (self_.*throw_fn_)(ex);
    }

public:
    handler_fn(handler_fn&) = delete;
    handler_fn& operator=(handler_fn&) = delete;

    handler_fn(T& self, callback_fn fn, throw_fn tr) noexcept
        : self_(self)
        , fn_(fn)
        , throw_fn_(tr)
    {
        assert(fn);
        assert(tr);
    }

    void set(callback_fn fn) noexcept
    {
        assert(fn);
        fn_ = fn;
    }

    void set(throw_fn fn) noexcept
    {
        assert(fn);
        throw_fn_ = fn;
    }
};

class parser
{
    handler& handler_;
    easy_handle_t handle_{nullptr};
    header_store hdr_{};

    template<class F>
    struct proxy
    {
        static size_t headercb(const char* data,
            size_t size, size_t nitems, void* self) noexcept
        {
            assert(self);
            return static_cast<F*>(self)->parse(data, size * nitems);
        }
    };

    std::size_t parse(const char* data, size_t size) noexcept
    {
        using namespace std::literals;
        try
        {
            std::string_view kv(data, size);
            // последним приходят символы пустой строки
            if ((kv == "\r\n"sv) || (kv == "\n"sv))
            {
                handler_.call(http_code(handle_), hdr_);
                return size;
            }

            // rtrim
            while (!kv.empty() &&
                ((kv.back() == '\n') || (kv.back() == '\r') ||
                 (kv.back() == ' ')))
            {
                kv = kv.substr(0, kv.size() - 1);
            }

            auto f = kv.find(':');
            if (f != std::string_view::npos)
            {
                // split by ':'
                auto key = kv.substr(0, f);
                auto value = kv.substr(f + 1);

                // ltrim
                while (!value.empty() && (value.front() == ' '))
                    value = value.substr(1);

                if (!value.empty())
                    hdr_.insert(key, value);
            }

            return size;
        }
        catch (...)
        {
            handler_.call_throw(std::current_exception());
        }

        return 0;
    }

public:
    parser(handler& handler)
        : handler_(handler)
    {   }

    void assign(easy_handle_t handle)
    {
        assert(handle);

        // курл вызывает калбек для каждой строки хидера
        // каждая строка закончится символом перевода строки
        set_opt(handle, CURLOPT_HEADERDATA, this);

        set_opt(handle, CURLOPT_HEADERFUNCTION,
                proxy<parser>::headercb);

        handle_ = handle;
    }
};

} // parser
} // curl
} // btpro
