#!/bin/bash
#
# Setup the ABC/Py environment and run the ABC/Py executable
# (ABC/Py stands for ABC with embedded Python)
#
# ABC/Py expects the following directory structure
#
# abc_root/
#   bin/
#     abc - this script
#     pyabc-exe - the ABC executable
#	  *.sh - engine scripts
#   lib/
#     pyabc/pyzz/pyliveness/pyaig/super_prove/etc - python libraries
#     python_library.zip - The Python standard library. Only if not using the system Python interpreter.
#     *.so - Python extensions, Only if not using the system Python interpreter.
#   scripts/
#     *.py - default directory for python scripts
#             

abspath()
{
    local cwd="$(pwd)"
    cd "$1"
    echo "$(pwd)"
    cd "${cwd}"
}

self=$0

self_dir=$(dirname "${self}")
self_dir=$(abspath "${self_dir}")

abc_root=$(dirname "${self_dir}")
abc_exe="${abc_root}/bin/abc.exe"

export PATH="${abc_root}/bin:$PATH"

export PYTHONPATH="${abc_root}/lib":"${PYTHONPATH}"
export LD_LIBRARY_PATH="${abc_root}/lib":"${LD_LIBRARY_PATH}"

if [ -f "${abc_root}/lib/python_library.zip" ] ; then
    export PYTHONHOME="${abc_root}"
    export PYTHONPATH="${abc_root}/lib/python_library.zip":"${PYTHONPATH}"
fi

if [ "$1" = "--debug" ]; then
    shift
    abc_debugger="$1"
    shift
    
    echo export PYTHONHOME=$PYTHONHOME
    echo export PYTHONPATH=$PYTHONPATH
    echo export LD_LIBRARY_PATH=$LD_LIBRARY_PATH
fi

if [ -f "${abc_root}/lib/super_prove/abc.rc" ] ; then
    exec ${abc_debugger} "${abc_exe}" -s -Q "source -s ${abc_root}/lib/super_prove/abc.rc" "$@"
else
    exec ${abc_debugger} "${abc_exe}" -s "$@"
fi
