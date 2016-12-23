cmake_minimum_required(VERSION 3.4)
project(paxos.src)

set(SOURCES
    bootstrap.cpp
    callback.cpp
    decree.cpp
    handler.cpp
    ledger.cpp
    logging.cpp
    messages.cpp
    parliament.cpp
    receiver.cpp
    replicaset.cpp
    roles.cpp
    sender.cpp
)

add_library(paxos SHARED ${SOURCES})
target_link_libraries(paxos
    PUBLIC
        ${boost_STATIC_LIB}
)
include_directories(${DEPENDENCIES_boost_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)
add_dependencies(paxos Boost)

set_property(TARGET paxos PROPERTY CXX_STANDARD 11)
set_target_properties(paxos
    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

set(paxos_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../include PARENT_SCOPE)
