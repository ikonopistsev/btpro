#pragma once

#include "btpro/btpro.hpp"

namespace btpro {
namespace ip {

class addr
{
public:
    using sa_family = decltype(sockaddr::sa_family);

private:
    sockaddr* sockaddr_{nullptr};
    ev_socklen_t socklen_{};

protected:
    addr(sockaddr* sockaddr) noexcept
        : sockaddr_(sockaddr)
    {   
        assert(sockaddr);
    }

    explicit addr(sockaddr* sockaddr, ev_socklen_t socklen) noexcept
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

    sa_family family() const noexcept
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

/*
    auto operator<=>(const addr& other) const noexcept
    {
        auto r = static_cast<int>(family()) - static_cast<int>(other.family());
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
*/
};

} // namespace ip
} // namespace btpro
