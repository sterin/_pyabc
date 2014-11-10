#include "sys.h"
#include "util.h"

#include <set>

#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/wait.h>

namespace pyabc
{

namespace
{

std::set<int> sigchld_wakeup_fds;

void sigchld_handler(int)
{
    save_restore_errno errno_scope;

    for( int fd : sigchld_wakeup_fds )
    {
        retry_eintr(write, fd, "C", 1);
    }
}

std::set<std::string> temporary_files;

void sigquit_handler(int sig)
{
    for (auto& fn : temporary_files)
    {
        unlink(fn.c_str());
    }

    abort();
}

void add_sigchld_fd(int fd)
{
    block_signals_scope scope{SIGCHLD};

    if ( sigchld_wakeup_fds.empty() )
    {
        install_signal_handler({SIGCHLD}, sigchld_handler);
    }

    sigchld_wakeup_fds.insert(fd);
}

void remove_sigchld_fd(int fd)
{
    block_signals_scope scope{SIGCHLD};

    sigchld_wakeup_fds.erase(fd);

    if ( sigchld_wakeup_fds.empty() )
    {
        uninstall_signal_handler({SIGCHLD});
    }
}

void atfork_prepare_handler()
{
    block_signals({SIGCHLD, SIGINT, SIGQUIT});
}

void atfork_parent_handler()
{
    unblock_signals({SIGCHLD, SIGINT, SIGQUIT});
}

std::set<int> child_fds;

void atfork_child_handler()
{
    // close all fds registered to be closed in the child process

    for (int fd : child_fds)
    {
        close(fd);
    }

    child_fds.clear();

    // uninstall sigchild handler

    if ( !sigchld_wakeup_fds.empty() )
    {
        uninstall_signal_handler({SIGCHLD});
    }

    sigchld_wakeup_fds.clear();
    temporary_files.clear();

    unblock_signals({SIGCHLD, SIGINT, SIGQUIT});

    kill_on_parent_death();
}

} // namespace

void atfork_child_add(PyObject* pyfd)
{
    int fd = Int_AsLong(pyfd);
    child_fds.insert(fd);
}

void atfork_child_remove(PyObject* pyfd)
{
    int fd = Int_AsLong(pyfd);
    child_fds.erase(fd);
}

void add_sigchld_fd(PyObject* pyfd)
{
    int fd = Int_AsLong(pyfd);
    add_sigchld_fd(fd);
}

void remove_sigchld_fd(PyObject* pyfd)
{
    int fd = Int_AsLong(pyfd);
    remove_sigchld_fd(fd);
}

void sys_init()
{
    pthread_atfork(
        atfork_prepare_handler,
        atfork_parent_handler,
        atfork_child_handler
    );

    install_signal_handler({SIGINT, SIGQUIT}, sigquit_handler);
}

} // namespace pyabc

extern "C"
int Util_SignalSystem(const char* cmd)
{
    int pid = fork();

    if (pid == 0)
    {
        const char* argv[] = {
            "sh",
            "-c",
            cmd,
            nullptr
        };

        execv("/bin/sh", const_cast<char**>(argv));

        abort();
    }
    else if (pid < 0)
    {
        return -1;
    }

    int status = 0;

    pyabc::retry_eintr(waitpid, pid, &status, 0);

    return status;
}

extern "C"
void Util_SignalTmpFileRemove(const char* fname, int fLeave)
{
    pyabc::block_signals_scope scope{SIGINT, SIGQUIT};

    if (!fLeave)
    {
        unlink(fname);
    }

    pyabc::temporary_files.erase(fname);
}

extern "C"
int tmpFile(const char* prefix, const char* suffix, char** out_name);

extern "C"
int Util_SignalTmpFile(const char* prefix, const char* suffix, char** out_name)
{
    pyabc::block_signals_scope scope{SIGINT, SIGQUIT};

    int fd = tmpFile(prefix, suffix, out_name);

    if (fd >= 0)
    {
        pyabc::temporary_files.insert(*out_name);
    }

    return fd;
}
