
add_library(ServerCore STATIC 
    Server.h Server.cpp 
    Session.h Session.cpp
)

target_include_directories(
    ServerCore
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
    ServerCore 
    PUBLIC 
        Utils 
        ProjectInclude 
        Boost::system 
        Boost::thread 
        Boost::asio
        Boost::format
)

add_executable(Server main.cpp)


target_link_libraries(
    Server
    PRIVATE 
        ServerCore
)