cmake_minimum_required(VERSION 3.4)
project(paxos.dependencies.boost)

include(ExternalProject)

set(boost_libraries
    filesystem
    iostreams
    serialization
    system
    thread
)

foreach(_M ${boost_libraries})
    set(boost_with_libraries ${boost_with_libraries} --with-${_M})
endforeach(_M)

ExternalProject_Add(Boost
    URL http://downloads.sourceforge.net/project/boost/boost/1.56.0/boost_1_56_0.tar.gz
    URL_MD5 8c54705c424513fa2be0042696a3a162
    SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/boost"
    INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}"
    CONFIGURE_COMMAND ./bootstrap.sh --prefix=<INSTALL_DIR>
    BUILD_COMMAND ./b2 ${boost_with_libraries}
    INSTALL_COMMAND ./b2 ${boost_with_libraries} install
    BUILD_IN_SOURCE 1
)

set(DEPENDENCIES_boost_DIR ${CMAKE_CURRENT_BINARY_DIR} PARENT_SCOPE)

foreach(_M ${boost_libraries})
    set(boost_STATIC_LIB ${boost_STATIC_LIB} ${CMAKE_CURRENT_BINARY_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}boost_${_M}${CMAKE_STATIC_LIBRARY_SUFFIX})
endforeach(_M)
set(boost_STATIC_LIB ${boost_STATIC_LIB} PARENT_SCOPE)
