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
        src/groups.cpp)


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
        dl
)
