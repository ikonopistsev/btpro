#pragma once

#include "curl/curl.h"

#include <functional>
#include <exception>
#include <cassert>

namespace btpro {
namespace curl {

using easy_handle_t = CURL*;
using multi_handle_t = CURLM*;
using throw_fn_type = std::function<void(std::exception_ptr)>;

const char* str_error(CURLcode err)
{
    return curl_easy_strerror(err);
}

const char* str_error(CURLMcode err)
{
    return curl_multi_strerror(err);
}

template<typename T>
void set_opt(easy_handle_t handle, CURLoption option, const T& value)
{
    assert(handle);
    CURLcode code = curl_easy_setopt(handle, option, value);
    if (code != CURLE_OK)
        throw std::runtime_error(str_error(code));
}

template<typename T>
void set_opt(multi_handle_t handle, CURLMoption option, const T& value)
{
    assert(handle);
    CURLMcode code = curl_multi_setopt(handle, option, value);
    if (code != CURLM_OK)
        throw std::runtime_error(str_error(code));
}

struct launch
{
    explicit launch(long f)
    {
        CURLcode code = curl_global_init(f);
        if (code != CURLE_OK)
            throw std::runtime_error(str_error(code));
    }

    ~launch() noexcept
    {
        curl_global_cleanup();
    }
};

static inline void startup(long f = CURL_GLOBAL_DEFAULT)
{
    static const launch launch(f);
}

} // namsspace curl
} // namespace btpro
