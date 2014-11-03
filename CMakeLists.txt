cmake_minimum_required(VERSION 2.8.4)

project(xxx)

find_package(PythonLibs REQUIRED)
include_directories(${PYTHON_INCLUDE_DIRS})

include_directories( ../pywrapper/src )

include_directories( ../abc/src )
link_directories( ../abc )

add_definitions( -std=c++11 -Wno-write-strings -DLIN64 )

set(SOURCE_FILES pyabc.cpp command.cpp sys.cpp cex.cpp  util.cpp )

PYTHON_ADD_MODULE( _pyabcx SHARED ${SOURCE_FILES} )
target_link_libraries( _pyabcx abc readline )