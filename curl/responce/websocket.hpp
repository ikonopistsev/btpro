#pragma once

#include "btpro/curl/resp.hpp"
#include "btpro/curl/responce/base.hpp"
#include "btdef/text.hpp"

namespace btpro {
namespace curl {
namespace responce {

class websocket
    : protected base
{
    class handler
    {
    public:
        using text_fn_type = std::function<void(std::string_view)>;
        text_fn_type on_text_{};

    public:
        handler() = default;

        void set(text_fn_type fn)
        {
            on_text_ = std::move(fn);
        }

        void call_text(std::string_view text)
        {
            on_text_(text);
        }
    };

    using handler_fn_type = std::function<void(handler&)>;

    handler handler_{};
    handler_fn_type handler_fn_{};

    header::store hdr_{};
    header::parser<websocket> hparse_{*this, hdr_};

    bool stream_{false};
    std::string protocol_{};
    std::string client_key_{};

public:
    websocket() = default;

    void assign(easy_handle_t easy, std::string_view proto)
    {
        assert(easy);

        using namespace std::literals;

    // I found it here:
    // https://github.com/barbieri/barbieri-playground/tree/master/curl-websocket

        /*
         * BEGIN: work around CURL to get WebSocket:
         *
         * WebSocket must be HTTP/1.1 GET request where we must keep the
         * "send" part alive without any content-length and no chunked
         * encoding and the server answer is 101-upgrade.
         */
        set_opt(easy, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);

        /* Use CURLOPT_UPLOAD=1 to force "send" even with a GET request,
         * however it will set HTTP request to PUT
         */
        set_opt(easy, CURLOPT_UPLOAD, 1L);

        /*
         * Then we manually override the string sent to be "GET".
         */
        set_opt(easy, CURLOPT_CUSTOMREQUEST, "GET");

        /*
         * CURLOPT_UPLOAD=1 with HTTP/1.1 implies:
         *     Expect: 100-continue
         * but we don't want that, rather 101. Then force: 101.
         */
        push_header("Expect: 101"sv);

        /*
         * CURLOPT_UPLOAD=1 without a size implies in:
         *     Transfer-Encoding: chunked
         * but we don't want that, rather unmodified (raw) bites as we're
         * doing the websockets framing ourselves. Force nothing.
         */
        push_header("Transfer-Encoding:"sv);

        /* END: work around CURL to get WebSocket. */

        /* regular mandatory WebSockets headers */
        push_header("Connection: Upgrade"sv);
        push_header("Upgrade: websocket"sv);
        push_header("Sec-WebSocket-Version: 13"sv);

        /* Sec-WebSocket-Key: <24-bytes-base64-of-random-key> */
        uint8_t rnd_buf[16];
        ssl::rand rnd;
        rnd(rnd_buf, sizeof(rnd_buf));

        ssl::base64 b64;
        client_key_ = b64.encode(rnd_buf, sizeof(rnd_buf));

        push("Sec-WebSocket-Key", client_key_);

        if (!proto.empty())
            push("Sec-WebSocket-Protocol", proto);

        // запрещаем использовать повторно
        set_opt(easy, CURLOPT_FORBID_REUSE, 1l);
        // просим курл создать новый сокет
        set_opt(easy, CURLOPT_FRESH_CONNECT, 1l);
        set_opt(easy, CURLOPT_TCP_KEEPALIVE, 1l);

        set_opt(easy, CURLOPT_WRITEDATA, this);
        set_opt(easy, CURLOPT_WRITEFUNCTION, proxy<websocket>::writecb);

        set_opt(easy, CURLOPT_READDATA, this);
        set_opt(easy, CURLOPT_READFUNCTION, proxy<websocket>::readcb);
        set_opt(easy, CURLOPT_FAILONERROR, 1l);

        hdr_.assign(easy);

        base::assign(easy);

        // создаем парсер проотокла
        wslay_.client();

        easy_ = easy;
    }


    virtual void done(easy_handle_t easy, CURLcode code) noexcept override
    {

    }

    // является ли вебсокетом
    virtual bool stream() noexcept override
    {
        return stream_;
    }

    bool call(header::store_ref hdr)
    {
        using namespace std::literals;

        // получаем sec-websocket-accept
        constexpr auto accept_hdr = "sec-websocket-accept"sv;
        auto accept_key = hdr.get(accept_hdr);
        if (accept_key.empty())
            return false;

        // выполняем проверку sec-websocket-accept
        constexpr auto uuid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"sv;
        btdef::text str(client_key_);
        str += uuid;

        ssl::sha1 sha1;
        ssl::sha1::outbuf_type sha1buf;
        sha1(str.data(), str.size(), sha1buf);

        ssl::base64 b64;
        if (accept_key != b64.encode(sha1buf, sizeof(sha1buf)))
            return false;

        // запрос является вебсокетом
        stream_ = true;

        handler_fn_(handler_);

        return true
    }

public:


};

} // namespace
} // namespace
} // namespace
