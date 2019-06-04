#pragma once

#include <system_error>

namespace btpro {
namespace posix {

static inline int error() noexcept
{
    return errno;
}

static inline std::error_code error_code(int code = error()) noexcept
{
    return std::error_code(code, std::generic_category());
}

} // namespace posix
} // namespace btpro
