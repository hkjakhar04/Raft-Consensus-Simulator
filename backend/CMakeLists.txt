cmake_minimum_required(VERSION 3.14)
project(RaftSimulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find threads
find_package(Threads REQUIRED)

add_executable(RaftSimulator
    src/main.cpp
    src/raft_core/Node.cpp
)

target_include_directories(RaftSimulator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/raft_core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/telemetry
)

target_link_libraries(RaftSimulator PRIVATE Threads::Threads)
