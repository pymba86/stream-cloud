cmake_minimum_required(VERSION 2.8)

set(PROJECT stream-api)

project(${PROJECT} CXX)

add_compile_options(-std=c++11)

message (STATUS "${PROJECT_NAME} : CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

add_compile_options(
        -v
        -fno-rtti
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -pthread ")


list(APPEND HEADER_FILES
        src/transport_base.hpp
        src/http.hpp
        src/websocket.hpp
        )

list(APPEND SOURCE_FILES
        src/transport_base.cpp
        src/http.cpp
        src/websocket.cpp
        )

add_library(
        ${PROJECT}
        ${SOURCE_FILES}
        ${HEADER_FILES}
)


set_target_properties( ${PROJECT} PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(    ${PROJECT}        ${Boost_LIBRARIES}  stream-config
        stream-core
        stream-io
        stream-providers
        stream-router)

target_include_directories(
        ${PROJECT}
        PUBLIC
        stream-config
        stream-core
        stream-io
        stream-providers
        stream-router
)