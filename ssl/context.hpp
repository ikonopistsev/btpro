#pragma once

#include "btpro/btpro.hpp"

#include <openssl/err.h>
#include <openssl/ssl.h>

#include <memory>
#include <stdexcept>

namespace btpro {
namespace ssl {

using handle_t = SSL_CTX*;

namespace detail {

template<class R>
struct context_destroy;

template<>
struct context_destroy<tag_ref>
{
    static constexpr inline void destroy_handle(handle_t) noexcept
    {   }
};

template<>
struct context_destroy<tag_obj>
{
    static inline void destroy_handle(handle_t ctx) noexcept
    {
        if (nullptr != ctx)
            SSL_CTX_free(ctx);
    }
};

} // detail

template<class R>
class basic_context;

typedef basic_context<tag_ref> context_ref;
typedef basic_context<tag_obj> context;

template<class R>
class basic_context
{
public:
    static constexpr bool is_ref = R::is_ref;

private:
    handle_t ctx_{ nullptr };

    handle_t assert_handle() const noexcept
    {
        auto hqueue = handle();
        assert(hqueue);
        return hqueue;
    }

public:
    basic_context() = default;

    ~basic_context() noexcept
    {
        detail::context_destroy<R>::destroy_handle(ctx_);
    }

    basic_context(basic_context&& that) noexcept
    {
        std::swap(ctx_, that.ctx_);
    }

    basic_context(handle_t ctx) noexcept
        : ctx_(ctx)
    {
        assert(ctx);
        static_assert(is_ref, "context_ref only");
    }

    basic_context(const context& other) noexcept
        : basic_context(other.handle())
    {   }

    basic_context(const context_ref& other) noexcept
        : basic_context(other.handle())
    {   }

    basic_context& operator=(basic_context&& that) noexcept
    {
        std::swap(ctx_, that.ctx_);
        return *this;
    }

    void assign(handle_t ctx) noexcept
    {
        assert(ctx);
        ctx_ = ctx;
    }

    context_ref& operator=(handle_t ctx) noexcept
    {
        assign(ctx);
        return *this;
    }

    context_ref& operator=(const context& other) noexcept
    {
        assign(other.handle());
        return *this;
    }

    context_ref& operator=(const context_ref& other) noexcept
    {
        assign(other.handle());
        return *this;
    }

    handle_t handle() const noexcept
    {
        return ctx_;
    }

    operator handle_t() const noexcept
    {
        return handle();
    }

    bool empty() const noexcept
    {
        return nullptr == handle();
    }

    void destroy() noexcept
    {
        detail::context_destroy<R>::destroy_handle(ctx_);
        ctx_ = nullptr;
    }

    void create_client()
    {
        static_assert(!is_ref, "no context_ref");

        assert(empty());

        auto ctx = SSL_CTX_new(TLS_client_method());
        if (!ctx)
            throw std::runtime_error("TLS_client_method");

        ctx_ = ctx;
    }

    void create_server(const config& conf)
    {
        static_assert(!is_ref, "no context_ref");

        assert(empty());

        auto ctx = SSL_CTX_new(TLS_server_method());
        if (!ctx)
            throw std::runtime_error("TLS_server_method");

        ctx_ = ctx;
    }

    // https://www.openssl.org/docs/man1.0.2/man3/SSL_CTX_load_verify_locations.html
    void load_verify_locations(const char *ca_file, const char *ca_path)
    {
        if (!SSL_CTX_load_verify_locations(assert_handle(), ca_file, ca_path))
            throw std::runtime_error("SSL_CTX_load_verify_locations");
    }

    // Currently supported versions are SSL3_VERSION,
    // TLS1_VERSION, TLS1_1_VERSION, TLS1_2_VERSION for TLS
    // and DTLS1_VERSION, DTLS1_2_VERSION for DTLS.
    void set_min_proto_version(int version)
    {
        if (!SSL_CTX_set_min_proto_version(assert_handle(), version))
            throw std::runtime_error("SSL_CTX_set_min_proto_version");
    }

    void set_options(long options)
    {
        if (!SSL_CTX_set_options(assert_handle(), options))
            throw std::runtime_error("SSL_CTX_set_min_proto_version");
    }

// unsigned char vector[] = {
//     6, 's', 'p', 'd', 'y', '/', '1',
//     8, 'h', 't', 't', 'p', '/', '1', '.', '1'
//     2, 'h', '2'
// };
// unsigned int length = sizeof(vector);
// SSL_CTX_set_alpn_protos(handle, vector, length);

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

} // namepsace ssl
} // namespace btpro
