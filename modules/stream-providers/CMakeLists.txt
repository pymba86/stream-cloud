cmake_minimum_required(VERSION 2.8)

set(PROJECT stream-providers)

project(${PROJECT} CXX)

add_compile_options(-std=c++14)

message (STATUS "${PROJECT_NAME} : CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

add_compile_options(
        -v
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -pthread")



list(APPEND HEADER_FILES
        src/http/http_server.hpp
        src/http/http_context.hpp
        src/http/http_session.hpp
        src/http/listener.hpp
        src/ws/ws_listener.hpp
        src/ws/ws_session.hpp
        src/ws/ws_server.hpp
        src/http/websocket_session.hpp
        )

list(APPEND SOURCE_FILES
        src/http/http_server.cpp
        src/http/http_session.cpp
        src/http/listener.cpp
        src/ws/ws_session.cpp
        src/ws/ws_server.cpp
        src/ws/ws_listener.cpp
        src/http/websocket_session.cpp
        )

add_library(
        ${PROJECT}
        ${SOURCE_FILES}
        ${HEADER_FILES}
)

target_link_libraries(    ${PROJECT}        ${Boost_LIBRARIES}    stream-core
        stream-api
        stream-config
        stream-io)

target_include_directories(
        ${PROJECT}
        PUBLIC
        stream-core
        stream-api
        stream-config
        stream-io
)