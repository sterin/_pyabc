include(FindThreads)

set(pyabc_source_files
    pyabc.cpp
    sys.h
    sys.cpp
    util.h
    util.cpp
)

pybind11_add_module(_pyabc ${pyabc_source_files})
target_compile_features(_pyabc PUBLIC cxx_std_17)
target_link_libraries(_pyabc PUBLIC libabc-pic Threads::Threads)

pybind11_add_module(_pyabc-static STATIC ${pyabc_source_files})
target_compile_features(_pyabc-static PUBLIC cxx_std_17)
target_compile_definitions(_pyabc-static PUBLIC PYABC_EMBEDDED_MODULE)
target_link_libraries(_pyabc-static PUBLIC libabc Threads::Threads)

add_executable(pyabc.exe main.cpp ${pyabc_source_files})
target_link_libraries(pyabc.exe PRIVATE _pyabc-static pybind11::embed) # _pyzz-static)
