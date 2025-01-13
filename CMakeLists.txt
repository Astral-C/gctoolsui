cmake_minimum_required(VERSION 3.8)
project(gctoolsui)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig)
pkg_check_modules(GTKMM gtkmm-4.0)

link_directories(${GTKMM_LIBRARY_DIRS})

add_subdirectory(lib/gctoolsplusplus)

file(GLOB_RECURSE GCTOOLSUI_SRC
    "lib/gctoolsplusplus/lib/bStream/bstream.cpp"
    "src/*.cpp"
    "src/*.c"
    "include/*.hpp"
    "include/*.h"
)

configure_file(ui/gctools.ui ${CMAKE_CURRENT_BINARY_DIR}/gctools.ui COPYONLY)
add_executable(gctoolsui ${GCTOOLSUI_SRC})
target_include_directories(gctoolsui PUBLIC include lib/gctoolsplusplus/lib/bStream lib/gctoolsplusplus/include ${GTKMM_INCLUDE_DIRS})

target_link_libraries(gctoolsui PUBLIC gctools++ ${GTKMM_LIBRARIES})