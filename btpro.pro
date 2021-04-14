TEMPLATE = lib

CONFIG -= qt
CONFIG -= app_bundle
CONFIG += staticlib c++17 warn_on

TARGET = btpro

CONFIG(release, debug|release) {
    DEFINES += NDEBUG
}

INCLUDEPATH += ../

unix:!macx {
    CONFIG += link_pkgconfig
    PKGCONFIG += libevent
}

macx {
    INCLUDEPATH += /usr/local/include
}

HEADERS += \
    curl/client.hpp \
    curl/curl.hpp \
    curl/header/store.hpp \
    curl/header/parser.hpp \
    curl/info.hpp \
    curl/io/buffer.hpp \
    curl/option.hpp \
    curl/request.hpp \
    curl/request/base.hpp \
    curl/request/get.hpp \
    curl/request/websocket.hpp \
    curl/resp.hpp \
    curl/responce/base.hpp \
    curl/responce/get.hpp \
    curl/responce/websocket.hpp \
    curl/slist.hpp \
    curl/websocket.hpp \
    http/connection.hpp \
    http/request.hpp \
    ssl/base64.hpp \
    ssl/context.hpp \
    ssl/rand.hpp \
    ssl/sha.hpp \
    tcp/acceptor.hpp \
    tcp/bev.hpp \
    tcp/listener.hpp \
    btpro.hpp \
    buffer.hpp \
    queue.hpp \
    sock_addr.hpp \
    socket.hpp \
    tcp/acceptorfn.hpp \
    tcp/tcp.hpp \
    sock_opt.hpp \
    ipv4/addr.hpp \
    ipv6/addr.hpp \
    ip/addr.hpp \
    posix/posix.hpp \
    posix/error_code.hpp \
    wsa/error_code.hpp \
    wsa/wsa.hpp \
    win/error_code.hpp \
    win/error_category.hpp \
    win/win.hpp \
    thread.hpp \
    ipv4/multicast_group.hpp \
    ipv4/multicast_source_group.hpp \
    config.hpp \
    evcore.hpp \
    evheap.hpp \
    evstack.hpp \
    dns.hpp \
    tcp/bevfn.hpp \
    ssl/connector.hpp \
    tcp/connector.hpp \
    uri.hpp \
    ssl/bevsock.hpp \
    tcp/bevsockfn.hpp \
    wslay/context.hpp
