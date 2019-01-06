cmake_minimum_required(VERSION 2.8)

set(PROJECT stream-settings)

project(${PROJECT} CXX)

add_compile_options(-std=c++14)

message (STATUS "${PROJECT_NAME} : CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

add_compile_options(
        -v
)


list(APPEND HEADER_FILES
        src/settings.hpp
        )

list(APPEND SOURCE_FILES
        src/settings.cpp
        )

add_library(
        ${PROJECT}
        ${SOURCE_FILES}
        ${HEADER_FILES}
)


set_target_properties( ${PROJECT} PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(${PROJECT}
        ${Boost_LIBRARIES}
        stream-core
        stream-api
        stream-config
        stream-io
        stream-providers
        )

target_include_directories(
        ${PROJECT}
        PUBLIC
        stream-core
        stream-api
        stream-config
        stream-io
        stream-providers

)