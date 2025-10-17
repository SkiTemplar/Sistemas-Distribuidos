cmake_minimum_required(VERSION 3.5)

project(ChatTCP LANGUAGES CXX)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Servidor
add_executable(server server.cpp)

# Cliente
add_executable(client client.cpp)

# Configuración específica por plataforma
if(WIN32)
    target_link_libraries(server ws2_32)
    target_link_libraries(client ws2_32)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(server Threads::Threads)
    target_link_libraries(client Threads::Threads)
endif()
