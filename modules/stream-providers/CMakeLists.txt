cmake_minimum_required(VERSION 2.8)

set(PROJECT stream-providers)

project(${PROJECT} CXX)

add_compile_options(-std=c++11)

message (STATUS "${PROJECT_NAME} : CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

add_compile_options(
        -v
        -fno-rtti
        -fno-exceptions
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -pthread ")


list(APPEND HEADER_FILES
        src/http/http_server.hpp
        )

list(APPEND SOURCE_FILES
        src/http/http_server.cpp)

add_library(
        ${PROJECT}
        ${SOURCE_FILES}
        ${HEADER_FILES}
)


set_target_properties( ${PROJECT} PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_include_directories(
        ${PROJECT}
        PUBLIC
        stream-core
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)