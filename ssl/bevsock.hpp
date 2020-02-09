#pragma once

#include "btpro/dns.hpp"
#include "btpro/tcp/bev.hpp"
#include "btpro/ssl/ssl.hpp"
#include "event2/bufferevent_ssl.h"
#include <openssl/ssl.h>
#include "memory"

namespace btpro {
namespace ssl {

template<bufferevent_ssl_state BEV_SSL_STATE,
         int BEV_OPT_DEF = BEV_OPT_CLOSE_ON_FREE>
class connector
{
public:
    typedef tcp::bev::handle_t handle_t;

private:
    openssl& ssl_;
    queue& queue_;
    dns& dns_;

    handle_t create_bufferevent(btpro::socket sock)
    {
        auto ssl = SSL_new(ssl_.handle());
        if (!ssl)
            throw std::runtime_error("SSL_new");

        auto handle = bufferevent_openssl_socket_new(queue_.handle(),
            sock.fd(), ssl, BEV_SSL_STATE, BEV_OPT_DEF);

        // если не создали сокет убиваем ssl
        if (!handle)
            SSL_shutdown(ssl);

        return handle;
    }

public:
    connector(openssl& ssl, queue& queue)
        : ssl_(ssl)
        , queue_(queue)
    {   }

    connector(openssl& ssl, queue& queue, dns& dns)
        : ssl_(ssl)
        , queue_(queue)
        , dns_(dns)
    {   }

    tcp::bev create(socket sock = socket())
    {
        auto handle = create_bufferevent(sock);
        if (!handle)
            throw std::runtime_error("bufferevent_socket_new");

        return tcp::bev(tcp::bev::value_type(handle, bufferevent_free));
    }

    void connect(tcp::bev& bev, const std::string& hostname, int port)
    {
        bev.connect(dns_, hostname, port);
    }

    void shutdown(handle_t handle)
    {
        auto ssl = bufferevent_openssl_get_ssl(handle);
        if (ssl)
        {
            SSL_set_shutdown(ssl, SSL_RECEIVED_SHUTDOWN);
            SSL_shutdown(ssl);
        }
    }

    void shutdown(tcp::bev& bev)
    {
        shutdown(bev.handle());
    }
};

} // namespace ssl
} // namespace btpro
