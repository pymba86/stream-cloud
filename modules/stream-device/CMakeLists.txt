cmake_minimum_required(VERSION 2.8)

set(CURRENT_TARGET stream-device)


add_compile_options(-std=c++14)

add_executable(${CURRENT_TARGET}
        src/device.cpp
        src/manager.hpp
        src/manager.cpp
        src/control.hpp
        src/control.cpp
        )



target_link_libraries(
        ${CURRENT_TARGET}
        ${Boost_LIBRARIES}
        stream-core
        stream-config
        stream-settings
        stream-api
        stream-io
        stream-providers
        stream-client
        dl
)
