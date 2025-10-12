 
cmake_minimum_required(VERSION 3.5)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(server LANGUAGES CXX)
add_executable(server utils.h utils.cpp server.cpp clientManager.cpp clientManager.h)
target_link_libraries(server pthread)


project(client LANGUAGES CXX)
add_executable(client utils.h utils.cpp client.cpp clientManager.cpp clientManager.h)
target_link_libraries(client pthread)
