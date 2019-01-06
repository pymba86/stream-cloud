cmake_minimum_required(VERSION 2.8)

set(PROJECT stream-config)

project(${PROJECT} CXX)

add_compile_options(-std=c++14)

message (STATUS "${PROJECT_NAME} : CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

add_compile_options(
        -v
)


list(APPEND HEADER_FILES
        src/context.hpp
        src/data_provider.hpp
        src/abstract_service.hpp
        src/dynamic_environment.hpp
        src/configuration.hpp
        src/forward.hpp
        )

list(APPEND SOURCE_FILES
        src/data_provider.cpp
        src/abstract_service.cpp
        src/dynamic_environment.cpp
        src/configuration.cpp
        )

add_library(
        ${PROJECT}
        ${SOURCE_FILES}
        ${HEADER_FILES})


set_target_properties( ${PROJECT} PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(    ${PROJECT}        ${Boost_LIBRARIES}  stream-core)

target_include_directories(
        ${PROJECT}
        PUBLIC

        stream-core
)