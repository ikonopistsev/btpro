#pragma once

#include "btpro/queue.hpp"
#include "event2/dns.h"

namespace btpro {

#ifdef EVDNS_BASE_INITIALIZE_NAMESERVERS
static const auto dns_initialize_nameservers = int{
    EVDNS_BASE_INITIALIZE_NAMESERVERS
};
#endif

#ifdef EVDNS_BASE_DISABLE_WHEN_INACTIVE
static const auto dns_disable_incative = int{
    EVDNS_BASE_DISABLE_WHEN_INACTIVE
};
#endif

using dns_handle_t = evdns_base*;

class dns
{
public:
    using handle_t = dns_handle_t;

private:

    static inline void destroy(dns_handle_t value) noexcept
    {
        if (nullptr != value)
            evdns_base_free(value, DNS_ERR_SHUTDOWN);
    };

    handle_t hdns_{nullptr};

    handle_t assert_handle() const noexcept
    {
        auto hqueue = handle();
        assert(hqueue);
        return hqueue;
    }

    handle_t create(queue_handle_t hqueue, int opt = -1)
    {
        assert(hqueue);
        assert(empty());

        auto hdns = evdns_base_new(hqueue, opt);
        if (!hdns)
            throw std::runtime_error("evdns_base_new");

        return hdns;
        //randomize_case("0");
    }

public:
    explicit dns(handle_t hdns)
        : hdns_{hdns}
    {
        assert(hdns);
        randomize_case("0");
    }

    dns() 
        : dns(create())
    {   }

    ~dns() noexcept
    {
        destroy(hdns_);
    }

    dns(dns&& that) noexcept
    {
        std::swap(hdns_, that.hdns_);
    }

    dns& operator=(dns&& that) noexcept
    {
        std::swap(hdns_, that.hdns_);
        return *this;
    }

    dns(const dns& other) = delete;
    dns& operator=(const dns& other) = delete;

    void assign(handle_t hdns) noexcept
    {
        assert(hdns);
        hdns_ = hdns;
    }

    handle_t handle() const noexcept
    {
        return hdns_;
    }

    operator handle_t() const noexcept
    {
        return handle();
    }

    bool empty() const noexcept
    {
        return nullptr == handle();
    }

    dns& set(const char *key, const char *val)
    {
        assert(key && val);
        assert((key[0] != '\0') && (val[0] != '\0'));

        detail::check_result("evdns_base_set_option",
            evdns_base_set_option(assert_handle(), key, val));
        return *this;
    }

    dns& randomize_case(const char *val)
    {
        return set("randomize-case", val);
    }

    dns& timeout(const char *val)
    {
        return set("timeout", val);
    }

    dns& max_timeouts(const char *val)
    {
        return set("max-timeouts", val);
    }
};

} // namespace btpro
