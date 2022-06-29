#pragma once

#include "btpro/buffer.hpp"
#include "btpro/curl/option.hpp"
#include "btpro/curl/slist.hpp"
#include "btpro/curl/header/parser.hpp"
#include "btpro/curl/io/buffer.hpp"
#include "btpro/curl/resp.hpp"

#include "btpro/wslay/context.hpp"

#include "btpro/ssl/base64.hpp"
#include "btpro/ssl/rand.hpp"
#include "btpro/ssl/sha.hpp"

#include <list>
#include <functional>
#include <string_view>
#include <iostream>

namespace btpro {
namespace curl {

class operation
{
    static inline easy_handle_t create_handle()
    {
        auto easy = curl_easy_init();
        if (!easy)
            throw std::runtime_error("curl_easy_init");
        return easy;
    }

    std::unique_ptr<CURL, decltype(&curl_easy_cleanup)>
        easy_{create_handle(), curl_easy_cleanup};

protected:
    request_header outhdr_{};
    std::unique_ptr<detail::base_resp> resp_{};

    void reset(detail::base_resp *resp)
    {
        assert(resp);

        resp_.reset(resp);
    }

public:
    operation(detail::base_resp *resp) noexcept
        : resp_(resp)
    {   }

    virtual ~operation()
    {   }

    // добавить хдер
    void push(std::string_view key, std::string_view value)
    {
        outhdr_.push(key, value);
    }

    // добаить хидер строку формат - "key:value"
    void push_header(std::string_view kv)
    {
        outhdr_.push_header(kv);
    }

    easy_handle_t handle() const noexcept
    {
        return easy_.get();
    }

    operator easy_handle_t() const noexcept
    {
        return handle();
    }

    bool empty() const noexcept
    {
        return nullptr == handle();
    }

    void set(const std::string& url)
    {
        set_opt(handle(), CURLOPT_URL, url.c_str());
    }

    void set(CURLoption pref, long val)
    {
        set_opt(handle(), pref, val);
    }

    template<class T, CURLoption Opt>
    void set(const basic_option<T, Opt>& opt)
    {
        opt(*this);
    }

    CURLcode perform()
    {
        if (!outhdr_.empty())
            outhdr_(handle());

        return curl_easy_perform(handle());
    }

    void done(CURLcode code) noexcept
    {
        resp_->done(handle(), code);
    }

    void error(std::exception_ptr ex) noexcept
    {
        resp_->error(ex);
    }

    // является ли вебсокетом
    bool stream() noexcept
    {
        return resp_->stream();
    }
};

//template<class Data, class Header, class Protocol>
//struct basic_responce
//{

//};

//using responce_fn_type = std::function<void(responce)>;

//class get
//    : public base
//{
//public:
//    using inbuf_type = io::buffer<get, io::append>;

//protected:
//    responce_fn_type on_done_{};
//    header::store hdrs_{};
//    header::parser<get> hdr_{*this, hdrs_};
//    inbuf_type inbuf_{*this};

//    virtual void done(CURLcode code, easy_handle_t easy) noexcept override
//    {
//        if (on_done_)
//            on_done_(inbuf_.data(), code, easy);
//    }

//public:
//    get() = default;

//    constexpr bool call(header::store_cref) noexcept
//    {
//        return true;
//    }

//    void call(std::exception_ptr ex) noexcept
//    {
//        base::call(ex);
//    }

//    void assign(easy_handle_t easy)
//    {
//        assert(easy);
//        base::assign(easy);
//        hdr_.assign(easy);
//        inbuf_.assign(easy);
//    }

//    void set(responce_fn_type fn)
//    {
//        on_done_ = std::move(fn);
//    }

//    std::string print()
//    {
//        auto size = inbuf_.size();
//        if (size)
//        {
//            std::string rc;
//            rc.resize(size);
//            inbuf_.copyout(rc.data(), rc.size(#include "btpro/wslay/context.hpp"
//            return rc;
//        }

//        return std::string();
//    }
//};

//class post
//    : public get
//{
//public:
//    using outbuf_type = io::buffer<post, io::copyout>;

//private:
//    outbuf_type outbuf_{*this};

//public:
//    post() = default;

//    void assign(easy_handle_t handle, buffer buf)
//    {
//        assert(handle);
//        get::assign(handle);
//        outbuf_.append(std::move(buf));
//        outbuf_.assign(handle);
//    }
//};

//class websocket
//    : public base
//{
//    easy_handle_t easy_{nullptr};

//    header::store hdrs_{};
//    header::parser<websocket> hdr_{*this, hdrs_};

//    std::string client_key_{};

//    // прокси класс обработчик
//    // скрыаем интерфейсные методы
//    // возможност заменять методы
//    using wslay_handler_type = wslay::handler<websocket>;
//    wslay_handler_type wslay_handler_{*this,
//        &websocket::wslay_receive, &websocket::wslay_message};
//    using wslay_context = wslay::context<wslay_handler_type>;
//    wslay_context wslay_{wslay_handler_};
//    wslay_context::holder holder_{wslay_};

//    // стал ли запрос вебсокетом
//    bool stream_{false};

//    template<class F>
//    struct proxy
//    {
//        static size_t readcb(char *buffer,
//            size_t size, size_t nitems, void *self) noexcept
//        {
//            assert(self);
//            return static_cast<F*>(self)->write_to(buffer, size * nitems);
//        }

//        static size_t writecb(const char* data,
//            size_t size, size_t nitems, void *self) noexcept
//        {
//            assert(self);
//            return static_cast<F*>(self)->read_from(data, size * nitems);
//        }
//    };

//    // вызывает курл когда хочет что-то отправить
//    // мы должны записать
//    size_t write_to(char *buffer, size_t size) noexcept
//    {
//        if (wslay_.want_write())
//        {
//            // пишем напрямую из wslay в curl буффер
//            auto sz = wslay_.write(reinterpret_cast<uint8_t*>(buffer), size);
//            return (sz > 0) ? sz : CURL_READFUNC_ABORT;
//        }

//        return CURL_READFUNC_PAUSE;
//    }

//    size_t read_from(const char* data, size_t size) noexcept
//    {
//        inbuf_.append(data, size);

//        // уведомляем wslay что есть данные для чтения
//        // wslay вызывает recv_callback и msg_callback
//        wslay_.receive();

//        return size;
//    }

//    bool stream() noexcept override final
//    {
//        return stream_;
//    }

//    ssize_t wslay_receive(uint8_t *data, size_t len)
//    {
//        auto size = inbuf_.copyout(data, len);
//        inbuf_.drain(size);
//        return size;
//    }

//    void wslay_message(const wslay_event_on_msg_recv_arg *arg)
//    {
//        if (arg->opcode == WSLAY_TEXT_FRAME)
//        {
//            using namespace std::literals;
//            std::string_view text((const char*)arg->msg, arg->msg_length);

//            std::cout << text << std::endl;

//            constexpr auto needle = "{\"ping\":"sv;
//            auto f = text.find(needle);
//            if (f != std::string_view::npos)
//            {

//                std::string pong;
//                pong += "{\"pong\":"sv;
//                pong += text.substr(f + needle.size());

//                wslay_.queue_text(pong);

//                curl_easy_pause(easy_, 0);
//            }
//        }
//    }

//public:
//    websocket() = default;

//    void call(std::exception_ptr ex) noexcept
//    {
//        base::call(ex);
//    }

//    void send_logon(std::string_view login, std::string_view passwd)
//    {
//        using namespace std::literals;

//        ssl::sha1 sha1;
//        ssl::sha1::outbuf_type sha1buf;
//        sha1(passwd.data(), passwd.size(), sha1buf);
//        auto password_hash = btdef::to_hex(
//            reinterpret_cast<const char*>(sha1buf), sizeof(sha1buf));

//        std::string text;
//        text += "{\"login\":\""sv;
//        text += login;
//        text += "\",\"password_hash\":\""sv;
//        text += password_hash;
//        text += "\",\"symbols\":[]}"sv;

//        wslay_.queue_text(text);
//        // убрали curl c паузы
//        curl_easy_pause(easy_, 0);
//    }

//    bool call(header::store_cref hdr)
//    {
//        using namespace std::literals;

//        // получаем sec-websocket-accept
//        auto accept_key = hdr.get("sec-websocket-accept"sv);
//        if (accept_key.empty())
//            return false;

//        // выполняем проверку sec-websocket-accept
//        constexpr auto uuid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"sv;
//        std::string str(client_key_);
//        str += uuid;

//        ssl::sha1 sha1;
//        ssl::sha1::outbuf_type sha1buf;
//        sha1(str.data(), str.size(), sha1buf);

//        ssl::base64 b64;
//        if (accept_key != b64.encode(sha1buf, sizeof(sha1buf)))
//            return false;

//        // запрос является вебсокетом
//        stream_ = true;

//        send_logon("demopro"sv, "demopro"sv);

//        return true;
//    }

//    void assign(easy_handle_t easy, std::string_view proto)
//    {
//        assert(easy);

//        using namespace std::literals;

//// I found it here:
//// https://github.com/barbieri/barbieri-playground/tree/master/curl-websocket

//        /*
//         * BEGIN: work around CURL to get WebSocket:
//         *
//         * WebSocket must be HTTP/1.1 GET request where we must keep the
//         * "send" part alive without any content-length and no chunked
//         * encoding and the server answer is 101-upgrade.
//         */
//        set_opt(easy, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);

//        /* Use CURLOPT_UPLOAD=1 to force "send" even with a GET request,
//         * however it will set HTTP request to PUT
//         */
//        set_opt(easy, CURLOPT_UPLOAD, 1L);

//        /*
//         * Then we manually override the string sent to be "GET".
//         */
//        set_opt(easy, CURLOPT_CUSTOMREQUEST, "GET");

//        /*
//         * CURLOPT_UPLOAD=1 with HTTP/1.1 implies:
//         *     Expect: 100-continue
//         * but we don't want that, rather 101. Then force: 101.
//         */
//        push_header("Expect: 101"sv);

//        /*
//         * CURLOPT_UPLOAD=1 without a size implies in:
//         *     Transfer-Encoding: chunked
//         * but we don't want that, rather unmodified (raw) bites as we're
//         * doing the websockets framing ourselves. Force nothing.
//         */
//        push_header("Transfer-Encoding:"sv);

//        /* END: work around CURL to get WebSocket. */

//        /* regular mandatory WebSockets headers */
//        push_header("Connection: Upgrade"sv);
//        push_header("Upgrade: websocket"sv);
//        push_header("Sec-WebSocket-Version: 13"sv);

//        /* Sec-WebSocket-Key: <24-bytes-base64-of-random-key> */
//        uint8_t rnd_buf[16];
//        ssl::rand rnd;
//        rnd(rnd_buf, sizeof(rnd_buf));

//        ssl::base64 b64;
//        client_key_ = b64.encode(rnd_buf, sizeof(rnd_buf));

//        push("Sec-WebSocket-Key", client_key_);

//        if (!proto.empty())
//            push("Sec-WebSocket-Protocol", proto);

//        // запрещаем использовать повторно
//        set_opt(easy, CURLOPT_FORBID_REUSE, 1l);
//        // просим курл создать новый сокет
//        set_opt(easy, CURLOPT_FRESH_CONNECT, 1l);
//        set_opt(easy, CURLOPT_TCP_KEEPALIVE, 1l);

//        set_opt(easy, CURLOPT_WRITEDATA, this);
//        set_opt(easy, CURLOPT_WRITEFUNCTION, proxy<websocket>::writecb);

//        set_opt(easy, CURLOPT_READDATA, this);
//        set_opt(easy, CURLOPT_READFUNCTION, proxy<websocket>::readcb);
//        set_opt(easy, CURLOPT_FAILONERROR, 1l);

//        hdr_.assign(easy);

//        base::assign(easy);

//        // создаем парсер проотокла
//        wslay_.client();

//        easy_ = easy;
//    }

//    void done(CURLcode code, easy_handle_t) noexcept override final
//    {
//        std::cout << str_error(code) << std::endl;
//    }
//};

} // namespace curl
} // namespace btpro
