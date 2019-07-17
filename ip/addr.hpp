#pragma once

#include "btpro/btpro.hpp"

namespace btpro {
namespace ip {

class addr
{
public:
    typedef decltype(sockaddr::sa_family) family_type;

private:
    sockaddr* sockaddr_{nullptr};
    ev_socklen_t socklen_{};

protected:
    addr(sockaddr* sockaddr) noexcept
        : sockaddr_(sockaddr)
    {   
        assert(sockaddr);
    }

    addr(sockaddr* sockaddr, ev_socklen_t socklen) noexcept
        : sockaddr_(sockaddr)
        , socklen_(socklen)
    {   
        assert(sockaddr && (socklen > 0));
    }

    void set_socklen(ev_socklen_t socklen) noexcept
    {
        socklen_ = socklen;
    }

public:
    addr() = delete;
    addr(const addr&) = default;
    addr& operator=(const addr&) = default;

    static addr create(sockaddr* sockaddr, ev_socklen_t socklen) noexcept
    {
        return addr(sockaddr, socklen);
    }

    family_type family() const noexcept
    {
        return sa()->sa_family;
    }

    sockaddr* sa() noexcept
    {
        assert(sockaddr_);
        return sockaddr_;
    }

    const sockaddr* sa() const noexcept
    {
        assert(sockaddr_);
        return sockaddr_;
    }

    ev_socklen_t size() const noexcept
    {
        return socklen_;
    }

    sockaddr* operator->() noexcept
    {
        assert(sockaddr_);
        return sockaddr_;
    }

    const sockaddr* operator->() const noexcept
    {
        assert(sockaddr_);
        return sockaddr_;
    }

    int compare(const addr& other) const noexcept
    {
        int r = static_cast<int>(family()) - static_cast<int>(other.family());
        if (!r)
        {
            std::size_t sz = static_cast<std::size_t>(size());
            int s = static_cast<int>(sz) - static_cast<int>(other.size());
            if (!s)
                return std::memcmp(sa(), other.sa(), sz);
            return s;
        }
        return r;
    }

    bool operator<(const addr& other) const noexcept
    {
        return compare(other) < 0;
    }

    bool operator==(const addr& other) const noexcept
    {
        return compare(other) == 0;
    }

    bool operator!=(const addr& other) const noexcept
    {
        return !(*this == other);
    }
};

} // namespace ip
} // namespace btpro
