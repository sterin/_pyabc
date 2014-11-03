#ifndef pyabc_sys__H
#define pyabc_sys__H

#include "pyabc.h"

namespace pyabc
{

void atfork_child_add(PyObject *pyfd);
void atfork_child_remove(PyObject* pyfd);

void install_sigchld_handler(PyObject *pyfd);
void uninstall_sigchld_handler();

} // namespace pyabc

#endif // ifndef pyabc_sys__H
