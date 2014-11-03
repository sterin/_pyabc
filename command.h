#ifndef pyabc_command__H
#define pyabc_command__H

#include "pyabc.h"

namespace pyabc
{

void set_command_callback( PyObject* callback );

ref<PyObject> run_command(PyObject* arg);
void register_command(PyObject* args, PyObject* kwds);

} // namespace pyabc

#endif // ifndef pyabc_command__H
