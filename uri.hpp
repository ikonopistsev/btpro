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

    this_type& self() noexcept
    {
        return *this;
    }

    const this_type& self() const noexcept
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

    value_type& self() noexcept
    {
        return *handle_;
    }

    const value_type& self() const noexcept
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

        auto& s = scheme();
        if (!s.empty())
        {
            static const char sep[] = "://";
            static const std::size_t sep_size = sizeof(sep);

            result.append(s);
            result.append(sep, sep_size);
        }

        auto& u = user();
        if (!u.empty())
        {
            result.append(u);
            if (!p.empty())
            {
                result.append(':');
                result.append(p);
            }
            result.append('@');
        }

        auto h = host();
        if (!h.empty())
        {

        }
    }

public:
    value_type& data() noexcept
    {
        return data_.self();
    }

    const value_type& data() const noexcept
    {
        return data_.self();
    }

    uri() = default;

    uri(uri&) = default;
    uri(uri&&) = default;
    uri& operator=(uri&) = default;
    uri& operator=(uri&&) = default;

    template<class P>
    uri(const uri<P>& other)
    {
        auto& l = data();
        auto& r = other.data();
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
        auto& l = data();
        auto& r = other.data();
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
        data().scheme_ = value;
    }

    template<class String>
    void set_scheme(const String& scheme)
    {
        assign(data().scheme_, scheme);
    }

    void set_user(const char* value)
    {
        assert(value);
        data().user_ = value;
    }

    template<class String>
    void set_user(const String& user)
    {
        assign(data().user_, user);
    }

    void set_password(const char* value)
    {
        assert(value);
        data().password_ = value;
    }

    template<class String>
    void set_password(const String& password)
    {
        assign(data().password_, password);
    }

    void set_host(const char* value)
    {
        assert(value);
        data().host_ = value;
    }

    template<class String>
    void set_host(const String& host)
    {
        assign(data().host_, host);
    }

    void set_port(const char* value)
    {
        assert(value);
        data().port_ = value;
    }

    template<class String>
    void set_port(const String& port)
    {
        assign(data().port_, port);
    }

    void set_path(const char* value)
    {
        assert(value);
        data().path_ = value;
    }

    template<class String>
    void set_path(const String& path)
    {
        assign(data().path_, path);
    }

    void set_query(const char* value)
    {
        assert(value);
        data().query_ = value;
    }

    template<class String>
    void set_query(const String& query)
    {
        assign(data().query_, query);
    }

    void set_fragment(const char* value)
    {
        assert(value);
        data().fragment_ = value;
    }

    template<class String>
    void set_fragment(const String& fragment)
    {
        assign(data().fragment_, fragment);
    }

    const string_type& scheme() const noexcept
    {
        return data().scheme_;
    }

    const string_type& user() const noexcept
    {
        return data().user_;
    }

    const string_type& password() const noexcept
    {
        return data().password_;
    }

    const string_type& host() const noexcept
    {
        return data().host_;
    }

    const string_type& port() const noexcept
    {
        return data().port_;
    }

    int dport() const noexcept
    {
        return std::atoi(port().data());
    }

    const string_type& path() const noexcept
    {
        return data().path_;
    }

    const string_type& query() const noexcept
    {
        return data().query_;
    }

    const string_type& fragment() const noexcept
    {
        return data().fragment_;
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
