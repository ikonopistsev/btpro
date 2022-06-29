#pragma once

#include "btpro/curl/responce/base.hpp"
#include "btpro/curl/option.hpp"

namespace btpro {
namespace curl {
namespace request {

class base
{
    static inline easy_handle_t create_handle()
    {
        auto easy = curl_easy_init();
        if (!easy)
            throw std::runtime_error("curl_easy_init");
        return easy;
    }

    std::unique_ptr<CURL, decltype(&curl_easy_cleanup)>
        easy_{create_handle(), curl_easy_cleanup};

protected:
    request_header outhdr_{};
    std::unique_ptr<responce::base> responce_{};

    void reset(responce::base *resp)
    {
        assert(resp);

        responce_.reset(resp);
    }

public:
    base(responce::base *resp) noexcept
        : responce_(resp)
    {   }

    virtual ~base()
    {   }

    // добавить хдер
    void push(std::string_view key, std::string_view value)
    {
        outhdr_.push(key, value);
    }

    // добаить хидер строку формат - "key:value"
    void push_header(std::string_view kv)
    {
        outhdr_.push_header(kv);
    }

    easy_handle_t handle() const noexcept
    {
        return easy_.get();
    }

    operator easy_handle_t() const noexcept
    {
        return handle();
    }

    bool empty() const noexcept
    {
        return nullptr == handle();
    }

    void set(const std::string& url)
    {
        set_opt(handle(), CURLOPT_URL, url.c_str());
    }

    void set(CURLoption pref, long val)
    {
        set_opt(handle(), pref, val);
    }

    template<class T, CURLoption Opt>
    void set(const basic_option<T, Opt>& opt)
    {
        opt(*this);
    }

    CURLcode perform()
    {
        if (!outhdr_.empty())
            outhdr_(handle());

        return curl_easy_perform(handle());
    }

    void done(CURLcode code) noexcept
    {
        resp_->done(handle(), code);
    }

    void error(std::exception_ptr ex) noexcept
    {
        resp_->error(ex);
    }

    // является ли вебсокетом
    bool stream() noexcept
    {
        return resp_->stream();
    }
};

} // namespace
} // namespace
} // namespace
