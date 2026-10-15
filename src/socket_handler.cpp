#include "socket_handler.hpp"
#include "logger.hpp"

#define MAX_THREAD_COUNT (std::jthread::hardware_concurrency() > 4u) ? 6u : 2u

#include <cstring>

#include <format>
#include <string>
#include <stdexcept>
#include <mutex>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using sockaddr_in_t = struct sockaddr_in;
using sockaddr_t = struct sockaddr;

namespace std {
    void* (*memset_explicit_fn)(void*, int, std::size_t) = memset_explicit;
}

std::vector<socket_handler> socket_handler::global_socket_holder(5);
std::atomic_bool socket_handler::atomic_signal_gate { false };
volatile std::sig_atomic_t socket_handler::POSIX_signal_runtime { 1 };

static std::mutex socket_initialiser_guard;

socket_handler::socket_handler(
) : m_signal_emplace(1),
    m_signal_thread_id(1),
    m_unique_counter(0ull)
{}

socket_handler::socket_handler(
    const socket_handler& other
) : m_signal_emplace(1),
    m_signal_thread_id(1),
    m_internal_pool(
        std::move(other.m_internal_pool)
    ),
    m_unique_counter(
        other.m_unique_counter
    )
{}

socket_handler::socket_handler(
    socket_handler&& other
) : m_signal_emplace(1),
    m_signal_thread_id(1),
    m_internal_pool(
        std::move(
            other.m_internal_pool
        )
    ),
    m_unique_counter(
        other.m_unique_counter
    )
{
    other.m_unique_counter = 0ull;
    other.m_internal_pool.shutdown();
}

socket_handler& socket_handler::operator= (
    const socket_handler& other
) {
    if (
        this != &other
    ) {
        m_internal_pool = std::move(*const_cast<
            typename std::add_pointer_t<typename std::remove_const_t<decltype(other.m_internal_pool)>>
        >(&other.m_internal_pool));
        m_unique_counter = other.m_unique_counter;
    }

    return *this;
}

socket_handler& socket_handler::operator= (
    socket_handler&& other
) {
    if (
        this != &other
    ) {
        m_internal_pool = std::move(other.m_internal_pool);
        m_unique_counter = other.m_unique_counter;

        other.m_unique_counter = 0ull;
        other.m_internal_pool.shutdown();
    }

    return *this;
}

void socket_handler::_M_emplace_data(
    std::jthread& _worker,
    void (*fn)(std::stop_token, int, std::string_view),
    std::tuple<std::stop_token, int, std::string_view> arg
) {
    m_signal_emplace.acquire();
    m_unique_counter++;
    m_internal_pool.emplace_data(
        m_signal_thread_id,
        std::move(_worker),
        fn,
        arg
    );
    m_signal_emplace.release();
}

void socket_handler::_M_stop() {
    if (
        m_stoppable
    ) {
        m_internal_pool.shutdown();
    }
}

void server_detail::_deal_with_signal([[maybe_unused]] int signal) {
    for (
        socket_handler& sock : socket_handler::global_socket_holder
    ) {
        sock._M_stop();
    }
}

void socket_handler::_M_start_signal() {
    if (
        !atomic_signal_gate.load(std::memory_order_acquire)
    ) {
        atomic_signal_gate.store(
            true,
            std::memory_order_release
        );

        server_detail::_deal_with_signal(SIGINT);
    }
}

void socket_handler::startup_socket(
    std::string_view _port,
    int _conn_request
) {
    int port = std::atoi(
        _port.data()
    );

    int sock = socket(
        AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP
    );

    sockaddr_in_t address;
    std::memset_explicit_fn(
        &address,
        0,
        sizeof(sockaddr_in_t)
    );

    address.sin_family = AF_INET;
    address.sin_port = htons(
        static_cast<uint16_t>(port)
    );

    int bind_val = bind(
        sock,
        reinterpret_cast<
        sockaddr_t*
        >(
            &address
        ),
        sizeof(sockaddr_in_t)
    );

    if (
        bind_val < 0
    ) {
        log::log_console(
            "Error binding the socket to the address.",
            LOG_LEVEL::EXCEPTION
        );
        throw std::runtime_error("The 'bind_val' variable is a negative value.");
    }

    int listen_val = listen(
        sock,
        _conn_request
    );

    if (
        listen_val < 0
    ) {
        log::log_console(
            "Error in trying to listen to the amount of connections within the socket.",
            LOG_LEVEL::EXCEPTION
        );
        throw std::runtime_error("The 'listen_val' variable is a negative value.");
    }

    sockaddr_in_t remote_address;
    std::memset_explicit_fn(
        &remote_address,
        0,
        sizeof(sockaddr_in_t)
    );

    socklen_t remote_addr_len { sizeof address };
    constexpr int BUFFER_LEN  { 1024 };

    constexpr auto handle_client = [](
        std::stop_token jthr_token,
        int client_socket,
        std::string_view client_ip
    ) static -> void {
        while (
            !jthr_token.stop_requested()
        ) {
            log::log_console(
                "Connected to client.",
                LOG_LEVEL::NORMAL
            );
            char buffer[BUFFER_LEN];
            std::memset_explicit_fn(
                buffer,
                0,
                BUFFER_LEN
            );

            int bytes_recieved = recv(
                client_socket,
                buffer,
                BUFFER_LEN-1,
                0
            );

            if (
                bytes_recieved < 0
            ) {
                log::log_console(
                    "Could not recieve any data from the client.",
                    LOG_LEVEL::FAIL
                );
                return;
            } else if (
                bytes_recieved == 0
            ) {
                log::log_console(
                    "Disconnecting from the client.",
                    LOG_LEVEL::NORMAL
                );
                shutdown(
                    client_socket,
                    SHUT_RDWR
                );
                return;
            } else {
                log::log_console(
                    std::format("Client message: {}", buffer).c_str(),
                    LOG_LEVEL::NORMAL
                );
                
                std::string response(std::format("Hello client at: {}", client_ip.data()));

                int bytes_send = send(
                    client_socket,
                    response.c_str(),
                    response.length(),
                    0
                );

                if (
                    bytes_send < 0
                ) {
                    log::log_console(
                        "Could not send the message.",
                        LOG_LEVEL::FAIL
                    );
                    return;
                }
            }
        }

        log::log_console(
            "Thread stopped.",
            LOG_LEVEL::NORMAL
        );
    };

    m_stoppable = true;

    this->_M_start_signal();
    while (
        POSIX_signal_runtime &&
        m_stoppable
    ) {
        if (
            MAX_THREAD_COUNT <= m_internal_pool.get_pool_count()
        ) {
            std::this_thread::yield();
            continue;
        }

        int client_sock = accept(
            sock,
            reinterpret_cast<
            sockaddr_t*
            >(
                &remote_address
            ),
            &remote_addr_len
        );

        if (
            client_sock < 0
        ) {
            log::log_console(
                "Client Error.",
                LOG_LEVEL::FAIL
            );
            continue;
        }

        char* ip_addr = inet_ntoa(remote_address.sin_addr);

        std::jthread worker(
            handle_client,
            client_sock,
            ip_addr
        );

        this->_M_emplace_data(
            worker,
            handle_client,
            {worker.get_stop_token(), client_sock, ip_addr}
        );
    }
}

void socket_libary::initialise_socket(
    socket_handler* _socket,
    int conn_requests,
    const char* _port_num
) {
    std::lock_guard<std::mutex> init(socket_initialiser_guard);
    socket_handler::global_socket_holder.emplace_back(
        std::move(*_socket)
    );

    (socket_handler::global_socket_holder.back()).startup_socket(
        _port_num,
        conn_requests
    );
}
