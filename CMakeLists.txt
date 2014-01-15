project(lemuri-session)

cmake_minimum_required(VERSION 2.8.6 FATAL_ERROR)
find_package(PkgConfig REQUIRED)
find_package(Qt5 COMPONENTS
    Core
    Gui
    Network
    DBus
)

set(CMAKE_BUILD_TYPE "Debug")

set(CMAKE_AUTOMOC ON)
# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Forbid in-tree building
if(${CMAKE_SOURCE_DIR} MATCHES ${CMAKE_BINARY_DIR})
      message(STATUS "Please do an out-of-tree build:")
      message(STATUS "rm -f CMakeCache.txt && mkdir build && cd build; cmake .. && make")
      message(FATAL_ERROR "In-tree-build detected!")
endif(${CMAKE_SOURCE_DIR} MATCHES ${CMAKE_BINARY_DIR})

add_definitions("-DLOCALSTATEDIR=\"${LOCALSTATEDIR}\"" -std=gnu++11)

set(CMAKE_INSTALL_LIBDIR "lib/${CMAKE_LIBRARY_ARCHITECTURE}" CACHE PATH "Output directory for libraries")

configure_file(config.h.in ${CMAKE_BINARY_DIR}/config.h)

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
    ${CMAKE_CURRENT_BINARY_DIR}
)

add_subdirectory(src)
