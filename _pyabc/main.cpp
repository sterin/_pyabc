#include <pybind11/embed.h>

#include <misc/util/abc_global.h>

ABC_NAMESPACE_HEADER_START

int Abc_RealMain(int argc, char *argv[]);

ABC_NAMESPACE_HEADER_END

namespace py = pybind11;

int main(int argc, char *argv[])
{
    Py_NoSiteFlag = 1;

    py::scoped_interpreter guard;

    try
    {
        py::module_::import("pyabc");
    }
    catch (py::error_already_set &e)
    {
        fprintf(stderr, "error: could not load module pyabc:\n");
        fprintf(stderr, "%s\n", e.what());
    }

    return ABC_NAMESPACE_PREFIX Abc_RealMain(argc, argv);
}
