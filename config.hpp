#pragma once

#include "btpro/btpro.hpp"
#include <vector>
#include <string>

namespace btpro {

class config
{
public:
    typedef event_config* handle_t;

private:
    static inline handle_t create()
    {
        auto hconf = event_config_new();
        if (!hconf)
            throw std::runtime_error("event_base_new");
        return hconf;
    }

    std::unique_ptr<event_config, decltype(&event_config_free)>
        hconf_{create(), event_config_free};

public:
    config() = default;

    config(config&) = delete;
    config& operator=(config&) = delete;

    explicit config(int flag)
    {
        set_flag(flag);
    }

    void require_features(int value)
    {
        auto res = event_config_require_features(handle(), value);
        if (code::fail == res)
            throw std::runtime_error("event_config_require_features");
    }

    void avoid_method(const std::string& method)
    {
        auto res = event_config_avoid_method(handle(), method.c_str());
        if (code::fail == res)
            throw std::runtime_error("event_config_avoid_method");
    }

    void set_flag(int flag)
    {
        auto res = event_config_set_flag(handle(), flag);
        if (code::fail == res)
            throw std::runtime_error("event_config_set_flag");
    }

    static inline std::vector<std::string> supported_methods()
    {
        std::vector<std::string> res;
        auto method = event_get_supported_methods();
        for (std::size_t i = 0; method[i] != nullptr; ++i)
            res.emplace_back(method[i], strlen(method[i]));

        return res;
    }

    handle_t handle() const noexcept
    {
        return hconf_.get();
    }

    operator handle_t() const noexcept
    {
        return handle();
    }
};

} // namespace btpro

