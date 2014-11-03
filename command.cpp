#include "command.h"

#include <base/main/main.h>

#include <stdio.h>

namespace pyabc
{

ref<PyObject> run_command(PyObject* arg)
{
    const char* cmd = String_AsString(arg);

   int rc;

    {
        enable_threads scope;

        Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();

        rc = Cmd_CommandExecute(pAbc, cmd);
    }

    return Int_FromLong(rc);
}

static ref<PyObject> internal_python_command_callback;

void set_command_callback( PyObject* callback )
{
    internal_python_command_callback = borrow(callback);
}

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

        ref<PyObject> res = Object_CallFunction(internal_python_command_callback, "(O)", args.get());

        return Int_AsLong(res);
    }
    catch(...)
    {
        return -1;
    }
}

void register_command(PyObject* args, PyObject* kwds)
{
    static char *kwlist[] = { "sGroup", "sName", "fchanges", NULL };

    char* sGroup = nullptr;
    char* sName = nullptr;
    borrowed_ref<PyObject> fChanges;

    Arg_ParseTupleAndKeywords(args, kwds, "ss|O:register_command", kwlist, &sGroup, &sName, &fChanges);

    Abc_Frame_t* pAbc = Abc_FrameGetGlobalFrame();

    Cmd_CommandAdd( pAbc, sGroup, sName, static_cast<Cmd_CommandFuncType>(abc_command_callback), fChanges);
}


} // namespace pyabc
