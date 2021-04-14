#pragma once

#include "btpro/dns.hpp"
#include "btpro/tcp/bev.hpp"
#include "btpro/ssl/context.hpp"
#include "event2/bufferevent_ssl.h"
#include <openssl/ssl.h>
#include <openssl/err.h>

namespace btpro {
namespace ssl {

class bevtls
{
public:
    using bev = tcp::bev;

private:
    bev& bev_;

public:
    bevtls(bev& bev) noexcept
        : bev_(bev)
    {   }

    void create(handle_t ctx, queue_ref queue, socket sock,
                bufferevent_ssl_state state, int options)
    {
        assert(ctx);

        auto ssl = SSL_new(ctx);
        if (!ssl)
            throw std::runtime_error("SSL_new");

        auto hbev = bufferevent_openssl_socket_new(queue,
            sock.fd(), ssl, state, options);
        // если не создали сокет убиваем ssl
        if (!hbev)
        {
            SSL_shutdown(ssl);
            throw std::runtime_error("bufferevent_openssl_socket_new");
        }

        bev_.destroy();
        bev_.attach(hbev);
    }

    void destory()
    {
        shutdown();
        bev_.destroy();
    }

    unsigned long get_openssl_error()
    {
        return bufferevent_get_openssl_error(bev_);
    }

    std::string get_openssl_error_string(unsigned long err)
    {
        std::string rc;
        rc.resize(256);

        ERR_error_string_n(err, rc.data(), rc.size());
        rc.resize(std::strlen(rc.data()));

        return rc;
    }

    void connect(dns_ref dns, const std::string& hostname, int port)
    {
        auto ssl = bufferevent_openssl_get_ssl(bev_);
        if (!ssl)
            throw std::runtime_error("bufferevent_openssl_get_ssl");

        // для tls надо установить имя хоста к которому подключаемся
        auto ret = SSL_set_tlsext_host_name(ssl, hostname.c_str());
        if (!ret)
            throw std::runtime_error("SSL_set_tlsext_host_name");

        bev_.connect(dns, hostname, port);
    }

    void shutdown() noexcept
    {
        auto ssl = bufferevent_openssl_get_ssl(bev_);
        if (ssl)
        {
            SSL_set_shutdown(ssl, SSL_RECEIVED_SHUTDOWN);
            SSL_shutdown(ssl);
        }
        else
        {
            // FIXME add alert
        }
    }
};

} // namespace ssl
} // namespace btpro
