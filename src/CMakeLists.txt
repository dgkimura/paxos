cmake_minimum_required(VERSION 3.4)
project(paxos.src)

set(SOURCES
    callback.cpp
    decree.cpp
    logging.cpp
    messages.cpp
    paxos.cpp
    receiver.cpp
    replicaset.cpp
    roles.cpp
    sender.cpp
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_library(paxos SHARED ${SOURCES})
target_link_libraries(paxos
    PRIVATE
        ${boost_STATIC_LIB}
)
include_directories(${DEPENDENCIES_boost_DIR}/include)
add_dependencies(paxos Boost)

set_property(TARGET paxos PROPERTY CXX_STANDARD 11)

set(paxos_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
