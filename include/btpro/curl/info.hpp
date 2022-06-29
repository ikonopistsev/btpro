#pragma once

#include "btpro/curl/curl.hpp"

namespace btpro {
namespace curl {

template<class T, CURLINFO Inf>
T get_info(easy_handle_t handle)
{
    assert(handle);

    T rc = T();

    CURLcode code = curl_easy_getinfo(handle, Inf, &rc);
    if (code != CURLE_OK)
        throw std::runtime_error(str_error(code));

    return rc;
}

auto content_length(easy_handle_t handle)
{
    return get_info<curl_off_t, CURLINFO_SIZE_DOWNLOAD_T>(handle);
}

auto effective_url(easy_handle_t handle)
{
    return get_info<const char*, CURLINFO_EFFECTIVE_URL>(handle);
}

auto response_code(easy_handle_t handle)
{
    return get_info<long, CURLINFO_RESPONSE_CODE>(handle);
}

auto http_code(easy_handle_t handle)
{
    return response_code(handle);
}

auto http_connect_code(easy_handle_t handle)
{
    return get_info<long, CURLINFO_HTTP_CONNECTCODE>(handle);
}

auto file_time(easy_handle_t handle)
{
    return get_info<long, CURLINFO_FILETIME>(handle);
}

auto total_time(easy_handle_t handle)
{
    return get_info<double, CURLINFO_TOTAL_TIME>(handle);
}

auto name_lookup_time(easy_handle_t handle)
{
    return get_info<double, CURLINFO_NAMELOOKUP_TIME>(handle);
}

auto num_connects(easy_handle_t handle)
{
    return get_info<long, CURLINFO_NUM_CONNECTS>(handle);
}

auto connect_time(easy_handle_t handle)
{
    return get_info<double, CURLINFO_CONNECT_TIME>(handle);
}

auto appconnect_time(easy_handle_t handle)
{
    return get_info<double, CURLINFO_APPCONNECT_TIME>(handle);
}

auto pretransfer_time(easy_handle_t handle)
{
    return get_info<double, CURLINFO_PRETRANSFER_TIME>(handle);
}

auto starttransfer_time(easy_handle_t handle)
{
    return get_info<double, CURLINFO_STARTTRANSFER_TIME>(handle);
}

auto redirect_time(easy_handle_t handle)
{
    return get_info<double, CURLINFO_REDIRECT_TIME>(handle);
}

auto redirect_count(easy_handle_t handle)
{
    return get_info<long, CURLINFO_REDIRECT_COUNT>(handle);
}

auto redirect_url(easy_handle_t handle)
{
    return get_info<const char*, CURLINFO_REDIRECT_URL>(handle);
}

auto size_upload(easy_handle_t handle)
{
    return get_info<double, CURLINFO_SIZE_UPLOAD>(handle);
}

auto size_download(easy_handle_t handle)
{
    return get_info<double, CURLINFO_SIZE_DOWNLOAD>(handle);
}

auto speed_download(easy_handle_t handle)
{
    return get_info<double, CURLINFO_SPEED_DOWNLOAD>(handle);
}

auto speed_upload(easy_handle_t handle)
{
    return get_info<double, CURLINFO_SPEED_UPLOAD>(handle);
}

auto header_size(easy_handle_t handle)
{
    return get_info<long, CURLINFO_HEADER_SIZE>(handle);
}

auto request_size(easy_handle_t handle)
{
    return get_info<long, CURLINFO_REQUEST_SIZE>(handle);
}

auto ssl_verify_result(easy_handle_t handle)
{
    return get_info<long, CURLINFO_SSL_VERIFYRESULT>(handle);
}

auto content_length_download(easy_handle_t handle)
{
    return get_info<double, CURLINFO_CONTENT_LENGTH_DOWNLOAD>(handle);
}

auto content_length_upload(easy_handle_t handle)
{
    return get_info<double, CURLINFO_CONTENT_LENGTH_UPLOAD>(handle);
}

auto content_type(easy_handle_t handle)
{
    return get_info<const char*, CURLINFO_CONTENT_TYPE>(handle);
}

auto http_auth_avail(easy_handle_t handle)
{
    return get_info<long, CURLINFO_HTTPAUTH_AVAIL>(handle);
}

#if (LIBCURL_VERSION_MAJOR == 7) && (LIBCURL_VERSION_MINOR >= 52)
auto protocol(easy_handle_t handle)
{
    return get_info<long, CURLINFO_PROTOCOL>(handle);
}
#endif

auto primary_ip(easy_handle_t handle)
{
    return get_info<const char*, CURLINFO_PRIMARY_IP>(handle);
}

} // namespace curl
} // namespace btpro
