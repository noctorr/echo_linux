#pragma once

#include <thread>
#include <cstdint>
#include <type_traits>
#include <string_view>
#include <tuple>

class thread_hasher final {
    private:
    mutable std::jthread _thread;
    std::uint64_t _id;
    void (*_func)(
        std::stop_token,
        int,
        std::string_view
    ) = nullptr;
    std::tuple<std::stop_token, int, std::string_view> _args;
    friend std::less<thread_hasher>;
    public:
    thread_hasher(
        std::jthread&&,
        void (*_fn)(std::stop_token, int, std::string_view),
        std::tuple<std::stop_token, int, std::string_view>,
        std::uint64_t
    );

    thread_hasher(
        std::jthread&&,
        void (*_fn)(std::stop_token, int, std::string_view),
        std::tuple<std::stop_token, int, std::string_view>
    );

    thread_hasher() = default;

    thread_hasher(
        const thread_hasher&
    );

    thread_hasher(
        thread_hasher&&
    );

    thread_hasher& operator= (
        const thread_hasher&
    );
    
    thread_hasher& operator= (
        thread_hasher&&
    );

    bool operator< (
        const thread_hasher& other
    ) const;

    void request_jthread_stop() const&;
};
