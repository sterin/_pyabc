include(FindThreads)

set(pyabc_source_files pyabc.cpp command.cpp sys.cpp cex.cpp util.cpp)

pyabc_python_add_module(_pyabc SHARED ${pyabc_source_files} _pyabc.cpp)
target_link_libraries(_pyabc PUBLIC libabc-pic pywrapper Threads::Threads)

pyabc_python_add_module(_pyabc-static STATIC ${pyabc_source_files})
target_link_libraries(_pyabc-static PUBLIC libabc pywrapper Threads::Threads)

add_executable(pyabc.exe main.cpp ${pyabc_source_files})
target_link_libraries(pyabc.exe PRIVATE _pyabc-static _pyzz-static)
