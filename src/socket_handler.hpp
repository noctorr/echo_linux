#pragma once

#include <queue>
#include "thread_hasher.hpp"

#include <semaphore>
#include <memory>
#include <atomic>

#include <csignal>

namespace std {
    template <>
    struct less<thread_hasher> {
        bool operator() (const thread_hasher& lhs, const thread_hasher& rhs) const {
            return lhs._id < rhs._id;
        }
    };
}

namespace server_detail {
    void _deal_with_signal(
        int
    );
}

class socket_handler;

namespace socket_libary {
    void initialise_socket(
        socket_handler*,
        int,
        const char*
    );
}

class socket_handler final {
    private:
    std::uint64_t m_unique_counter;
    std::binary_semaphore m_signal_emplace,
                          m_signal_thread_id;

    class thread_pool final {
        private:
        std::priority_queue<
            thread_hasher,
            std::vector<thread_hasher, std::allocator<thread_hasher>>,
            std::less<thread_hasher>
        > m_thread_data;
        std::uint_fast8_t m_pool_counter;

        public:
        thread_pool() = default;

        thread_pool(
            std::jthread&& _thread,
            void (*fn)(std::stop_token, int, std::string_view),
            std::tuple<std::stop_token, int, std::string_view> arg,
            std::uint64_t _counter
        ) : m_pool_counter(1) {
            m_thread_data.emplace(
                std::move(_thread),
                fn,
                arg,
                _counter
            );
        }

        thread_pool(
            thread_pool&& other
        ) : m_pool_counter(other.m_pool_counter),
        m_thread_data(std::move(other.m_thread_data))
        {}

        thread_pool(
            const thread_pool& other
        ) : m_pool_counter(
            other.m_pool_counter
        ),
        m_thread_data(
            std::move(other.m_thread_data)
        )
        {}

        thread_pool& operator= (
            thread_pool&& other
        ) {
            if (
                this != &other
            ) {
                m_thread_data = std::move(other.m_thread_data);
                m_pool_counter = other.m_pool_counter;
            }
            return *this;
        }

        void emplace_data(
            std::binary_semaphore& signal,
            std::jthread&& _worker,
            void (*fn)(std::stop_token, int, std::string_view),
            std::tuple<std::stop_token, int, std::string_view> arg
        ) {
            signal.acquire();
            m_pool_counter++;
            m_thread_data.emplace(
                std::move(_worker),
                fn,
                arg,
                m_pool_counter
            );
            signal.release();
        }

        void pop_data(
            std::binary_semaphore& signal
        ) {
            signal.acquire();
            m_pool_counter--;
            const thread_hasher& ref = m_thread_data.top();
            ref.request_jthread_stop();
            m_thread_data.pop();
            signal.release();
        }

        std::uint64_t get_pool_count() {
            return m_pool_counter;
        }

        void shutdown() {
            while (
                !m_thread_data.empty()
            ) {
                const thread_hasher& ref = m_thread_data.top();
                ref.request_jthread_stop();
                m_thread_data.pop();
            }
        }
    } m_internal_pool;

    friend void server_detail::_deal_with_signal(int);
    friend void socket_libary::initialise_socket(socket_handler*, int, const char*);
    public:
    explicit socket_handler();
    socket_handler(
        const socket_handler&
    );
    socket_handler(
        socket_handler&&
    );
    socket_handler& operator= (
        const socket_handler&
    );
    socket_handler& operator= (
        socket_handler&&
    );

    ~socket_handler() = default;

    private:
    bool m_stoppable = false;
    void startup_socket(
        std::string_view,
        int
    );

    void _M_emplace_data(
        std::jthread&,
        void (*fn)(std::stop_token, int, std::string_view),
        std::tuple<std::stop_token, int, std::string_view> arg
    );

    void _M_start_signal();
    void _M_stop();

    static std::atomic_bool atomic_signal_gate;
    static volatile std::sig_atomic_t POSIX_signal_runtime;
    static std::vector<socket_handler> global_socket_holder;
};
