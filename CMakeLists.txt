cmake_minimum_required(VERSION 3.20)
project(server_chinese_chess)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_definitions(_WIN32_WINNT=0x0601)

if(MSVC)
    add_compile_options(/permissive-)
endif()

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

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

# libpqxx for PostgreSQL
find_package(libpqxx CONFIG REQUIRED)

include_directories(include)

file(GLOB_RECURSE SRC_FILES "src/*.cpp")
add_executable(server_chinese_chess main.cpp ${SRC_FILES})

# jwt-cpp
find_package(jwt-cpp CONFIG REQUIRED)

target_link_libraries(server_chinese_chess PRIVATE
        Boost::system
        Threads::Threads
        ws2_32
        nlohmann_json::nlohmann_json
        libpqxx::pqxx
        jwt-cpp::jwt-cpp
)