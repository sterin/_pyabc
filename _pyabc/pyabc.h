#ifndef pyabc_pyabc__H
#define pyabc_pyabc__H

#include <pywrapper.h>
#include <pywrapper_types.h>
#include <pywrapper_thread.h>

#include <misc/util/abc_namespaces.h>

ABC_NAMESPACE_HEADER_START
ABC_NAMESPACE_HEADER_END

namespace pyabc
{
using namespace py;
ABC_NAMESPACE_USING_NAMESPACE

void init();

} // namespace pyabc

#endif // ifndef pyabc_pyabc__H
