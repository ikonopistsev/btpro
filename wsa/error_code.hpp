#pragma once

#include "btpro/win/error_category.hpp"

namespace btpro {
namespace wsa {

static inline int error() noexcept
{
    return ::WSAGetLastError();
}

static inline std::error_code error_code(int code = error()) noexcept
{
    return std::error_code(code, win::error_category());
}

} // namespace wsa
} // namespace btpro
