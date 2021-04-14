#pragma once

#include "btpro/curl/curl.hpp"
#include <functional>

namespace btpro {
namespace curl {
namespace responce {

class base
{
public:
    using error_fn_type = std::function<void(std::exception_ptr)>;

protected:
    error_fn_type error_fn_{};

public:
    virtual void set(error_fn_type fn)
    {
        error_fn_ = std::move(fn);
    }

    virtual void assign(easy_handle_t) = 0;

    virtual void done(easy_handle_t, CURLcode) noexcept = 0;

    virtual void call(std::exception_ptr ex) noexcept
    {
        if (error_fn_)
            error_fn_(ex);
    }

    // является ли вебсокетом
    virtual bool stream() noexcept
    {
        return false;
    }
};

} // namespace
} // namespace
} // namespace
