cmake_minimum_required(VERSION 3.4)
project(paxos.src)

set(SOURCES
    acceptor.cpp
    callback.cpp
    decree.cpp
    learner.cpp
    ledger.cpp
    paxos.cpp
    proposer.cpp
    receiver.cpp
    replicaset.cpp
    sender.cpp
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_library(paxos SHARED ${SOURCES})

set_property(TARGET paxos PROPERTY CXX_STANDARD 11)

set(paxos_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
