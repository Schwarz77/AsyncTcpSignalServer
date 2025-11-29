#find_package(Boost 1.82 REQUIRED COMPONENTS system thread)

include_directories(${Boost_INCLUDE_DIRS})

add_executable(Client main.cpp Client.h Client.cpp)

target_link_libraries(Client PRIVATE Utils)
