cmake_minimum_required(VERSION 3.20)

# Silence dev warnings from newer CMake (FetchContent timestamps, FindBoost removal).
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

project(server_chinese_chess)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(server_chinese_chess main.cpp)

file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS "src/*.cpp")

target_sources(server_chinese_chess PRIVATE ${PROJECT_SOURCES})
target_include_directories(server_chinese_chess PRIVATE include)

if(WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0601)
endif()
if(MSVC)
    add_compile_options(/permissive- /Zc:dllexportInlines-)
endif()

find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(libpqxx CONFIG REQUIRED)
find_package(jwt-cpp CONFIG REQUIRED)

include(FetchContent)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)

target_link_libraries(server_chinese_chess PRIVATE
    Boost::system
    Threads::Threads
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    libpqxx::pqxx
    jwt-cpp::jwt-cpp
)
if(WIN32)
    target_link_libraries(server_chinese_chess PRIVATE ws2_32)
endif()

if(MSVC)
    target_link_options(server_chinese_chess PRIVATE /FORCE:MULTIPLE)
endif()
