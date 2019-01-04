cmake_minimum_required(VERSION 3.5)

list(APPEND BOOST_COMPONENTS
        thread
        system
        locale
        timer
        filesystem
        serialization
        )

find_package (Boost 1.68  REQUIRED COMPONENTS ${BOOST_COMPONENTS})
include_directories (${Boost_INCLUDE_DIR})

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBOOST_ASIO_HAS_MOVE -DCXXOPTS_NO_RTTI" CACHE STRING "compile flags" FORCE)


add_compile_options(
        -v
        -Wall
        -Wextra
)

include_directories(modules/stream-api/src)
include_directories(modules/stream-core/src)
include_directories(modules/stream-broker/src)
include_directories(modules/stream-providers/src)
include_directories(modules/stream-config/src)
include_directories(modules/stream-router/src)
include_directories(modules/stream-platform/src)
include_directories(modules/stream-manager/src)
include_directories(modules/stream-client/src)

add_subdirectory(modules/stream-core)
add_subdirectory(modules/stream-api)
add_subdirectory(modules/stream-broker)
add_subdirectory(modules/stream-providers)
add_subdirectory(modules/stream-config)
add_subdirectory(modules/stream-router)
add_subdirectory(modules/stream-platform)
add_subdirectory(modules/stream-manager)
add_subdirectory(modules/stream-client)
add_subdirectory(examples/websocket)

