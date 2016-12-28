cmake_minimum_required(VERSION 3.4)
project(paxos.unittests)

include(ExternalProject)

ExternalProject_Add(
    gtest
    URL https://github.com/google/googletest/archive/release-1.7.0.zip
    URL_HASH SHA256=b58cb7547a28b2c718d1e38aee18a3659c9e3ff52440297e965f5edffe34b6d0
    UPDATE_COMMAND ""
    INSTALL_COMMAND ""
)

set(SOURCES
    callback_unittest.cpp
    context_unittest.cpp
    decree_unittest.cpp
    fields_unittest.cpp
    handler_unittest.cpp
    ledger_unittest.cpp
    messages_unittest.cpp
    queue_unittest.cpp
    replicaset_unittest.cpp
    roles_unittest.cpp
    serialization_unittest.cpp
)

add_executable(all_unittests ${SOURCES})
add_dependencies(all_unittests gtest)

set_property(TARGET all_unittests PROPERTY CXX_STANDARD 11)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if (UNIX AND NOT APPLE)
    # XXX: GCC link order matters. Ensure that 'rt' links after 'pthread'.
    set(PLATFORM_LIBRARIES rt)
endif()

target_link_libraries(all_unittests
    PRIVATE
        paxos
        ${CMAKE_CURRENT_BINARY_DIR}/gtest-prefix/src/gtest-build/${CMAKE_STATIC_LIBRARY_PREFIX}gtest_main${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${CMAKE_CURRENT_BINARY_DIR}/gtest-prefix/src/gtest-build/${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX}
        Threads::Threads
        ${PLATFORM_LIBRARIES}
)

include_directories(${CMAKE_BINARY_DIR}/unittests/gtest-prefix/src/gtest/include)
include_directories(${CMAKE_BINARY_DIR}/dependencies/boost/include)
include_directories(${CMAKE_SOURCE_DIR}/include)
