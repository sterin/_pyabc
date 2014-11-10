#include "pyabc.h"

#include "command.h"
#include "util.h"
#include "cex.h"
#include "sys.h"

#include <signal.h>

#include <base/main/main.h>

namespace pyabc
{
using namespace py;

ref<PyObject> n_ands()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);

    if ( pNtk && Abc_NtkIsStrash(pNtk) )
    {
        return Int_FromLong(Abc_NtkNodeNum(pNtk));
    }

    return None;
}

ref<PyObject> n_nodes()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);

    if ( pNtk )
    {
        return Int_FromLong(Abc_NtkNodeNum(pNtk));
    }

    return None;
}

ref<PyObject> n_pis()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    Abc_Ntk_t* pNtk = Abc_FrameReadNtk(pAbc);

    if (pNtk)
    {
        return Int_FromLong(Abc_NtkPiNum(pNtk));
    }

    return None;
}

ref<PyObject> n_pos()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    Abc_Ntk_t* pNtk = Abc_FrameReadNtk(pAbc);

    if (pNtk)
    {
        return Int_FromLong(Abc_NtkPoNum(pNtk));
    }

    return None;
}

ref<PyObject> n_latches()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);

    if ( pNtk )
    {
        return Int_FromLong(Abc_NtkLatchNum(pNtk));
    }

    return None;
}

ref<PyObject> n_levels()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);

    if ( pNtk )
    {
        return Int_FromLong(Abc_NtkLevel(pNtk));
    }

    return None;
}

ref<PyObject> n_area()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);

    if ( pNtk && Abc_NtkHasMapping(pNtk) )
    {
        return Float_FromDouble(Abc_NtkGetMappedArea(pNtk));
    }

    return None;
}

ref<PyObject> has_comb_model()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);

    return Bool_FromLong(pNtk && pNtk->pModel);
}

ref<PyObject> has_seq_model()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);

    return Bool_FromLong( pNtk && pNtk->pSeqModel );
}

ref<PyObject> n_bmc_frames()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    return Int_FromLong( Abc_FrameReadBmcFrames(pAbc) );
}

ref<PyObject> prob_status()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    return Int_FromLong( Abc_FrameReadProbStatus(pAbc) );
}
ref<PyObject> is_valid_cex()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);

    return Bool_FromLong( pNtk && Abc_FrameReadCex(pAbc) && Abc_NtkIsValidCex( pNtk, Abc_FrameReadCex(pAbc) ) );
}

ref<PyObject> is_true_cex()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);

    return Bool_FromLong( pNtk && Abc_FrameReadCex(pAbc) && Abc_NtkIsTrueCex( pNtk, Abc_FrameReadCex(pAbc) ) );
}

ref<PyObject> n_cex_pis()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();

    if( Abc_FrameReadCex(pAbc) )
    {
       return Int_FromLong( Abc_FrameReadCexPiNum( pAbc ));
    }

    return None;
}

ref<PyObject> n_cex_regs()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();

    if (Abc_FrameReadCex(pAbc))
    {
        return Int_FromLong(Abc_FrameReadCexRegNum(pAbc));
    }

    return None;
}

ref<PyObject> cex_po()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();

    if( Abc_FrameReadCex(pAbc) )
    {
        return Int_FromLong(Abc_FrameReadCexPo( pAbc ));
    }

    return None;
}

ref<PyObject> cex_frame()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();

    if ( Abc_FrameReadCex(pAbc) )
    {
        return Int_FromLong( Abc_FrameReadCexFrame( pAbc ));
    }

    return None;
}

ref<PyObject> n_phases()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);

    return Int_FromLong( pNtk ? Abc_NtkPhaseFrameNum(pNtk) : 1 );
}

ref<PyObject> is_const_po( PyObject* args, PyObject* kwds )
{
    static char *kwlist[] = { "iPoNum", NULL };
    int iPoNum;
    Arg_ParseTupleAndKeywords(args, kwds, "i:is_const_po", kwlist, &iPoNum);

    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    return Bool_FromLong( Abc_FrameCheckPoConst( pAbc, iPoNum ) );
}

void create_abc_array(PyObject* seq)
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    Vec_Int_t *vObjIds = Abc_FrameReadObjIds(pAbc);

    Vec_IntClear( vObjIds );

    for_iterator(seq, [&](PyObject* item)
    {
        Vec_IntPush( vObjIds, Int_AsLong(item));
    });
}

ref<PyObject> eq_classes()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    Vec_Ptr_t *vPoEquivs = Abc_FrameReadPoEquivs(pAbc);

    if( ! vPoEquivs )
    {
        return None;
    }

    ref<PyObject> classes = List_New( Vec_PtrSize(vPoEquivs) );

    int i;
    Vec_Int_t* v;
    Vec_PtrForEachEntry( Vec_Int_t*, vPoEquivs, v, i )
    {
        ref<PyObject> l = List_New( Vec_IntSize(v));

        int elem, i;
        Vec_IntForEachEntry( v, elem, i)
        {
            List_SetItem( l, i, Int_FromLong(elem) );
        }

        PyList_SetItem( classes, i, l );
    }

    return classes;
}

void
init()
{
    Abc_Start();

    static PyMethodDef pyzz_methods[] =
    {
        PYTHONWRAPPER_FUNC_NOARGS(n_ands, 0, ""),
        PYTHONWRAPPER_FUNC_NOARGS(n_nodes, 0, ""),
        PYTHONWRAPPER_FUNC_NOARGS(n_pis, 0, ""),
        PYTHONWRAPPER_FUNC_NOARGS(n_pos, 0, ""),
        PYTHONWRAPPER_FUNC_NOARGS(n_latches, 0, ""),
        PYTHONWRAPPER_FUNC_NOARGS(n_levels, 0, ""),

        PYTHONWRAPPER_FUNC_NOARGS(n_area, 0, ""),
        PYTHONWRAPPER_FUNC_NOARGS(has_comb_model, 0, ""),
        PYTHONWRAPPER_FUNC_NOARGS(has_seq_model, 0, ""),
        PYTHONWRAPPER_FUNC_NOARGS(n_bmc_frames, 0, ""),
        PYTHONWRAPPER_FUNC_NOARGS(prob_status, 0, ""),
        PYTHONWRAPPER_FUNC_NOARGS(is_valid_cex, 0, ""),
        PYTHONWRAPPER_FUNC_NOARGS(is_true_cex, 0, ""),
        PYTHONWRAPPER_FUNC_NOARGS(n_cex_pis, 0, ""),
        PYTHONWRAPPER_FUNC_NOARGS(n_cex_regs, 0, ""),
        PYTHONWRAPPER_FUNC_NOARGS(cex_po, 0, ""),
        PYTHONWRAPPER_FUNC_NOARGS(cex_frame, 0, ""),
        PYTHONWRAPPER_FUNC_NOARGS(n_phases, 0, ""),
        PYTHONWRAPPER_FUNC_KEYWORDS(is_const_po, 0, ""),

        PYTHONWRAPPER_FUNC_NOARGS(cex_get_vector, 0, ""),
        PYTHONWRAPPER_FUNC_NOARGS(cex_get, 0, ""),
        PYTHONWRAPPER_FUNC_NOARGS(status_get_vector, 0, ""),

        PYTHONWRAPPER_FUNC_O(run_command, 0, ""),
        PYTHONWRAPPER_FUNC_O(set_command_callback, 0, ""),
        PYTHONWRAPPER_FUNC_KEYWORDS(register_command, 0, ""),

        PYTHONWRAPPER_FUNC_O(atfork_child_add, 0, "after a fork(), close fd in the child process"),
        PYTHONWRAPPER_FUNC_O(atfork_child_remove, 0, "remove fd from the list of file descriptors to be closed after fork()"),

        PYTHONWRAPPER_FUNC_O(add_sigchld_fd, 0, "add a file descriptor to receive a byte every time SIGCHLD is recieved "),
        PYTHONWRAPPER_FUNC_O(remove_sigchld_fd, 0, ""),

        { 0 }
    };

    borrowed_ref<PyObject> mod = InitModule3(
        "_pyabc",
        pyzz_methods,
        "Python interface to ABC"
    );

    cex::initialize(mod);

    sys_init();
}

} // namespace pyabc

extern "C"
void init_pyabc()
{
    pyabc::init();
}
