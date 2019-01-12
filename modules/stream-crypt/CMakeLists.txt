cmake_minimum_required(VERSION 2.8)

set(PROJECT stream-auth)

project(${PROJECT} CXX)

add_compile_options(-std=c++14)

message (STATUS "${PROJECT_NAME} : CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")


list(APPEND HEADER_FILES
        src/base64.hpp
        )

list(APPEND SOURCE_FILES
        src/base64.cpp
        )

add_library(
        ${PROJECT}
        ${SOURCE_FILES}
        ${HEADER_FILES}
)


set_target_properties( ${PROJECT} PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/src)
