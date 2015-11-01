#ifndef pyabc_pyabc__H
#define pyabc_pyabc__H

#include <pywrapper.h>
#include <pywrapper_types.h>
#include <pywrapper_thread.h>

#ifdef ABC_NAMESPACE
#  define ABC_NAMESPACE_HEADER_START namespace ABC_NAMESPACE {
#  define ABC_NAMESPACE_HEADER_END }
#  define ABC_NAMESPACE_PREFIX ABC_NAMESPACE::
#else
#  define ABC_NAMESPACE_HEADER_START
#  define ABC_NAMESPACE_HEADER_END
#  define ABC_NAMESPACE_PREFIX
#endif

namespace pyabc
{

using namespace py;

void init();

} // namespace pyabc

#endif // ifndef pyabc_pyabc__H
