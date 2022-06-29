#include "btpro/curl/request/base.hpp"

namespace btpro {
namespace curl {
namespace request {

class websocket
    : public base
{
    std::string protocol_{};
    std::string client_key_{};

    virtual void assign(easy_handle_t handle)
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

        base::assign(easy);
    }
};

} // namespace request
} // namespace curl
} // namespace btpro
