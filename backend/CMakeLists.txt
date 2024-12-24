cmake_minimum_required(VERSION 3.10)
project(chat)

set(CMAKE_CXX_STANDARD 14)
find_package(Boost REQUIRED COMPONENTS system)

add_executable(chat_server main.cpp server.cpp)
target_link_libraries(chat_server PRIVATE Boost::system pthread) 