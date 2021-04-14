#pragma once

#include "btpro/curl/slist.hpp"

#include <string_view>
#include <cstdio>

namespace btpro {
namespace curl {

template<class T, CURLoption Opt>
class basic_option;

template<CURLoption Opt>
class basic_option<std::string, Opt>
{
    std::string value_{};
    CURLoption opt_{Opt};

    void set(easy_handle_t handle) const
    {
        set_opt(handle, opt_, value_.data());
    }

public:
    basic_option(std::string val) noexcept
        : value_(std::move(val))
    {   }

    template<class E>
    void operator()(E& target) const
    {
        set(target.handle());
    }
};

auto url(std::string val)
{
    return basic_option<std::string, CURLOPT_URL>(std::move(val));
}

auto username(std::string val)
{
    return basic_option<std::string, CURLOPT_USERNAME>(std::move(val));
}

auto password(std::string val)
{
    return basic_option<std::string, CURLOPT_PASSWORD>(std::move(val));
}

auto userpwd(std::string val)
{
    return basic_option<std::string, CURLOPT_USERPWD>(std::move(val));
}

auto cookie(std::string val)
{
    return basic_option<std::string, CURLOPT_COOKIE>(std::move(val));
}

auto cookie_file(std::string val)
{
    return basic_option<std::string, CURLOPT_COOKIEFILE>(std::move(val));
}

auto cookie_list(std::string val)
{
    return basic_option<std::string, CURLOPT_COOKIELIST>(std::move(val));
}

auto cookie_jar(std::string val)
{
    return basic_option<std::string, CURLOPT_COOKIEJAR>(std::move(val));
}

auto proxy(std::string val)
{
    return basic_option<std::string, CURLOPT_PROXY>(std::move(val));
}

auto noproxy(std::string val)
{
    return basic_option<std::string, CURLOPT_NOPROXY>(std::move(val));
}

auto proxy_username(std::string val)
{
    return basic_option<std::string, CURLOPT_PROXYUSERNAME>(std::move(val));
}

auto proxy_password(std::string val)
{
    return basic_option<std::string, CURLOPT_PROXYPASSWORD>(std::move(val));
}

auto proxy_userpwd(std::string val)
{
    return basic_option<std::string, CURLOPT_PROXYUSERPWD>(std::move(val));
}

auto tlsauth_username(std::string val)
{
    return basic_option<std::string, CURLOPT_TLSAUTH_USERNAME>(std::move(val));
}

auto tlsauth_password(std::string val)
{
    return basic_option<std::string, CURLOPT_TLSAUTH_PASSWORD>(std::move(val));
}

auto referer(std::string val)
{
    return basic_option<std::string, CURLOPT_REFERER>(std::move(val));
}

auto user_agent(std::string val)
{
    return basic_option<std::string, CURLOPT_USERAGENT>(std::move(val));
}

auto ssl_engine(std::string val)
{
    return basic_option<std::string, CURLOPT_SSLENGINE>(std::move(val));
}

auto ssl_cert_type(std::string val)
{
    return basic_option<std::string, CURLOPT_SSLCERTTYPE>(std::move(val));
}

auto ssl_cert(std::string val)
{
    return basic_option<std::string, CURLOPT_SSLCERT>(std::move(val));
}

auto key_passwd(std::string val)
{
    return basic_option<std::string, CURLOPT_KEYPASSWD>(std::move(val));
}

auto ssl_key_type(std::string val)
{
    return basic_option<std::string, CURLOPT_SSLKEYTYPE>(std::move(val));
}

auto ca_info(std::string val)
{
    return basic_option<std::string, CURLOPT_CAINFO>(std::move(val));
}

auto accept_encoding(std::string val)
{
    return basic_option<std::string, CURLOPT_ACCEPT_ENCODING>(std::move(val));
}

using request_header = basic_slist<CURLOPT_HTTPHEADER>;

} // namespace http
} // namespace minie

