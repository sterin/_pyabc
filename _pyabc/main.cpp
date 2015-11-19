#include "pyabc.h"

#include <misc/util/abc_global.h>

ABC_NAMESPACE_HEADER_START

int Abc_RealMain(int argc, char *argv[]);

ABC_NAMESPACE_HEADER_END

namespace pyzz
{
void zz_init();
}

int main(int argc, char *argv[])
{
    Py_NoSiteFlag = 1;

    PyImport_AppendInittab("_pyabc", pyabc::init);
    PyImport_AppendInittab("_pyzz", pyzz::zz_init);

    py::initialize interpreter(argv[0]);

    try
    {
        py::Import_ImportModule("pyabc");
    }
    catch(py::exception&)
    {
        fprintf( stderr, "error: could not load module pyabc:\n");
        PyErr_Print();
    }

    return ABC_NAMESPACE_PREFIX Abc_RealMain(argc, argv);
}
