#pragma once

#include "btpro/socket.hpp"

namespace btpro {

// хэндл динамического эвента
class evheap
{
private:
    event_handle_t handle_{ nullptr };

public:
    static inline event_handle_t create_event(queue_handle_t queue,
        evutil_socket_t fd, event_flag_t ef, event_callback_fn fn, void *arg)
    {
        auto handle = event_new(queue, fd, ef, fn, arg);
        if (!handle)
            throw std::runtime_error("event_new");
        return handle;
    }

    evheap() = default;

    // захват хэндла
    explicit evheap(event_handle_t handle) noexcept
        : handle_(handle)
    {   }

    // создание объекта
    void create(queue_handle_t queue, evutil_socket_t fd,
        event_flag_t ef, event_callback_fn fn, void *arg)
    {
        // если создаем повторно поверх
        // значит что-то пошло не так
        assert(queue && fn && !handle_);

        // создаем объект эвента
        handle_ = create_event(queue, fd, ef, fn, arg);
    }

    void create(queue_handle_t queue, be::socket sock, event_flag_t ef,
        event_callback_fn fn, void *arg)
    {
        create(queue, sock.fd(), ef, fn, arg);
    }

    void create(queue_handle_t queue, event_flag_t ef,
                event_callback_fn fn, void *arg)
    {
        create(queue, -1, ef, fn, arg);
    }

    void attach(event_handle_t handle) noexcept
    {
        handle_ = handle;
    }

    void attach(const evheap& other) noexcept
    {
        handle_ = other.handle();
    }

    void deallocate() noexcept
    {
        if (handle_)
        {
            event_del(handle_);
            event_free(handle_);
        }
    }

    void destroy() noexcept
    {
        deallocate();
        handle_ = nullptr;
    }

    event_handle_t handle() const noexcept
    {
        return handle_;
    }

    operator event_handle_t() const noexcept
    {
        return handle();
    }

    bool empty() const noexcept
    {
        return handle_ == nullptr;
    }
};

} // namespace btpro
