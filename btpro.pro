TEMPLATE = lib

CONFIG -= qt
CONFIG -= app_bundle
CONFIG += staticlib c++14 warn_on

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
    LIBS += -L/usr/local/Cellar/libevent/2.1.8/lib -levent -levent_pthreads
}

HEADERS += \
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
    evstack.hpp
