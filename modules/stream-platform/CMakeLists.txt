cmake_minimum_required(VERSION 2.8)

set(CURRENT_TARGET stream-platform)


add_compile_options(-std=c++14)

add_executable(${CURRENT_TARGET} src/platform.cpp)



target_link_libraries(
        ${CURRENT_TARGET}
        ${Boost_LIBRARIES}
        stream-core
        stream-config
        stream-api
        stream-io
        stream-providers
        stream-router
        dl
)
