cmake_minimum_required(VERSION 3.4)
project(paxos)

set(SOURCES
    acceptor.cpp
    proposer.cpp
    learner.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_library(paxos SHARED ${SOURCES})
