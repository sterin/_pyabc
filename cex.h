#ifndef pyabc_cex__H
#define pyabc_cex__H

#include "pyabc.h"

ABC_NAMESPACE_HEADER_START
typedef struct Abc_Cex_t_ Abc_Cex_t;
ABC_NAMESPACE_HEADER_END

namespace pyabc
{

class cex :
    public type_base<cex>
{
public:

    cex(Abc_Cex_t* pCex);
    ~cex();

    static void initialize(PyObject* module);

    ref<PyObject> n_regs();
    ref<PyObject> n_pis();

    ref<PyObject> po();
    ref<PyObject> frame();

    void put();


private:

    ABC_NAMESPACE_PREFIX Abc_Cex_t* _pCex;
};

ref<PyObject> cex_get_vector();
ref<PyObject> cex_get();
ref<PyObject> status_get_vector();

} // namespace pyabc

#endif // ifndef pyabc_cex__H
