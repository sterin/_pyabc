#include <pybind11/stl.h>
#include <pybind11/pybind11.h>

#include <base/main/main.h>
#include <base/main/mainInt.h>

#include "sys.h"

#include <iostream>

namespace py = pybind11;

namespace pyabc
{

int n_ands()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);

    if ( pNtk && Abc_NtkIsStrash(pNtk) )
    {
        return Abc_NtkNodeNum(pNtk);
    }

    return -1;
}

int n_nodes()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);

    if ( pNtk )
    {
        return Abc_NtkNodeNum(pNtk);
    }

    return -1;
}

int n_pis()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    Abc_Ntk_t* pNtk = Abc_FrameReadNtk(pAbc);

    if (pNtk)
    {
        return Abc_NtkPiNum(pNtk);
    }

    return -1;
}

int n_pos()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    Abc_Ntk_t* pNtk = Abc_FrameReadNtk(pAbc);

    if (pNtk)
    {
        return Abc_NtkPoNum(pNtk);
    }

    return -1;
}

int n_latches()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);

    if ( pNtk )
    {
        return Abc_NtkLatchNum(pNtk);
    }

    return -1;
}

int n_levels()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);

    if ( pNtk )
    {
        return Abc_NtkLevel(pNtk);
    }

    return -1;
}

double n_area()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);

    if ( pNtk && Abc_NtkHasMapping(pNtk) )
    {
        return Abc_NtkGetMappedArea(pNtk);
    }

    return -1;
}

bool has_comb_model()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);

    return pNtk && pNtk->pModel;
}

bool has_seq_model()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);

    return  pNtk && pNtk->pSeqModel ;
}

int n_bmc_frames()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    return Abc_FrameReadBmcFrames(pAbc);
}

int prob_status()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    return Abc_FrameReadProbStatus(pAbc);
}

bool is_valid_cex()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);

    return pNtk && Abc_FrameReadCex(pAbc) && Abc_NtkIsValidCex( pNtk, static_cast<Abc_Cex_t_*>(Abc_FrameReadCex(pAbc)) );
}

bool is_true_cex()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);

    return pNtk && Abc_FrameReadCex(pAbc) && Abc_NtkIsTrueCex( pNtk, static_cast<Abc_Cex_t_*>(Abc_FrameReadCex(pAbc)) );
}

int n_cex_pis()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();

    if( Abc_FrameReadCex(pAbc) )
    {
       return Abc_FrameReadCexPiNum( pAbc );
    }

    return -1;
}

int n_cex_regs()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();

    if (Abc_FrameReadCex(pAbc))
    {
        return Abc_FrameReadCexRegNum(pAbc);
    }

    return -1;
}

int cex_po()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();

    if( Abc_FrameReadCex(pAbc) )
    {
        return Abc_FrameReadCexPo( pAbc );
    }

    return -1;
}

int cex_frame()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();

    if ( Abc_FrameReadCex(pAbc) )
    {
        return Abc_FrameReadCexFrame( pAbc );
    }

    return -1;
}

int n_phases()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);

    return pNtk ? Abc_NtkPhaseFrameNum(pNtk) : 1;
}

int is_const_po(int iPoNum)
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    return Abc_FrameCheckPoConst( pAbc, iPoNum );
}

void create_abc_array(const std::vector<int> seq)
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    Vec_Int_t *vObjIds = Abc_FrameReadObjIds(pAbc);

    Vec_IntClear( vObjIds );

    for(const auto item : seq)
    {
        Vec_IntPush( vObjIds, item);
    }
}

int pyabc_array_read_entry(int i)
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    Vec_Int_t *vObjIds = Abc_FrameReadObjIds(pAbc);

    if( !vObjIds )
        return -1;

    return Vec_IntEntry( vObjIds, i );
}

std::vector<int> to_vector(Vec_Int_t* v)
{
    std::vector<int> res;
    
    int elem, i;
    Vec_IntForEachEntry( v, elem, i)
    {
        res.push_back(elem);
    }

    return res;    
}

std::variant<nullptr_t, std::vector<std::vector<int>>> eq_classes()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    Vec_Ptr_t *vPoEquivs = Abc_FrameReadPoEquivs(pAbc);

    if( ! vPoEquivs )
    {
        return nullptr;
    }

    std::vector<std::vector<int>> classes;

    int i;
    Vec_Int_t* v;

    Vec_PtrForEachEntry( Vec_Int_t*, vPoEquivs, v, i )
    {
        classes.push_back( to_vector(v) );
    }

    return classes;
}

std::variant<nullptr_t, std::vector<int>> co_supp(int iCo)
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);

    if ( !pNtk )
    {
        return nullptr;
    }

    Vec_Int_t* vSupp = Abc_NtkNodeSupportInt( pNtk, iCo );

    if( !vSupp )
    {
        return nullptr;
    }

    auto co_supp = to_vector( vSupp );

    Vec_IntFree( vSupp );

    return co_supp;
}

std::variant<nullptr_t, int> _is_func_iso(int iCo1, int iCo2, int fCommon)
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);

    if ( !pNtk )
    {
        return nullptr;
    }

    return Abc_NtkFunctionalIso( pNtk, iCo1, iCo2, fCommon );
}

class cex
{
public:

    cex(Abc_Cex_t* pCex)
    {
        if (pCex)
        {
            _pCex = Abc_CexDup(pCex, -1);
        }
    }

    ~cex()
    {
        Abc_CexFree(_pCex);
    }

    int n_regs()
    {
        return _pCex->nRegs;
    }

    int n_pis()
    {
        return _pCex->nPis;
    }

    int po()
    {
        return _pCex->iPo;
    }

    int frame()
    {
        return _pCex->iFrame;
    }

    void put()
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

private:

    Abc_Cex_t* _pCex;
};

using cex_return_type = std::variant<nullptr_t, bool, cex>;

std::variant<nullptr_t, std::vector<cex_return_type>> cex_get_vector()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    Vec_Ptr_t* vCexVec = Abc_FrameReadCexVec(pAbc);

    if( ! vCexVec )
    {
        return nullptr;
    }

    std::vector<cex_return_type> res;

    for(int i=0; i<Vec_PtrSize(vCexVec) ; i++)
    {
        Abc_Cex_t* pCex = static_cast<Abc_Cex_t*>(Vec_PtrEntry(vCexVec, i));

        if( ! pCex )
        {
            res.push_back(nullptr);
        }
        else if ( pCex == reinterpret_cast<Abc_Cex_t*>(1) )
        {
            res.push_back(true);
        }
        else
        {
            res.push_back(cex{pCex});
        }
    }

    return res;
}

cex_return_type cex_get()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    Abc_Cex_t* pCex = static_cast<Abc_Cex_t_*>(Abc_FrameReadCex(pAbc));

    if( ! pCex )
    {
        return nullptr;
    }

    if ( pCex == reinterpret_cast<Abc_Cex_t*>(1) )
    {
        return true;
    }

    return cex{pCex};
}

std::variant<nullptr_t, std::vector<int>> status_get_vector()
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    Vec_Int_t* vStatusVec = Abc_FrameReadStatusVec(pAbc);

    if( ! vStatusVec )
    {
        return nullptr;
    }

    std::vector<int> res;

    for(int i=0; i<Vec_IntSize(vStatusVec) ; i++)
    {
        res.push_back( Vec_IntEntry( vStatusVec, i ) );
    }

    return res;
}

py::function python_frame_done_callback;

void frame_done_callback(int frame, int po, int status)
{
    try
    {
        py::gil_scoped_acquire acquire;
        python_frame_done_callback(frame, po, status);
    }
    catch(...)
    {
    }
}

py::function set_frame_done_callback(py::function callback)
{
    py::function prev = python_frame_done_callback;
    python_frame_done_callback = py::reinterpret_borrow<py::function>(callback);
    return prev;
}

py::function python_command_callback;

static int abc_command_callback(Abc_Frame_t* pAbc, int argc, char** argv)
{
    try
    {
        std::vector<const char*> args;

        for(int i=0; i<argc; i++)
        {
            args.push_back( argv[i]);
        }
        
        py::gil_scoped_acquire acquire;
        py::int_ res = python_command_callback(args);
        return res;
    }
    catch(...)
    {
        return -1;
    }
}

void set_command_callback(py::function callback)
{
    python_command_callback = py::reinterpret_borrow<py::function>(callback);;
}

void register_command(char* sGroup, char* sName, int fChanges)
{
    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();
    Cmd_CommandAdd( pAbc, sGroup, sName, static_cast<Cmd_CommandFuncType>(abc_command_callback), fChanges);
}

class abc_func_on_frame_done_scope
{
public:

    abc_func_on_frame_done_scope(Abc_Frame_Callback_BmcFrameDone_Func func)
    {
        pAbc->pFuncOnFrameDone = func;
    }

    ~abc_func_on_frame_done_scope()
    {
        pAbc->pFuncOnFrameDone = prev;
    }

private:

    Abc_Frame_t* pAbc{ Abc_FrameGetGlobalFrame() };
    Abc_Frame_Callback_BmcFrameDone_Func prev{ pAbc->pFuncOnFrameDone };
};

int run_command(const char* cmd)
{
    py::gil_scoped_release release;

    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();

    if( python_frame_done_callback && !python_frame_done_callback.is_none() )
    {
        abc_func_on_frame_done_scope scope{frame_done_callback};
        return Cmd_CommandExecute(pAbc, cmd);
    }
    else
    {
        return Cmd_CommandExecute(pAbc, cmd);
    }
}

} // namespace pyabc

#ifdef PYABC_EMBEDDED_MODULE
#include <pybind11/embed.h>
PYBIND11_EMBEDDED_MODULE(_pyabc, m) {
#else
PYBIND11_MODULE(_pyabc, m) {
#endif
    m.doc() = "pyABC - a simple Python interface to ABC";

    m.def("n_ands", &pyabc::n_ands, "n_ands");
    m.def("n_nodes", &pyabc::n_nodes, "n_nodes");
    m.def("n_pis", &pyabc::n_pis, "n_pis");
    m.def("n_pos", &pyabc::n_pos, "n_pos");
    m.def("n_latches", &pyabc::n_latches, "n_latches");
    m.def("n_levels", &pyabc::n_levels, "n_levels");
    m.def("n_area", &pyabc::n_area, "n_area");
    m.def("has_comb_model", &pyabc::has_comb_model, "has_comb_model");
    m.def("has_seq_model", &pyabc::has_seq_model, "has_seq_model");
    m.def("n_bmc_frames", &pyabc::n_bmc_frames, "n_bmc_frames");
    m.def("prob_status", &pyabc::prob_status, "prob_status");
    m.def("is_valid_cex", &pyabc::is_valid_cex, "is_valid_cex");
    m.def("is_true_cex", &pyabc::is_true_cex, "is_true_cex");
    m.def("n_cex_pis", &pyabc::n_cex_pis, "n_cex_pis");
    m.def("n_cex_regs", &pyabc::n_cex_regs, "n_cex_regs");
    m.def("cex_po", &pyabc::cex_po, "cex_po");
    m.def("cex_frame", &pyabc::cex_frame, "cex_frame");
    m.def("n_phases", &pyabc::n_phases, "n_phases");
    m.def("is_const_po", &pyabc::is_const_po, "is_const_po");
    m.def("eq_classes", &pyabc::eq_classes, "eq_classes");
    m.def("co_supp", &pyabc::co_supp, "co_supp");
    m.def("_is_func_iso", &pyabc::_is_func_iso, "_is_func_iso");
    
    py::class_<pyabc::cex>(m, "CEX")
        .def("n_regs", &pyabc::cex::n_regs)
        .def("n_pis", &pyabc::cex::n_pis)
        .def("po", &pyabc::cex::po)
        .def("frame", &pyabc::cex::frame)
        .def("put", &pyabc::cex::put)
        ;

    m.def("cex_get_vector", &pyabc::cex_get_vector, "cex_get_vector");
    m.def("cex_get", &pyabc::cex_get, "cex_get");
    m.def("status_get_vector", &pyabc::status_get_vector, "status_get_vector");

    m.def("atfork_child_add", &pyabc::atfork_child_add, "after a fork(), close fd in the child process");
    m.def("atfork_child_remove", &pyabc::atfork_child_remove, "remove fd from the list of file descriptors to be closed after fork()");
    m.def("add_sigchld_fd", &pyabc::add_sigchld_fd, "add a file descriptor to receive a byte every time SIGCHLD is recieved ");
    m.def("remove_sigchld_fd", &pyabc::remove_sigchld_fd, "");

    m.def("set_frame_done_callback", &pyabc::set_frame_done_callback, "set_frame_done_callback");
    m.def("set_command_callback", &pyabc::set_command_callback, "set_command_callback");
    m.def("register_command", &pyabc::register_command, "register_command");
    m.def("run_command", &pyabc::run_command, "run_command");

    pyabc::sys_init();
}
