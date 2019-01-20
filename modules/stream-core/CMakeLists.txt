cmake_minimum_required(VERSION 2.8)

set(PROJECT stream-core)

project(${PROJECT} CXX)

add_compile_options(-std=c++14)

message (STATUS "${PROJECT_NAME} : CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

add_compile_options(
        -v
)

set(HEADER_FILES
        src/metadata.hpp
        src/actor/actor.hpp
        src/intrusive_ptr.hpp
        src/actor/abstract_actor.hpp
        src/forwards.hpp
        src/actor/actor_address.hpp
        src/actor/basic_actor.hpp
        src/messaging/message.hpp
        src/messaging/any.hpp
        src/messaging/message_header.hpp
        src/messaging/mail_box.hpp
        src/messaging/blocking_mail_queue.hpp
        src/behavior/type_action.hpp
        src/behavior/context.hpp
        src/behavior/abstract_action.hpp
        src/behavior/reactions.hpp
        src/environment/environment.hpp
        src/environment/abstract_environment.hpp
        src/environment/cooperation.hpp
        src/environment/group.hpp
        src/environment/abstract_group.hpp
        src/environment/storage_space.hpp
        src/environment/adjacency_list.hpp
        src/executor/coordinator.hpp
        src/executor/execution_device.hpp
        src/executor/worker.hpp
        src/executor/executable.hpp
        src/executor/abstract_coordinator.hpp
        src/executor/policy/work_sharing.hpp
        src/channel/channel.hpp
        src/channel/abstract_channel.hpp
        src/standard_handlers/add_channel.hpp
        src/standard_handlers/skip.hpp
        src/standard_handlers/sync_contacts.hpp
        src/actor/local_actor.hpp
        src/actor/blocking_actor.hpp
        src/actor/async_actor.hpp
        src/queue.hpp
        )

set(SOURCE_FILES
        src/actor/actor.cpp
        src/actor/abstract_actor.cpp
        src/actor/actor_address.cpp
        src/actor/async_actor.cpp
        src/actor/blocking_actor.cpp
        src/actor/local_actor.cpp
        src/behavior/context.cpp
        src/behavior/type_action.cpp
        src/behavior/reactions.cpp
        src/behavior/abstract_action.cpp src/channel/channel.cpp src/channel/abstract_channel.cpp src/environment/abstract_group.cpp src/environment/cooperation.cpp src/environment/environment.cpp src/environment/group.cpp src/environment/storage_space.cpp src/messaging/message.cpp src/messaging/blocking_mail_queue.cpp src/messaging/message_header.cpp src/standard_handlers/add_channel.cpp src/standard_handlers/skip.cpp "src/standard_handlers/sync_contacts.cpp" ../stream-broker/src/actor/platform.hpp ../stream-broker/src/actor/broker.cpp ../stream-providers/src/http/http_server.hpp ../stream-providers/src/http/http_server.cpp)


add_library(
        ${PROJECT}
        ${SOURCE_FILES}
        ${HEADER_FILES}
)

set_target_properties( ${PROJECT} PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(    ${PROJECT}        ${Boost_LIBRARIES})

target_include_directories(
        ${PROJECT}
        PUBLIC
)