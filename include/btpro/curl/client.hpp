#pragma once

#include "btpro/curl/request.hpp"
#include "btpro/evcore.hpp"
#include "btpro/queue.hpp"

#include <vector>
#include <type_traits>
#include <algorithm>

namespace stx
{
    namespace lambda_detail
    {
        template<class Ret, class Cls, class IsMutable, class... Args>
        struct types
        {
            using is_mutable = IsMutable;

            enum { arity = sizeof...(Args) };

            using return_type = Ret;

            template<size_t i>
            struct arg
            {
                typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
            };
        };
    }

    template<class Ld>
    struct lambda_type
        : lambda_type<decltype(&Ld::operator())>
    {};

    template<class Ret, class Cls, class... Args>
    struct lambda_type<Ret(Cls::*)(Args...)>
        : lambda_detail::types<Ret,Cls,std::true_type,Args...>
    {};

    template<class Ret, class Cls, class... Args>
    struct lambda_type<Ret(Cls::*)(Args...) const>
        : lambda_detail::types<Ret,Cls,std::false_type,Args...>
    {};
}

namespace btpro {
namespace curl {

class client
{
public:
    using handle_t = multi_handle_t;
    using error_fn_type = std::function<void(std::string_view)>;

private:
    queue_ref queue_;
    evs timer_{};
    error_fn_type error_fn_{};

    std::unique_ptr<CURLM, decltype(&curl_multi_cleanup)>
        handle_{nullptr, curl_multi_cleanup};

    class client_operation;

    using operation_list_type = std::list<client_operation>;
    using operation_ptr_type = operation_list_type::iterator;

    operation_list_type running_{};
    operation_list_type complete_{};

    class client_operation
        : public operation
    {
    private:
        operation_ptr_type ptr_{};

    public:
        client_operation(detail::base_resp *resp)
            : operation(resp)
        {
            set_opt(handle(), CURLOPT_PRIVATE, this);
        }

        client_operation(detail::base_resp *resp, const std::string& url)
            : operation(resp)
        {
            assert(resp);
            resp->assign(handle());
            set_opt(handle(), CURLOPT_PRIVATE, this);
            operation::set(url);

#ifndef NDEBUG
            operation::set(CURLOPT_VERBOSE, 1l);
#endif
        }

        void assign(operation_ptr_type ptr) noexcept
        {
            ptr_ = ptr;
        }

        auto id() const noexcept
        {
            return ptr_;
        }

        CURLcode perform(multi_handle_t multi)
        {
            assert(multi);

            if (!outhdr_.empty())
                outhdr_(handle());

            CURLMcode err = curl_multi_add_handle(multi, handle());
            if (CURLM_OK != err)
                throw std::runtime_error(str_error(err));

            return CURLE_OK;
        }

        void reset(detail::base_resp *resp)
        {
            assert(resp);
            resp->assign(handle());
            resp_.reset(resp);
        }
    };

    client(client&) = delete;
    client& operator=(client&) = delete;

    static inline handle_t create()
    {
        handle_t handle = curl_multi_init();
        if (!handle)
            throw std::runtime_error("curl_multi_init");
        return handle;
    }

    void set_timer(const timeval& tv) noexcept
    {
        timer_.add(tv);
    }

    void kill_timer() noexcept
    {
        timer_.remove();
    }

    static inline void add_event(client& that, 
        handle_t easy, curl_socket_t sock, int curl_event)
    {
        int type = event_mask(curl_event);
        // объект esonvh
        // не содержит деструктора с удалением
        evheap ev;
        ev.create(that.base(), sock, EV_PERSIST|type, event_cb, &that);
        int result = event_add(ev.handle(), nullptr);
        if (result == code::fail)
            throw std::runtime_error("event_add");

        // просто передаем
        that.assign(easy, sock, ev.handle());
    }

    static inline void set_event(client& that,  handle_t,
        curl_socket_t socket, int curl_event, evheap& ev)
    {
        int type = event_mask(curl_event);

        auto h = ev.handle();
        if (event_initialized(h))
            event_del(h);

        int result = event_assign(h, that.base(),
            socket, EV_PERSIST|type, event_cb, &that);
        if (result == code::fail)
            throw std::runtime_error("event_assign");

        result = event_add(h, nullptr);
        if (result == code::fail)
            throw std::runtime_error("event_add");
    }

    static inline void remove_event(client& that, handle_t easy,
        curl_socket_t sock, evheap& ev)
    {
        ev.destroy();
        that.assign(easy, sock, nullptr);
    }

    static inline int sock_fn(handle_t easy, curl_socket_t sock,
        int type, void *cl, void *eh)
    {
        client& that = *static_cast<client*>(cl);
        if (eh)
        {
            evheap ev;
            ev.attach(static_cast<event_handle_t>(eh));
            if (type == CURL_POLL_REMOVE)
                remove_event(that, easy, sock, ev);
            else
                set_event(that, easy, sock, type, ev);
        }
        else
            add_event(that, easy, sock, type);

        return 0;
    }

    static inline int timer_fn(multi_handle_t, long ms, void *cl) noexcept
    {
        client& that = *static_cast<client*>(cl);

        if (ms < 0)
        {
            that.kill_timer();
            return 0;
        }

        that.set_timer(make_timeval(std::chrono::milliseconds(ms)));
        return 0;
    }

    static void timer_cb(int, short, void *cl) noexcept
    {
        client& that = *static_cast<client*>(cl);
        that.dispatch(CURL_SOCKET_TIMEOUT, 0);
    }

    static inline int event_mask(short curl_event)
    {
        // EV_READ = CURL_CSELECT_IN
        // EV_WRITE = CURL_CSELECT_OUT
        // check this
        static_assert((EV_READ == 2) && (CURL_CSELECT_IN == 1));
        static_assert((EV_WRITE == 4) && (CURL_CSELECT_OUT == 2));

        return ((CURL_CSELECT_IN|CURL_CSELECT_OUT) & curl_event) << 1;
    }

    static inline void event_cb(int fd, short kind, void *cl) noexcept
    {
        client& that = *static_cast<client*>(cl);

        static_assert((EV_READ == 2) && (CURL_CSELECT_IN == 1));
        static_assert((EV_WRITE == 4) && (CURL_CSELECT_OUT == 2));

        int action = ((EV_READ|EV_READ) & kind) >> 1;

        that.dispatch(fd, action);
    }

    void assign(handle_t easy, curl_socket_t sock, event_handle_t ev)
    {
        CURLMcode err = curl_multi_assign(assert_handle(), sock, ev);
        if (err != CURLM_OK)
            throw std::runtime_error(str_error(err));

        auto& op = get_op(easy);
        // нужно отработать удаление
        // если у нас вебсокет в режиме стрима
        if (nullptr == ev)
        {
            if (op.stream())
                complete_.splice(complete_.end(), running_, op.id());
        }
    }

    void remove(easy_handle_t easy) noexcept
    {
        assert(easy);

        CURLMcode err = curl_multi_remove_handle(assert_handle(), easy);
        if (err != CURLM_OK)
            on_error(str_error(err));
    }

    client_operation& get_op(easy_handle_t easy) noexcept
    {
        client_operation *ptr = nullptr;
        CURLcode err = curl_easy_getinfo(easy, CURLINFO_PRIVATE, &ptr);
        if (err != CURLE_OK)
        {
            using namespace std::literals;
            on_error("CURLINFO_PRIVATE"sv);
        }

        assert(ptr);

        return *ptr;
    }

    void done(client_operation& req, CURLcode code) noexcept
    {
        try
        {
            req.done(code);
            running_.erase(req.id());
        }
        catch (const std::exception& e)
        {
            on_error(e);
        }
        catch(...)
        {   
            on_error(std::string_view("client::done"));
        }
    }

    void dispatch_force(operation_list_type& list, CURLcode code) noexcept
    {
        try
        {
            for (auto& req : list)
            {
                remove(req);
                req.done(code);
            }

            list.clear();
        }
        catch (const std::exception& e)
        {
            on_error(e);
        }
        catch(...)
        {
            on_error(std::string_view("client::dispatch_force"));
        }
    }

    int dispatch(int fd, int action) noexcept
    {
        int count = 0;
        int running = 0;
        CURLMcode err = curl_multi_socket_action(
            assert_handle(), fd, action, &running);
        if (err == CURLM_OK)
        {
            CURLMsg *info = nullptr;
            while ((info = curl_multi_info_read(assert_handle(), &count)))
            {
                if (info->msg == CURLMSG_DONE)
                {
                    auto easy = info->easy_handle;
                    assert(easy);

                    remove(easy);

                    done(get_op(easy), info->data.result);
                }
            }
        }
        else
        {
            auto text = str_error(err);
            on_error(std::string_view(text, strlen(text)));
        }

        dispatch_force(complete_, CURLE_RECV_ERROR);

        return count;
    }

    void clean() noexcept
    {
        dispatch_force(running_, CURLE_READ_ERROR);
        dispatch_force(complete_, CURLE_READ_ERROR);
    }

    void on_error(const std::exception& e) noexcept
    {
        on_error(e.what());
    }

    void on_error(std::string_view text) noexcept
    {
        try
        {

            std::cout << "ON_ERROR: " << text << std::endl;

            if (error_fn_)
                error_fn_(text);
        }
        catch (...)
        {   }
    }

    handle_t assert_handle() const noexcept
    {
        auto result = handle();
        assert(result);
        return result;
    }

    client_operation& create_request()
    {
        auto ptr = running_.emplace(running_.end(), nullptr);
        ptr->assign(ptr);
        return *ptr;
    }

    client_operation& create_request(const std::string& url, detail::base_resp *resp)
    {
        auto ptr = running_.emplace(running_.end(), resp, std::cref(url));
        ptr->assign(ptr);
        return *ptr;
    }

public:

    client(queue_ref queue)
        : queue_(queue)
        , handle_(create(), curl_multi_cleanup)
    {
        set_opt(assert_handle(), CURLMOPT_SOCKETFUNCTION, client::sock_fn);
        set_opt(assert_handle(), CURLMOPT_SOCKETDATA, this);
        set_opt(assert_handle(), CURLMOPT_TIMERFUNCTION, client::timer_fn);
        set_opt(assert_handle(), CURLMOPT_TIMERDATA, this);

        timer_.create(queue, -1, 0, client::timer_cb, this);
    }

    ~client()
    {
        clean();
    }

    client& set(CURLMoption pref, long val)
    {
        set_opt(handle(), pref, val);
        return *this;
    }
    
    handle_t handle() const noexcept
    {
        return handle_.get();
    }

    operator handle_t() const noexcept
    {
        return handle();
    }

    queue_ref base() const noexcept
    {
        return queue_;
    }

    void set(error_fn_type fn)
    {
        error_fn_ = std::move(fn);
    }

    class get_req
    {
        client_operation& client_op_;

    public:
        get_req(client_operation& client_op) noexcept
            : client_op_(client_op)
        {   }

        // добавить хдер
        void push(std::string_view key, std::string_view value)
        {
            client_op_.push(key, value);
        }

        // добаить хидер строку формат - "key:value"
        void push_header(std::string_view kv)
        {
            client_op_.push_header(kv);
        }

        void set(const std::string& url)
        {
            client_op_.set(url);
        }

        void set(CURLoption pref, long val)
        {
            client_op_.set(pref, val);
        }

        template<class T, CURLoption Opt>
        void set(const basic_option<T, Opt>& opt)
        {
            opt(client_op_);
        }

        template<class F>
        void set(F fn)
        {
            using Arg0 = typename stx::lambda_type<decltype(fn)>::arg<0>::type;
            client_op_.reset(new detail::get_resp<Arg0>(std::move(fn)));
        }

        template<class F>
        void set(const std::string& url, F fn)
        {
            set(url);
            set(std::move(fn));
        }
    };


    template<class F>
    auto& get(F fn)
    {
        auto& req = create_request();
        try
        {
            fn(get_req(req));
            req.perform(handle());
            return req;
        }
        catch (const std::exception& e)
        {
            on_error(e);
        }

        return req;
    }


    template<class F>
    auto& get(const std::string& url, F fn)
    {

        using Arg0 = typename stx::lambda_type<decltype(fn)>::arg<0>::type;

        auto& req = create_request(url,
            new detail::get_resp<Arg0>(std::move(fn)));

        req.perform(handle());

        return req;
    }

//    void get(const std::string& url, get_equest_fn fn)
//    {
//        auto request_ref = create_get_request(resp_fn);

//    }

//    void get(const std::string& url, responce_fn_type fn)
//    {
//        request req;
//        req.create();

//        auto ptr = new curl::get();
//        req.set(ptr);

//        req.set(curl::url(url));
//        req.set(CURLOPT_HTTPGET, 1l);

//        req.set(CURLOPT_FAILONERROR, 1l);
//        req.set(CURLOPT_TCP_KEEPALIVE, 1l);
//        req.set(CURLOPT_NOSIGNAL, 1l);

//        // enable redirect following
//        req.set(CURLOPT_FOLLOWLOCATION, 1l);
//        req.set(CURLOPT_MAXREDIRS, 5l);

//        // enable all supported built-in compressions
//        req.set(accept_encoding(std::string()));

//        ptr->set(std::move(fn));
//        ptr->assign(req);

//#ifndef NDEBUG
//        req.set(CURLOPT_VERBOSE, 1l);
//#endif
//        add(req);
//    }

//    void post(const std::string& url, buffer buf, responce_fn_type fn)
//    {
//        request req;
//        req.create();

//        auto ptr = new curl::post();
//        req.set(ptr);

//        req.set(curl::url(url));
//        req.set(CURLOPT_HTTPPOST, 1l);

//        req.set(CURLOPT_FAILONERROR, 1l);
//        req.set(CURLOPT_TCP_KEEPALIVE, 1l);
//        req.set(CURLOPT_NOSIGNAL, 1l);

//        // enable redirect following
//        req.set(CURLOPT_FOLLOWLOCATION, 1l);
//        req.set(CURLOPT_MAXREDIRS, 5l);

//        // enable all supported built-in compressions
//        req.set(accept_encoding(std::string()));

//        ptr->set(std::move(fn));
//        ptr->assign(req, std::move(buf));


//#ifndef NDEBUG
//        req.set(CURLOPT_VERBOSE, 1l);
//#endif
//        add(req);
//    }

//    void open(const std::string& url, const std::string& proto)
//    {
//        request req;
//        req.create();

//        auto ptr = new curl::websocket();
//        req.set(ptr);

//        req.set(curl::url(url));
//        ptr->assign(req, proto);

//#ifndef NDEBUG
//        req.set(CURLOPT_VERBOSE, 1l);
//#endif
//        add(req);
//    }
};

} // namsspace curl
} // namespace btpro
