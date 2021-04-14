#pragma once

#include "btpro/curl/info.hpp"
#include "btpro/curl/header/store.hpp"

#include <string_view>

namespace btpro {
namespace curl {
namespace header {

template<class T>
class parser
{
    T& handler_;
    store_ref hdr_;

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
                if (handler_.call(hdr_))
                    return size;

                // это приведет к ошибке
                return 0;
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
            handler_.call(std::current_exception());
        }

        return 0;
    }

public:
    parser(T& handler, store_ref hdr) noexcept
        : handler_(handler)
        , hdr_(hdr)
    {   }

    void assign(easy_handle_t easy)
    {
        assert(easy);

        // курл вызывает калбек для каждой строки хидера
        // каждая строка закончится символом перевода строки
        set_opt(easy, CURLOPT_HEADERDATA, this);
        set_opt(easy, CURLOPT_HEADERFUNCTION,
                proxy<parser>::headercb);
    }

    void detach(easy_handle_t easy)
    {
        // курл вызывает калбек для каждой строки хидера
        // каждая строка закончится символом перевода строки
        set_opt(easy, CURLOPT_HEADERDATA, nullptr);
        set_opt(easy, CURLOPT_HEADERFUNCTION, nullptr);
    }
};

template<class H>
class auto_parser
{
    parser<H> parser_;
    easy_handle_t easy_{nullptr};

public:
    auto_parser(H& handler, store_ref hdr) noexcept
        : parser_(handler, hdr)
    {   }

    auto_parser(H& handler, store_ref hdr, easy_handle_t easy)
        : parser_(handler, hdr)
        , easy_(easy)
    {
        parser_.assign(easy);
    }

    ~auto_parser() noexcept
    {
        try
        {
            if (easy_)
                parser_.detach(easy_);
        }
        catch (...)
        {   }
    }

    void assign(easy_handle_t easy)
    {
        if (easy_)
            throw std::runtime_error("parser assigned");
        parser_.assign(easy);
        easy_ = easy;
    }

    void detach()
    {
        if (easy_)
        {
            parser_.detach(easy_);
            easy_ = nullptr;
        }
    }
};

} // parser
} // curl
} // btpro
