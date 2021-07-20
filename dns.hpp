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

typedef evdns_base* dns_handle_t;

class dns
{
public:
    typedef dns_handle_t handle_t;

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

public:
    dns() = default;

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

    void create(queue_handle_t hqueue, int opt = -1)
    {
        assert(hqueue);
        assert(empty());

        auto hdns = evdns_base_new(hqueue, opt);
        if (!hdns)
            throw std::runtime_error("evdns_base_new");

        hdns_ = hdns;
        randomize_case("0");
    }

    dns& set(const char *key, const char *val)
    {
        assert(key && val);
        assert((key[0] != '\0') && (val[0] != '\0'));

        auto result = evdns_base_set_option(assert_handle(), key, val);
        if (result == code::fail)
            throw std::runtime_error("evdns_base_set_option");
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
