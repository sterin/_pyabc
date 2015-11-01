include(FindThreads)

set(pyabc_source_files pyabc.cpp command.cpp sys.cpp cex.cpp)

python_add_module(_pyabc SHARED ${pyabc_source_files} _pyabc.cpp)
target_link_libraries(_pyabc libabc-pic pywrapper Threads::Threads)

python_add_module(_pyabc-static STATIC ${pyabc_source_files})
target_link_libraries(_pyabc-static libabc pywrapper Threads::Threads)

add_executable(pyabc_exe main.cpp ${pyabc_source_files})
target_link_libraries(pyabc_exe _pyabc-static _pyzz-static)
