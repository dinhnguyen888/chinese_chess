cmake_minimum_required(VERSION 3.20)
project(server_chinese_chess)

set(CMAKE_CXX_STANDARD 17)

# Boost
find_package(Boost REQUIRED COMPONENTS system)

# Threads
find_package(Threads REQUIRED)

# JSON
include(FetchContent)
FetchContent_Declare(
        json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

add_executable(server_chinese_chess main.cpp)


target_link_libraries(server_chinese_chess PRIVATE
        Boost::system
        Threads::Threads
        ws2_32
        nlohmann_json::nlohmann_json
)