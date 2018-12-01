cmake_minimum_required(VERSION 2.8)

set(PROJECT stream-client)

project(${PROJECT} CXX)

add_compile_options(-std=c++14)

message (STATUS "${PROJECT_NAME} : CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

add_compile_options(
        -v
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -pthread")



list(APPEND HEADER_FILES
        src/ws/ws_client.hpp
        src/ws/ws_session.hpp
        )

list(APPEND SOURCE_FILES
        src/ws/ws_client.cpp
        src/ws/ws_session.cpp
        )

add_library(
        ${PROJECT}
        ${SOURCE_FILES}
        ${HEADER_FILES}
)

target_link_libraries(    ${PROJECT}        ${Boost_LIBRARIES}    stream-core
        stream-api
        stream-config
        stream-io
        stream-providers)

target_include_directories(
        ${PROJECT}
        PUBLIC
        stream-core
        stream-api
        stream-config
        stream-io
        stream-providers
)