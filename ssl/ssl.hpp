#pragma once

#include "btpro/btpro.hpp"

#include <openssl/err.h>
#include <openssl/ssl.h>

#include <memory>
#include <stdexcept>

namespace btpro {
namespace ssl {

class openssl_library
{
public:
    openssl_library() noexcept
    {
        OpenSSL_add_all_algorithms();
        SSL_load_error_strings();
        SSL_library_init();
    }

    static void init() noexcept
    {
        static const openssl_library l;
    }
};

class openssl
{
public:
    typedef SSL_CTX* handle_t;

private:

    std::unique_ptr<SSL_CTX, decltype(&SSL_CTX_free)> handle_;

    openssl() = delete;
    openssl(const openssl&) = delete;
    openssl& operator=(const openssl& other) = delete;

public:
    openssl(handle_t handle) noexcept
        : handle_(handle, SSL_CTX_free)
    {   }

    openssl(openssl&&) = default;
    openssl& operator=(openssl&& other) = default;

    static void set_opt(handle_t handle, int opt) noexcept
    {
        SSL_CTX_set_options(handle, opt|SSL_OP_NO_SSLv2);
    }

    handle_t handle() const noexcept
    {
        auto handle = handle_.get();
        assert(handle);
        return handle;;
    }

    void set_cipher(const std::string& value)
    {
        SSL_CTX_set_cipher_list(handle(), value.c_str());
    }

    void set_dhparams(const std::string& file)
    {
        FILE *fh = nullptr;

#ifdef WIN32
        auto err = fopen_s(&fh, file.c_str(), "r");
        if (err)
            throw std::runtime_error("set_dhparams");
#else
        fh = fopen(file.c_str(), "r");
        if (!fh)
            throw std::runtime_error("set_dhparams");
#endif // WIN32

        DH *dh = PEM_read_DHparams(fh, 0, 0, 0);
        if (dh)
            SSL_CTX_set_tmp_dh(handle(), dh);

        fclose(fh);

        if (dh)
            DH_free(dh);
        else
            throw std::runtime_error("PEM_read_DHparams");
    }

    void set_cert(const std::string& file)
    {
        if (!SSL_CTX_use_certificate_file(handle(), file.c_str(), SSL_FILETYPE_PEM))
            throw std::runtime_error("SSL_CTX_use_certificate_file");
    }

    void set_pk(const std::string& file)
    {
        if (!SSL_CTX_use_PrivateKey_file(handle(), file.c_str(), SSL_FILETYPE_PEM))
            throw std::runtime_error("SSL_CTX_use_certificate_file");

        if (!SSL_CTX_check_private_key(handle()))
            throw std::runtime_error("SSL_CTX_check_private_key");
    }
};

static inline openssl client(int opt = 0)
{
    openssl_library::init();

    auto handle = SSL_CTX_new(SSLv23_client_method());
    if (!handle)
        throw std::runtime_error("SSLv23_client_method");

    openssl::set_opt(handle, opt|SSL_OP_NO_SSLv2);

    return openssl(handle);
}

static inline openssl server(int opt = 0)
{
    openssl_library::init();

    auto handle = SSL_CTX_new(SSLv23_server_method());
    if (!handle)
        throw std::runtime_error("SSLv23_server_method");

    openssl::set_opt(handle, opt|SSL_OP_NO_SSLv2);

    return openssl(handle);
}

} // namepsace ssl
} // namespace btpro
