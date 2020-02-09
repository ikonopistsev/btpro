#pragma once

#include "btpro/dns.hpp"
#include "btpro/tcp/bev.hpp"

namespace btpro {
namespace tcp {

template<std::size_t BEV_OPT_DEF = BEV_OPT_CLOSE_ON_FREE>
class bevconn
{
public:
    typedef bev::handle_t handle_t;

private:
    queue& queue_;
    dns* dns_ = nullptr;

    enum {
        bev_opt = BEV_OPT_DEF
    };

    handle_t create_bufferevent(btpro::socket sock)
    {
        return bufferevent_socket_new(queue_.handle(), sock.fd(), bev_opt);
    }

public:

    connector(queue& queue)
        : queue_(queue)
    {   }

    connector(queue& queue, dns& dns)
        : queue_(queue)
        , dns_(&dns)
    {   }

    bev create(socket sock = socket())
    {
        auto handle = create_bufferevent(sock);
        if (!handle)
            throw std::runtime_error("bufferevent_socket_new");

        return bev(handle);
    }

    void shutdown(handle_t)
    {   }

    void shutdown(bev&)
    {   }
};

} // namespace tcp
} // namespace btpro
