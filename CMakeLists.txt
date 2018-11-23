cmake_minimum_required(VERSION 3.10)

include_directories(modules/stream-core/src)
include_directories(modules/stream-io/src)
include_directories(modules/stream-providers/src)
include_directories(modules/stream-config/src)

add_subdirectory(modules/stream-core)
add_subdirectory(modules/stream-io)
add_subdirectory(modules/stream-providers)
add_subdirectory(modules/stream-config)

