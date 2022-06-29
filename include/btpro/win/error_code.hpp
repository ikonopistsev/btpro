#pragma once

#include "btpro/win/error_category.hpp"

namespace btpro {
namespace win {

static inline int error() noexcept
{
    return ::GetLastError();
}

static inline std::error_code error_code(int code = error()) noexcept
{
    return std::error_code(code, error_category());
}

} // namespace win
} // namespace btpro
