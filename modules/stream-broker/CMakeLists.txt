cmake_minimum_required(VERSION 2.8)

set(PROJECT stream-io)

project(${PROJECT} CXX)

add_compile_options(-std=c++11)

message (STATUS "${PROJECT_NAME} : CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

add_compile_options(
        -v
        -fno-exceptions
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -pthread ")


list(APPEND HEADER_FILES
        src/actor/platform.hpp
        src/environment/network_environment.hpp
        src/network/connection_identifying.hpp
        src/network/multiplexer.hpp
        )

list(APPEND SOURCE_FILES
        src/actor/broker.cpp
        src/environment/network_environment.hpp src/network/multiplexer.hpp src/network/connection_identifying.hpp)

add_library(
        ${PROJECT}
        ${SOURCE_FILES}
        ${HEADER_FILES}
)


set_target_properties( ${PROJECT} PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(    ${PROJECT}        ${Boost_LIBRARIES})

target_include_directories(
        ${PROJECT}
        PUBLIC
        stream-api
        stream-config
        stream-core
        stream-providers
        stream-router
)