#pragma once

#include <system_error>

#include "event2/util.h"

namespace btpro {

struct code
{
    static constexpr auto sucsess = int{ 0 };
    static constexpr auto fail = int{ -1 };
};

namespace win {
namespace detail {

class error_category
    : public std::error_category
{
public:
    error_category() = default;

    char const* name() const noexcept
    {
        return "win error";
    }

    std::string message(int code) const
    {
        char buf[4096];
        DWORD len = ::FormatMessageA(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, code, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
            buf, sizeof(buf), NULL);
        if (len)
        {
            while (len && ((buf[len - 1] == '\r') || (buf[len - 1] == '\n')))
                --len;
            return std::string(buf, len);
        }

        return std::string("N/A");
    }
};

} // namespace detail

static inline const detail::error_category& error_category() noexcept
{
    static const detail::error_category ec;
    return ec;
}

} // namespace win
} // namespace btpro
