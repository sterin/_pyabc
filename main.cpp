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

    py::initialize interpreter(argv[0]);

    py::Import_ImportModule("pyabc");

    return ABC_NAMESPACE_PREFIX Abc_RealMain(argc, argv);
}
