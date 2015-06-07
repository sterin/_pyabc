cmake_minimum_required(VERSION 2.8.4)

project(xxx)

set(CMAKE_CXX_COMPILER             "/usr/bin/clang++")

find_package(PythonLibs 2.7 REQUIRED)
include_directories(${PYTHON_INCLUDE_DIRS})

include_directories( ../pywrapper/src )

include_directories( ../abc/src )
link_directories( ../abc )

add_definitions( -std=c++11 -Wno-write-strings -DLIN64 )

set(SOURCE_FILES
    pyabc.cpp
    command.cpp
    sys.cpp
    cex.cpp
    )

PYTHON_ADD_MODULE( _pyabc SHARED ${SOURCE_FILES} )
target_link_libraries( _pyabc abc readline )

add_executable( abce ${SOURCE_FILES} main.cpp )
target_link_libraries( abce abc  python2.7 readline pthread dl )
