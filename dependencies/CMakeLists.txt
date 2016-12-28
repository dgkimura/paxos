cmake_minimum_required(VERSION 3.4)
project(paxos.dependencies)

add_subdirectory(boost)

set(boost_STATIC_LIB ${boost_STATIC_LIB} PARENT_SCOPE)
