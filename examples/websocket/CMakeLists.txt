cmake_minimum_required(VERSION 2.8)

set(CURRENT_TARGET stream-examples-wsclient)



add_executable(${CURRENT_TARGET}  main.cpp)



target_link_libraries(
        ${CURRENT_TARGET}
        ${Boost_LIBRARIES}
)
