cmake_minimum_required(VERSION 3.5)

#set(CMAKE_SYSTEM_NAME Linux)
#set(CMAKE_SYSTEM_VERSION 1)
#set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER $ENV{CC})
set(CMAKE_CXX_COMPILER $ENV{CXX})

set(Boost_USE_STATIC_LIBS ON)

list(APPEND BOOST_COMPONENTS
        thread
        system
        locale
        timer
        filesystem
        serialization
        program_options
        )

find_package(Boost 1.68 REQUIRED COMPONENTS ${BOOST_COMPONENTS})
include_directories(${Boost_INCLUDE_DIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSQLITE_THREADSAFE=1 -DBOOST_VARIANT_USE_RELAXED_GET_BY_DEFAULT -DBOOST_ASIO_HAS_MOVE -DCXXOPTS_NO_RTTI" CACHE STRING "compile flags" FORCE)


if (NOT CMAKE_BUILD_TYPE)
      set(CMAKE_BUILD_TYPE Release)
endif ()

#set(CMAKE_CXX_FLAGS "-Wall -Wextra")
#set(CMAKE_CXX_FLAGS_DEBUG "-g")
 set(CMAKE_CXX_FLAGS_RELEASE "-O3")

include_directories(modules/stream-api/src)
include_directories(modules/stream-core/src)
include_directories(modules/stream-broker/src)
include_directories(modules/stream-providers/src)
include_directories(modules/stream-config/src)
include_directories(modules/stream-platform/src)
include_directories(modules/stream-manager/src)
include_directories(modules/stream-client/src)
include_directories(modules/stream-settings/src)
include_directories(modules/stream-storage/src)
include_directories(modules/stream-crypt/src)
include_directories(modules/stream-device/src)
include_directories(modules/stream-manager-info/src)

add_subdirectory(modules/stream-core)
add_subdirectory(modules/stream-api)
add_subdirectory(modules/stream-broker)
add_subdirectory(modules/stream-providers)
add_subdirectory(modules/stream-config)
add_subdirectory(modules/stream-platform)
add_subdirectory(modules/stream-manager)
add_subdirectory(modules/stream-client)
add_subdirectory(modules/stream-settings)
add_subdirectory(modules/stream-storage)
add_subdirectory(modules/stream-crypt)
add_subdirectory(modules/stream-device)
add_subdirectory(modules/stream-manager-info)
add_subdirectory(examples/websocket)

