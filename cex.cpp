#include "cex.h"

#include <base/main/main.h>
#include <misc/util/utilCex.h>

namespace pyabc
{

cex::cex(Abc_Cex_t* pCex) :
    _pCex(nullptr)
{
    if (pCex)
    {
        _pCex = Abc_CexDup(pCex, -1);
    }
}

cex::~cex()
{
    Abc_CexFree(_pCex);
}

void
cex::initialize(PyObject* module)
{
    static PyMethodDef methods[] = {

        PYTHONWRAPPER_METH_NOARGS(cex, n_pis, 0, ""),
        PYTHONWRAPPER_METH_NOARGS(cex, n_regs, 0, ""),
        PYTHONWRAPPER_METH_NOARGS(cex, po, 0, ""),
        PYTHONWRAPPER_METH_NOARGS(cex, frame, 0, ""),

        { NULL }  // sentinel
    };

    _type.tp_methods = methods;

    base::initialize("_pyabc.cex");
    add_to_module(module, "cex");
}

ref<PyObject> cex::n_regs()
{
    return Int_FromLong( _pCex->nRegs );

}

ref<PyObject> cex::n_pis()
{
    return Int_FromLong( _pCex->nPis );
}

ref<PyObject> cex::po()
{
    return Int_FromLong( _pCex->iPo );
}

ref<PyObject> cex::frame()
{
    return Int_FromLong( _pCex->iFrame );
}

void cex::put()
{
    if ( _pCex )
    {
        Abc_FrameSetCex( Abc_CexDup(_pCex, -1) );
    }
    else
    {
        Abc_FrameSetCex( nullptr );
    }
}

ref<PyObject> cex_get_vector()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    Vec_Ptr_t* vCexVec = Abc_FrameReadCexVec(pAbc);

    if( ! vCexVec )
    {
        return None;
    }

    ref<PyObject> res = List_New( Vec_PtrSize(vCexVec) );

    for(int i=0; i<Vec_PtrSize(vCexVec) ; i++)
    {
        Abc_Cex_t* pCex = static_cast<Abc_Cex_t*>(Vec_PtrEntry(vCexVec, i));

        if( ! pCex )
        {
            List_SetItem( res, i, None );
        }
        else if ( pCex == reinterpret_cast<Abc_Cex_t*>(1) )
        {
            List_SetItem( res, i, True );
        }
        else
        {
            List_SetItem( res, i, cex::build(pCex) );
        }
    }

    return res;
}

ref<PyObject> cex_get()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    Abc_Cex_t* pCex = Abc_FrameReadCex(pAbc);

    if( ! pCex )
    {
        return None;
    }

    if ( pCex == reinterpret_cast<Abc_Cex_t*>(1) )
    {
        return True;
    }

    return cex::build(pCex);
}

ref<PyObject> status_get_vector()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    Vec_Int_t* vStatusVec = Abc_FrameReadStatusVec(pAbc);

    if( ! vStatusVec )
    {
        return None;
    }

    ref<PyObject> res = List_New( Vec_IntSize(vStatusVec) );

    for(int i=0; i<Vec_IntSize(vStatusVec) ; i++)
    {
        List_SetItem( res, i, Int_FromLong( Vec_IntEntry( vStatusVec, i ) ) );
    }

    return res;
}

} // namespace pyabc
