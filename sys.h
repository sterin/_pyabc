#ifndef pyabc_sys__H
#define pyabc_sys__H

#include "pyabc.h"

namespace pyabc
{

void atfork_child_add(PyObject *pyfd);
void atfork_child_remove(PyObject* pyfd);

void add_sigchld_fd(PyObject *pyfd);
void remove_sigchld_fd(PyObject *pyfd);

void sys_init();

} // namespace pyabc

#endif // ifndef pyabc_sys__H
