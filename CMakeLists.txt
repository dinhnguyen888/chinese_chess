cmake_minimum_required(VERSION 4.2)
project(server_chinese_chess)

set(CMAKE_CXX_STANDARD 17)

# Search for the Boost libraries
find_package(Boost REQUIRED COMPONENTS system)

include(FetchContent)
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

add_executable(server_chinese_chess main.cpp)

# Include Boost headers and link libraries
target_include_directories(server_chinese_chess PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(server_chinese_chess PRIVATE ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} nlohmann_json::nlohmann_json)
