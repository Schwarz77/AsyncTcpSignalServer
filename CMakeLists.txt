cmake_minimum_required(VERSION 3.14)
project(AsyncTcpServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/Include)

#find_package(Boost 1.82 REQUIRED COMPONENTS system thread)

# subdir
add_subdirectory(Server)
add_subdirectory(Server2)
add_subdirectory(Client)
add_subdirectory(Utils)