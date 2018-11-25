cmake_minimum_required(VERSION 3.10)

list(APPEND BOOST_COMPONENTS
        thread
        date_time
        system
        filesystem
        timer
        chrono
        program_options
        unit_test_framework
        locale
        )

find_package (Boost 1.68  REQUIRED COMPONENTS ${BOOST_COMPONENTS})
include_directories (SYSTEM ${Boost_INCLUDE_DIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -pthread  -lpthread -lboost_thread -lboost_system")


include_directories(modules/stream-api/src)
include_directories(modules/stream-core/src)
include_directories(modules/stream-io/src)
include_directories(modules/stream-providers/src)
include_directories(modules/stream-config/src)
include_directories(modules/stream-router/src)
include_directories(modules/stream-platform/src)

add_subdirectory(modules/stream-core)
add_subdirectory(modules/stream-api)
add_subdirectory(modules/stream-io)
add_subdirectory(modules/stream-providers)
add_subdirectory(modules/stream-config)
add_subdirectory(modules/stream-router)
add_subdirectory(modules/stream-platform)
add_subdirectory(examples/websocket)

