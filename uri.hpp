#pragma once

#include "btpro/btpro.hpp"

namespace btpro {
namespace detail {

template <class T>
class uri_data
{
public:
    typedef T string_type;
    typedef uri_data<T> this_type;
    typedef this_type value_type;

    string_type scheme_{};
    string_type user_{};
    string_type password_{};
    string_type host_{};
    string_type port_{};
    string_type path_{};
    string_type query_{};
    string_type fragment_{};

    uri_data() = default;

    this_type& that() noexcept
    {
        return *this;
    }

    const this_type& that() const noexcept
    {
        return *this;
    }
};

template <class T>
class data_ptr
{
public:
    typedef T value_type;
    typedef typename T::string_type string_type;
    typedef data_ptr<T> this_type;

private:
    std::unique_ptr<value_type> handle_{ std::make_unique<value_type>() };

public:
    data_ptr() = default;

    value_type& that() noexcept
    {
        return *handle_;
    }

    const value_type& that() const noexcept
    {
        return *handle_;
    }
};

} // namespace detail

template<class T>
class uri
{
public:
    typedef typename T::string_type string_type;
    typedef typename T::value_type value_type;

private:
    T data_{};

    template<class String>
    void assign(string_type& value, const String& other)
    {
        value.clear();
        value += other;
    }

    string_type build(const string_type& p) const noexcept
    {
        string_type result;
        auto h = host();
        if (h.empty())
            return  result;

        auto& s = scheme();
        if (!s.empty())
        {
            static const char sep[] = "://";
            result += s;
            result += std::cref(sep);
        }

        auto& u = user();
        if (!u.empty())
        {
            result += u;
            if (!p.empty())
            {
                result += ':';
                result += p;
            }
            result += '@';
        }

        result += h;

        auto& n = port();
        if (!n.empty())
        {
            result += ':';
            result += n;
        }

        auto& l = path();
        auto& q = query();
        auto& f = fragment();
        if (l.empty())
        {
            if (q.empty() && f.empty())
                return result;

            result += '/';
        }
        else
        {
            auto c = l.front();
            if (!((c == '/') || (c == '\\')))
                result += '/';
        }

        if (!q.empty())
        {
            result += '?';
            result += q;
        }

        if (!f.empty())
        {
            result += '#';
            result += f;
        }

        return result;
    }

public:
    value_type& that() noexcept
    {
        return data_.that();
    }

    const value_type& that() const noexcept
    {
        return data_.that();
    }

    uri() = default;

    uri(uri&) = default;
    uri(uri&&) = default;
    uri& operator=(uri&) = default;
    uri& operator=(uri&&) = default;

    template<class P>
    uri(const uri<P>& other)
    {
        auto& l = that();
        auto& r = other.that();
        assign(l.scheme_, r.scheme_);
        assign(l.user_, r.user_);
        assign(l.password_, r.password_);
        assign(l.host_, r.host_);
        assign(l.port_, r.port_);
        assign(l.path_, r.path_);
        assign(l.query_, r.query_);
        assign(l.fragment_, r.fragment_);
    }

    template<class P>
    uri& operator=(const uri<P>& other)
    {
        auto& l = that();
        auto& r = other.that();
        assign(l.scheme_, r.scheme_);
        assign(l.user_, r.user_);
        assign(l.password_, r.password_);
        assign(l.host_, r.host_);
        assign(l.port_, r.port_);
        assign(l.path_, r.path_);
        assign(l.query_, r.query_);
        assign(l.fragment_, r.fragment_);
        return *this;
    }

    void set_scheme(const char* value)
    {
        assert(value);
        that().scheme_ = value;
    }

    template<class String>
    void set_scheme(const String& scheme)
    {
        assign(that().scheme_, scheme);
    }

    void set_user(const char* value)
    {
        assert(value);
        that().user_ = value;
    }

    template<class String>
    void set_user(const String& user)
    {
        assign(that().user_, user);
    }

    void set_password(const char* value)
    {
        assert(value);
        that().password_ = value;
    }

    template<class String>
    void set_password(const String& password)
    {
        assign(that().password_, password);
    }

    void set_host(const char* value)
    {
        assert(value);
        that().host_ = value;
    }

    template<class String>
    void set_host(const String& host)
    {
        assign(that().host_, host);
    }

    void set_port(const char* value)
    {
        assert(value);
        that().port_ = value;
    }

    template<class String>
    void set_port(const String& port)
    {
        assign(that().port_, port);
    }

    void set_path(const char* value)
    {
        assert(value);
        that().path_ = value;
    }

    template<class String>
    void set_path(const String& path)
    {
        assign(that().path_, path);
    }

    void set_query(const char* value)
    {
        assert(value);
        that().query_ = value;
    }

    template<class String>
    void set_query(const String& query)
    {
        assign(that().query_, query);
    }

    void set_fragment(const char* value)
    {
        assert(value);
        that().fragment_ = value;
    }

    template<class String>
    void set_fragment(const String& fragment)
    {
        assign(that().fragment_, fragment);
    }

    const string_type& scheme() const noexcept
    {
        return that().scheme_;
    }

    const string_type& user() const noexcept
    {
        return that().user_;
    }

    const string_type& password() const noexcept
    {
        return that().password_;
    }

    const string_type& host() const noexcept
    {
        return that().host_;
    }

    const string_type& port() const noexcept
    {
        return that().port_;
    }

    int dport() const noexcept
    {
        return std::atoi(port().that());
    }

    const string_type& path() const noexcept
    {
        return that().path_;
    }

    const string_type& query() const noexcept
    {
        return that().query_;
    }

    const string_type& fragment() const noexcept
    {
        return that().fragment_;
    }

    string_type str() const
    {
        return build(password());
    }

    string_type str_safe() const
    {
        auto& p = password();
        if (!p.empty())
        {
            static const string_type stars("***");
            return build(stars);
        }
        return build(p);
    }
};

} // namespace btpro
