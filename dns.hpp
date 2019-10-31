#pragma once

#include "btpro/queue.hpp"
#include "event2/dns.h"

namespace btpro {

static const auto dns_initialize_nameservers = int{
    EVDNS_BASE_INITIALIZE_NAMESERVERS
};

static const auto dns_disable_incative = int{
    EVDNS_BASE_DISABLE_WHEN_INACTIVE
};

class dns
{
public:
    typedef evdns_base* handle_t;

private:
    static void clear(handle_t handle) noexcept
    {
        evdns_base_free(handle, DNS_ERR_SHUTDOWN);
    }

private:
    static inline handle_t create(queue& queue, int opt)
    {
        auto res = evdns_base_new(queue.handle(), opt);
        if (!res)
            throw std::runtime_error("evdns_base_new");
        return res;
    }

    handle_t assert_handle() const noexcept
    {
        auto res = handle_.get();
        assert(res);
        return res;
    }

    std::unique_ptr<evdns_base, decltype(&clear)> handle_;

public:
    dns() = delete;
    dns(dns&) = delete;
    dns& operator=(dns&) = delete;

    explicit dns(queue& queue)
        : handle_(create(queue, -1), &clear)
    {
        randomize_case("0");
    }

    explicit dns(queue& queue, int opt)
        : handle_(create(queue, opt), &clear)
    {
        randomize_case("0");
    }

    dns& set(const char *key, const char *val)
    {
        assert(key && val);
        assert(key[0] != '\0');
        assert(val[0] != '\0');
        auto result = evdns_base_set_option(handle(), key, val);
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

    handle_t handle() const noexcept
    {
        return assert_handle();
    }
};

} // namespace btpro
