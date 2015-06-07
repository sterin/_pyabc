import os
import sys

import _pyabc

_registered_commands = {}

def _cmd_callback(args):

    try:
        assert len(args) > 0

        cmd = args[0]

        if cmd not in _registered_commands:
            raise RuntimeError('command "%s" not registered')

        assert cmd in _registered_commands

        res = _registered_commands[cmd](args)

        assert type(res) == int, "User-defined Python command must return an integer."

        return res

    except Exception, e:

        import traceback
        traceback.print_exc()

    except SystemExit, se:
        pass

    return 0

_pyabc.set_command_callback( _cmd_callback )

def add_abc_command(fcmd, group, cmd, change):
    _registered_commands[cmd] = fcmd
    _pyabc.register_command( group, cmd, change)

_python_command_globals = {}

def cmd_python(cmd_args):

    import optparse

    usage = "usage: %prog [options] <Python files>"

    parser = optparse.OptionParser(usage, prog="python")

    parser.add_option("-c", "--cmd", dest="cmd", help="Execute Python command directly")
    parser.add_option("-v", "--version", action="store_true", dest="version", help="Display Python Version")

    options, args = parser.parse_args(cmd_args)

    if options.version:
        print sys.version
        return 0

    if options.cmd:
        exec options.cmd in _python_command_globals
        return 0

    scripts_dir = os.getenv('ABC_PYTHON_SCRIPTS', ".")
    scripts_dirs = scripts_dir.split(':')

    for fname in args[1:]:
        if os.path.isabs(fname):
            execfile(fname, _python_command_globals)
        else:
            for d in scripts_dirs:
                fname = os.path.join(scripts_dir, fname)
                if os.path.exists(fname):
                    execfile(fname, _python_command_globals)
                    break
            else:
                raise RuntimeError('file "%s" not found'% fname)

    return 0

add_abc_command(cmd_python, "Python", "python", 0)
