#find_package(Boost 1.82 REQUIRED COMPONENTS system thread)

include_directories(${Boost_INCLUDE_DIRS})

add_executable(Server main.cpp Server.h Server.cpp Session.h Session.cpp)

target_link_libraries(Server PRIVATE Utils)
