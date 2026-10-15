#include "thread_hasher.hpp"

#include <utility>

thread_hasher::thread_hasher(
    std::jthread&& _thr,
    void (*_fn)(std::stop_token, int, std::string_view),
    std::tuple<std::stop_token, int, std::string_view> arg,
    std::uint64_t _count
) : _thread(
    std::move(
        _thr
    )
), _id(
    _count
), _func(
    _fn
),
_args(
    arg
)
{}

thread_hasher::thread_hasher(
    std::jthread&& _thr,
    void (*_fn)(std::stop_token, int, std::string_view),
    std::tuple<std::stop_token, int, std::string_view> arg
) : _thread(
    std::move(
        _thr
    )
), _id(
    0ull
), _func(
    _fn
),
_args(
    arg
)
{}

thread_hasher::thread_hasher(
    const thread_hasher& other
) : _thread(
    std::move(
        other._thread
    )),
    _id(
        other._id
    ),
    _func(
        other._func
    ),
    _args(
        other._args
    )
{
    other._thread = std::jthread(
        _func,
        std::get<0>(other._args),
        std::get<1>(other._args),
        std::get<2>(other._args)
    );
}

thread_hasher::thread_hasher(
    thread_hasher&& other
) : _thread(
    std::move(
        other._thread
    )
),
_id(
    other._id
),
_func(
    other._func
),
_args(
    std::move(other._args)
)
{}

thread_hasher& thread_hasher::operator= (
    const thread_hasher& other
) {
    if (
        &other != this
    ) {
        _thread = std::move(other._thread);
        _id = other._id;
        _func = other._func;
        _args = other._args;

        other._thread = std::jthread(
            _func,
            std::get<0>(other._args),
            std::get<1>(other._args),
            std::get<2>(other._args)
        );
    }

    return *this;
}

thread_hasher& thread_hasher::operator= (
    thread_hasher&& other
) {
    if (
        &other != this
    ) {
        _thread = std::move(other._thread);
        _id = other._id;
        _func = other._func;
        _args = other._args;
    }
    return *this;
}

bool thread_hasher::operator< ( const thread_hasher& other ) const {
    return _id < other._id;
}

void thread_hasher::request_jthread_stop() const& {
    if (
        (_thread.get_stop_token()).stop_possible()
    ) {
        _thread.request_stop();
    }
}
