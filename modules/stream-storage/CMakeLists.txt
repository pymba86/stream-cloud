cmake_minimum_required(VERSION 2.8)

set(PROJECT stream-storage)

project(${PROJECT} CXX)

add_compile_options(-std=c++14)

message (STATUS "${PROJECT_NAME} : CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

add_compile_options(
        -v
)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSQLITE_THREADSAFE=1 -DSQLITE_OPEN_FULLMUTEX=1")

list(APPEND HEADER_FILES
        src/sqlite3.h
        src/sqlite_modern_cpp.h
        src/sqlite_modern_cpp/errors.h
        src/sqlite_modern_cpp/log.h
        src/sqlite_modern_cpp/sqlcipher.h
        src/sqlite_modern_cpp/utility/function_traits.h
        src/sqlite_modern_cpp/utility/uncaught_exceptions.h
        src/sqlite_modern_cpp/utility/utf16_utf8.h
        src/sqlite_modern_cpp/utility/variant.h
        src/sqlite_modern_cpp/lists/error_codes.h
        )

list(APPEND SOURCE_FILES
        src/sqlite3.c
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