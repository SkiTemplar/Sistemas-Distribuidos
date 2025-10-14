cmake_minimum_required(VERSION 3.16)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(Chat LANGUAGES CXX)

add_executable(server
        utils.cpp utils.h
        clientManager.cpp clientManager.h
        server.cpp
)
target_link_libraries(server pthread)

add_executable(client
        utils.cpp utils.h
        clientManager.cpp clientManager.h
        client.cpp
)
target_link_libraries(client pthread)
