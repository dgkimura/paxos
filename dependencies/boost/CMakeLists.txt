cmake_minimum_required(VERSION 3.4)
project(dependencies.boost)

include(ExternalProject)

if (APPLE)
    set(LINKFLAGS "linkflags=-stdlib=libc++")
    set(CXXFLAGS "cxxflags=-stdlib=libc++")
endif()

set(INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/boost-prefix")

ExternalProject_Add(boost
   URL "https://sourceforge.net/projects/boost/files/boost/1.68.0/boost_1_68_0.tar.gz"
   URL_HASH SHA256=da3411ea45622579d419bfda66f45cd0f8c32a181d84adfa936f5688388995cf
   UPDATE_COMMAND ""
   BUILD_IN_SOURCE 1
   CONFIGURE_COMMAND ./bootstrap.sh --with-libraries=filesystem,iostreams,log,program_options,serialization,system "${CXXFLAGS}" "--prefix=<INSTALL_DIR>"
   BUILD_COMMAND ./b2 link=static "${LINKFLAGS}" "${CXXFLAGS}" cxxflags=-fPIC cflags=-fPIC
   INSTALL_COMMAND ./b2 install "${LINKFLAGS}" "${CXXFLAGS}"
)

foreach(_MODULE filesystem iostreams log serialization system thread)
    add_library(dependency-boost_${_MODULE} STATIC IMPORTED GLOBAL)
    set_target_properties(dependency-boost_${_MODULE} PROPERTIES
      IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
      IMPORTED_LOCATION "${INSTALL_DIR}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}boost_${_MODULE}${CMAKE_SHARED_LIBRARY_SUFFIX}"
    )
endforeach(_MODULE)
