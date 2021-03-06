cmake_minimum_required(VERSION 2.8)

set(CURRENT_TARGET stream-manager)

add_compile_options(-std=c++14)

add_executable(${CURRENT_TARGET}
        src/manager.cpp
        src/router.hpp
        src/router.cpp
        src/users.hpp
        src/users.cpp
        src/groups.hpp
        src/groups.cpp
        src/subscriptions.hpp
        src/subscriptions.cpp
        src/connections.hpp
        src/connections.cpp
        src/devices.hpp
        src/devices.cpp
        src/platform.hpp
        src/platform.cpp
        )


target_link_libraries(
        ${CURRENT_TARGET}
        ${Boost_LIBRARIES}
        stream-core
        stream-config
        stream-api
        stream-io
        stream-providers
        stream-client
        stream-storage
        stream-manager-info
        dl
)
