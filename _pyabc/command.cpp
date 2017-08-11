#include "command.h"

#include <base/main/main.h>
#include <base/main/mainInt.h>

#include <stdio.h>

namespace pyabc
{

namespace
{

ref<PyObject> python_frame_done_callback{ py::None };

void frame_done_callback(int frame, int po, int status)
{
    try
    {
        gil_state_ensure scope;
        Object_CallFunction(python_frame_done_callback, "(iii)", frame, po, status);
    }
    catch(...)
    {
    }
}

} // unnamed namespace

ref<PyObject> run_command(PyObject* arg)
{
    const char* cmd = String_AsString(arg);

    int rc;

    {
        enable_threads scope;

        Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();

        if( python_frame_done_callback && python_frame_done_callback != py::None )
        {
            auto old_callback = pAbc->pFuncOnFrameDone;
            pAbc->pFuncOnFrameDone = frame_done_callback;
            rc = Cmd_CommandExecute(pAbc, cmd);
            pAbc->pFuncOnFrameDone = old_callback;
        }
        else
        {
            rc = Cmd_CommandExecute(pAbc, cmd);
        }
    }

    return Int_FromLong(rc);
}

ref<PyObject> set_frame_done_callback( PyObject* callback )
{
    ref<PyObject> prev = python_frame_done_callback;
    python_frame_done_callback = borrow(callback);
    return prev;
}

namespace
{

ref<PyObject> python_command_callback;

static int abc_command_callback(Abc_Frame_t * pAbc, int argc, char ** argv)
{
    try
    {
        gil_state_ensure scope;

        ref<PyObject> args = List_New(argc);

        for(int i=0; i<argc; i++)
        {
            List_SetItem(args, i, String_FromString(argv[i]) );
        }

        ref<PyObject> res = Object_CallFunction(python_command_callback, "(O)", args.get());

        return Int_AsLong(res);
    }
    catch(...)
    {
        return -1;
    }
}

} // unnamed namespace


void set_command_callback( PyObject* callback )
{
    python_command_callback = borrow(callback);
}


void register_command(PyObject* args, PyObject* kwds)
{
    static char *kwlist[] = { "sGroup", "sName", "fchanges", NULL };

    char* sGroup = nullptr;
    char* sName = nullptr;
    int fChanges = 0;

    Arg_ParseTupleAndKeywords(args, kwds, "ss|i:register_command", kwlist, &sGroup, &sName, &fChanges);

    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();

    Cmd_CommandAdd( pAbc, sGroup, sName, static_cast<Cmd_CommandFuncType>(abc_command_callback), fChanges);
}

} // namespace pyabc
