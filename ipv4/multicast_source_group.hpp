#pragma once

#include "btpro/sock_opt.hpp"
#include "btpro/ipv4/addr.hpp"

namespace btpro {
namespace ip {
namespace v4 {
namespace multicast_source_group {

namespace detail {
    static const char add_membership[] = "IP_ADD_SOURCE_MEMBERSHIP";
    static const char drop_membership[] = "IP_DROP_SOURCE_MEMBERSHIP";

    typedef sock_basic_option<ip_mreq_source,
        IPPROTO_IP, IP_ADD_SOURCE_MEMBERSHIP> add_type;
    typedef sock_basic_option<ip_mreq_source,
        IPPROTO_IP, IP_DROP_SOURCE_MEMBERSHIP> drop_type;

    static inline ip_mreq_source create_ip_mreq_source(const addr& group,
        const addr& src, const addr& iface) noexcept
    {
        ip_mreq_source res;
        res.imr_multiaddr.s_addr = group->sin_addr.s_addr;
        res.imr_interface.s_addr = iface->sin_addr.s_addr;
        res.imr_sourceaddr.s_addr = src->sin_addr.s_addr;
        return res;
    }
} // namespace detail

static inline detail::add_type join(const addr& group,
    const addr& src, const addr& iface = any()) noexcept
{
    return detail::add_type(detail::create_ip_mreq_source(group, src, iface),
        detail::add_membership);
}

static inline detail::drop_type leave(const addr& group,
    const addr& src, const addr& iface = any()) noexcept
{
    return detail::drop_type(detail::create_ip_mreq_source(group, src, iface),
        detail::drop_membership);
}

} // namespace multicast_source_group
} // namespace v4
} // namespace ip
} // namespace btpro


