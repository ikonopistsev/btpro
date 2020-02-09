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

typedef evdns_base* dns_handle_t;

namespace detail {

template<class R>
struct dns_destroy;

template<>
struct dns_destroy<tag_ref>
{
    constexpr static inline void destroy_handle(dns_handle_t) noexcept
    {   }
};

template<>
struct dns_destroy<tag_obj>
{
    static inline void destroy_handle(dns_handle_t value) noexcept
    {
        if (nullptr != value)
            evdns_base_free(value, DNS_ERR_SHUTDOWN);
    }
};

} // detail

template<class R>
class basic_dns;

typedef basic_dns<tag_ref> dns_ref;
typedef basic_dns<tag_obj> dns;

template<class R>
class basic_dns
{
public:
    typedef dns_handle_t handle_t;
    constexpr static bool is_ref = R::is_ref;

private:
    handle_t hdns_{nullptr};

    handle_t assert_handle() const noexcept
    {
        auto hqueue = handle();
        assert(hqueue);
        return hqueue;
    }

public:
    basic_dns() = default;

    ~basic_dns() noexcept
    {
        detail::dns_destroy<R>::destroy_handle(hdns_);
    }

    basic_dns(basic_dns&& that) noexcept
    {
        std::swap(hdns_, that.hdns_);
    }

    basic_dns& operator=(basic_dns&& that) noexcept
    {
        std::swap(hdns_, that.hdns_);
        return *this;
    }

    basic_dns(handle_t hdns) noexcept
        : hdns_(hdns)
    {
        assert(hdns);
        static_assert(is_ref, "dns_ref only");
    }

    dns_ref& operator=(handle_t hdns) noexcept
    {
        assert(hdns);
        hdns_ = hdns;
        return *this;
    }

    template<class T>
    basic_dns(const basic_queue<T>& other) noexcept
        : basic_dns(other.handle())
    {   }

    template<class T>
    dns_ref& operator=(const basic_dns<T>& other) noexcept
    {
        *this = other.hdns_;
        return *this;
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
        static_assert(!is_ref, "no dns_ref");

        auto hdns = evdns_base_new(hqueue, opt);
        if (!hdns)
            throw std::runtime_error("evdns_base_new");

        hdns_ = hdns;
        randomize_case("0");
    }

    basic_dns& set(const char *key, const char *val)
    {
        assert(key && val);
        assert((key[0] != '\0') && (val[0] != '\0'));

        auto result = evdns_base_set_option(assert_handle(), key, val);
        if (result == code::fail)
            throw std::runtime_error("evdns_base_set_option");
        return *this;
    }

    basic_dns& randomize_case(const char *val)
    {
        return set("randomize-case", val);
    }

    basic_dns& timeout(const char *val)
    {
        return set("timeout", val);
    }

    basic_dns& max_timeouts(const char *val)
    {
        return set("max-timeouts", val);
    }
};

} // namespace btpro
