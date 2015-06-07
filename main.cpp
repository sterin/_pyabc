#include "pyabc.h"

extern "C"
int Abc_RealMain(int argc, char* argv[]);

int main(int argc, char* argv[])
{
    Py_NoSiteFlag = 1;

    PyImport_AppendInittab("_pyabc", pyabc::init);

    py::initialize interpreter(argv[0]);

    py::Import_ImportModule("pyabc");

    return Abc_RealMain( argc, argv );
}
