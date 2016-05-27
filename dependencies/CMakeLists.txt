cmake_minimum_required(VERSION 3.4)
project(paxos.dependencies)

add_subdirectory(boost)

set(DEPENDENCIES_boost_DIR ${DEPENDENCIES_boost_DIR} PARENT_SCOPE)
set(boost_STATIC_LIB ${boost_STATIC_LIB} PARENT_SCOPE)
