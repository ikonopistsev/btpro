#pragma once

#include "btpro/curl/curl.hpp"

#include <string_view>
#include <string>

namespace btpro {
namespace curl {

template<long Opt>
class basic_slist
{
    curl_slist* slist_{nullptr};

    static curl_slist* make(curl_slist* slist, const std::string& text)
    {
        return curl_slist_append(slist, text.c_str());
    }

    static curl_slist* make(curl_slist* slist,
        std::string_view key, std::string_view val)
    {
        std::string text;
        text.reserve(key.size() + val.size() + 4);
        text += key;
        text += ':';
        text += ' ';
        text += val;
        return make(slist, text);
    }

public:
    basic_slist() = default;

    basic_slist(std::string_view key, std::string_view val)
        : slist_(make(slist_, key, val))
    {   }

    ~basic_slist() noexcept
    {
        reset();
    }

    void push(std::string_view key, std::string_view val)
    {
        slist_ = make(slist_, key, val);
    }

    void push_header(std::string_view value)
    {
        slist_ = curl_slist_append(slist_, value.data());
    }

    bool empty() const noexcept
    {
        return slist_ == nullptr;
    }

    void reset() noexcept
    {
        if (slist_)
        {
            curl_slist_free_all(slist_);
            slist_ = nullptr;
        }
    }

    void operator()(easy_handle_t handle) const
    {
        assert(handle);
        // можно выставлять nullptr
        set_opt(handle, static_cast<CURLoption>(Opt), slist_);
    }
};

} // namsspace curl
} // namespace btpro
