#pragma once

#include <cctype>
#include <limits>
#include <algorithm>
#include <unordered_map>
#include <string_view>

namespace btpro {
namespace curl {
namespace header {

template<class K, class V>
class basic_store
{
public:
    using hash_type = std::uint64_t;
    using value_type = typename std::pair<K, V>;
    using store_type = std::unordered_multimap<hash_type, value_type>;
    using iterator = typename store_type::iterator;
    using const_iterator = typename store_type::const_iterator;

private:
    store_type store_{};


public:
    constexpr
    static inline char tolower(char c) noexcept
    {
        return ((c > 'A') && (c < 'Z')) ? c + ('a' - 'A') : c;
    }

    constexpr
    static hash_type calc_hash(const char *p, const char *e) noexcept
    {
        hash_type hval = 0xcbf29ce484222325ull;
        while (p < e)
        {
            hval ^= static_cast<hash_type>(tolower(*p++));
            hval += (hval << 1) + (hval << 4) + (hval << 5) +
                (hval << 7) + (hval << 8) + (hval << 40);
        }
        return hval;
    }

    constexpr
    static hash_type calc_hash(std::string_view val) noexcept
    {
        return calc_hash(val.begin(), val.end());
    }

    basic_store() = default;

    template<class I>
    static auto& key(I iter) noexcept
    {
        auto& kv = std::get<1>(*iter);
        return std::get<0>(kv);
    }

    template<class I>
    static auto& value(I iter) noexcept
    {
        auto& kv = std::get<1>(*iter);
        return std::get<1>(kv);
    }

    constexpr
    static auto to_hash(std::string_view key) noexcept
    {
        assert(!key.empty());

        return calc_hash(key);
    }

    constexpr
    auto find(std::string_view key)
    {
        return find(to_hash(key));
    }

    constexpr
    auto find(std::size_t key_hash)
    {
        return store_.find(key_hash);
    }

    constexpr
    auto find(std::string_view key) const
    {
        return find(to_hash(key));
    }

    constexpr
    auto find(std::size_t key_hash) const
    {
        return store_.find(key_hash);
    }

    void insert(std::string_view key, std::string_view value)
    {
        assert(!key.empty());
        assert(!value.empty());

        auto h = to_hash(key);
        auto f = find(h);
        if (f != store_.cend())
        {
            auto& key_hash = f->first;
            auto& curr_value = std::get<1>(f->second);
            if (curr_value != value)
            {
                if (!curr_value.empty())
                {
                    store_.insert(f, std::make_pair(key_hash,
                        std::make_pair(K(key.data(), key.size()),
                            V(value.data(), value.size()))));
                }
                else
                    curr_value.assign(value.data(), value.size());
            }
        }
        else
        {
            store_.insert(std::make_pair(h,
                std::make_pair(K(key.data(), key.size()),
                    V(value.data(), value.size()))));
        }
    }

    constexpr
    std::string_view get(std::string_view key) const noexcept
    {
        return get_first(key);
    }

    constexpr
    std::string_view get_first(std::string_view key) const noexcept
    {
        auto f = find(key);
        return (f != store_.end()) ?
            std::get<1>(f->second) : std::string_view();
    }

    void clear_value() noexcept
    {
        auto i = store_.begin(),
             e = store_.end();
        while (i != e)
            value(i++).clear();
    }

    const_iterator begin() const noexcept
    {
        return store_.begin();
    }

    const_iterator end() const noexcept
    {
        return store_.end();
    }

    const_iterator cbegin() const noexcept
    {
        return begin();
    }

    const_iterator cend() const noexcept
    {
        return end();
    }

    auto size() const noexcept
    {
        return store_.size();
    }

    bool empty() const noexcept
    {
        return size() == 0;
    }

    std::string dump() const
    {
        std::string rc;
        rc.reserve(320);

        for (auto& h : *this)
        {
            auto& kv = std::get<1>(h);
            auto& key = std::get<0>(kv);
            auto& value = std::get<1>(kv);
            if (!value.empty())
            {
                if (!rc.empty())
                    rc += '\n';
                rc += key;
                rc += ':';
                rc += value;
            }
        }

        return rc;
    }
};

using store = basic_store<std::string, std::string>;
using store_cref = const store&;
using store_ref = store&;

} // namespace header
} // namespace curl
} // namespace btpro
