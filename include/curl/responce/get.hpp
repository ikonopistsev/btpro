#pragma once

#include "btpro/curl/responce/base.hpp"
#include "btpro/curl/io/buffer.hpp"
#include "btpro/curl/parser/parser.hpp"

namespace btpro {
namespace curl {
namespace responce {

template<class T>
class get
    : public base
{
public:
    using result_type = T;
    using fn_type = typename T::fn_type;
    using this_type = get<T>;
    using inbuf_type = io::buffer<this_type, io::append>;

    using header_fn_type = std::function<bool(header::store_cref)>;

private:
    fn_type done_{};

    header::store hdr_{};
    header::parser<this_type> hparse_{*this, hdr_};
    header_fn_type header_fn_{};

    inbuf_type inbuf_{*this};

    virtual void done(easy_handle_t easy, CURLcode code) noexcept override
    {
        try
        {
            done_(result_type(easy, code, inbuf_.data(), hdr_));
        }
        catch (...)
        {
            error(std::current_exception());
        }
    }

public:
    get(fn_type fn)
        : done_(std::move(fn))
    {   }

    virtual ~get()
    {   }

    void assign(easy_handle_t easy) override
    {
        assert(easy);
        hparse_.assign(easy);
        inbuf_.assign(easy);
    }

    void set(fn_type fn)
    {
        done_ = std::move(fn);
    }

    void set(header_fn_type fn)
    {
        header_fn_ = std::move(fn);
    }

    bool call(header::store_ref hdr)
    {
        return (header_fn_) ?
            header_fn_(hdr) : true;
    }
};

} // namespace
} // namespace
} // namespace
