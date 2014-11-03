from setuptools import setup, Extension

ext = Extension(
    'pyabc._pyabc',
    ['pyabc.cpp', 'command.cpp', 'cex.cpp', 'sys.cpp', 'util.cpp'],

    include_dirs=['../abc/src', '../pywrapper/src'],
    define_macros=[('LIN64', 1)],
    extra_compile_args=['-std=c++11', '-Wno-write-strings'],

    library_dirs=['../abc'],
    libraries=['abc'],
    )

setup(
    name='pyabc',
    version='1.0',
    ext_modules=[ext],
    py_modules=['pyabc'],
    )
