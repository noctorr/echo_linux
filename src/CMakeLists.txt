cmake_minimum_required(
    VERSION 4.4
)

project(
    echo_server_multithreaded LANGUAGES CXX
)

set(
    CMAKE_CXX_COMPILER g++
)

set(
    CMAKE_CXX_STANDARD 26
)

set(
    CMAKE_CXX_STANDARD_REQUIRED ON
)

set(
    CMAKE_CXX_EXTENSIONS OFF
)

set(
    CMAKE_CXX_FLAGS_RELEASE "-O3"
)

add_compile_options(
    "-Wall" "-pedantic" "-Wextra"
)

add_executable(
    ${PROJECT_NAME}
    main.cpp
    socket_handler.cpp
    thread_hasher.cpp
)
