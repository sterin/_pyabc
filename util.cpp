#include "util.h"

#include <fcntl.h>

#include <base/main/main.h>

using namespace py;

namespace
{

ref<PyObject> internal_system_callback;
ref<PyObject> internal_tmpfile_callback;
ref<PyObject> internal_tmpfile_remove_callback;

} // unnamed namespace

int Util_SignalSystem(const char* cmd)
{
    return system(cmd);
}

int tmpFile(const char* prefix, const char* suffix, char** out_name);

int Util_SignalTmpFile(const char* prefix, const char* suffix, char** out_name)
{
    return tmpFile(prefix, suffix, out_name);
}

void Util_SignalTmpFileRemove(const char* fname, int fLeave)
{
    if (! fLeave)
    {
        unlink(fname);
    }
}

extern "C"
int Util_SignalSystem(const char* cmd)
{
    if (!internal_system_callback)
    {
        return -1;
    }

    try
    {
        gil_state_ensure scope;

        ref <PyObject> res = Object_CallFunction(internal_system_callback, "s", cmd);

        return Int_AsLong(res);
    }
    catch (...)
    {
        return -1;
    }
}

extern "C"
int Util_SignalTmpFile(const char* prefix, const char* suffix, char** out_name)
{
    if (!internal_tmpfile_callback)
    {
        return -1;
    }

    try
    {
        gil_state_ensure scope;

        ref <PyObject> res = Object_CallFunction(internal_tmpfile_callback, "ss", prefix, suffix);

        *out_name = ABC_ALLOC(char, String_Size(res)+1);
        strcpy(*out_name, String_AsString(res));

        return open(*out_name, O_WRONLY);
    }
    catch (...)
    {
        return -1;
    }
}

extern "C"
void Util_SignalTmpFileRemove(const char* fname, int fLeave)
{
    if (!internal_tmpfile_remove_callback)
    {
        return;
    }

    try
    {
        gil_state_ensure scope;
        Object_CallFunction(internal_tmpfile_remove_callback, "si", fname, fLeave);
    }
    catch (...)
    {
    }
}

namespace pyabc
{

void set_util_callbacks(PyObject* args, PyObject* kwds)
{
    static char *kwlist[] = { "system_callback", "tmpfile_callback", "tmpfile_remove_callback", NULL };

    borrowed_ref<PyObject> system_callback;
    borrowed_ref<PyObject> tmpfile_callback;
    borrowed_ref<PyObject> tmpfile_remove_callback;

    Arg_ParseTupleAndKeywords(args, kwds, "OOO:set_util_callbacks", kwlist, &system_callback, &tmpfile_callback, &tmpfile_remove_callback);

    internal_system_callback = system_callback ;
    internal_tmpfile_callback = tmpfile_callback;
    internal_tmpfile_remove_callback = tmpfile_remove_callback;
}

} // namespace
