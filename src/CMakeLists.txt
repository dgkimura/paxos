cmake_minimum_required(VERSION 3.4)
project(paxos.src)

set(SOURCES
    callback.cpp
    decree.cpp
    handler.cpp
    ledger.cpp
    logging.cpp
    messages.cpp
    parliament.cpp
    replicaset.cpp
    roles.cpp
    sender.cpp
    server.cpp
)

add_library(paxos SHARED ${SOURCES})
add_definitions(-DBOOST_LOG_DYN_LINK)
target_link_libraries(paxos
    PUBLIC
        dependency-boost_filesystem
        dependency-boost_serialization
        dependency-boost_system
        dependency-boost_log
        dependency-boost_iostreams
        dependency-boost_thread
)
include_directories(${CMAKE_BINARY_DIR}/dependencies/boost/boost-prefix/include)
include_directories(${CMAKE_SOURCE_DIR}/include)
add_dependencies(paxos boost)

set_property(TARGET paxos PROPERTY CXX_STANDARD 11)
set_target_properties(paxos
    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)
