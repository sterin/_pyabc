#include "sys.h"

#include <set>
#include <initializer_list>

#include <pthread.h>
#include <errno.h>
#include <sys/prctl.h>

class block_signals
{
public:

    explicit block_signals(std::initializer_list<int> signals)
    {
        sigset_t mask;
        sigemptyset(&mask);

        for( int sig : signals )
        {
            sigaddset(&mask, sig);
        }

        sigprocmask(SIG_BLOCK, &mask, &_old);
    }

    ~block_signals()
    {
        sigprocmask(SIG_SETMASK, &_old, nullptr);
    }

private:

    sigset_t _old;
};


namespace
{

bool sigchild_handler_installed = false;

int sigchld_wakeup_fd = -1;

void sigchld_handler(int)
{
    int old_errno = errno;

    if (sigchld_wakeup_fd > -1)
    {
        while( write(sigchld_wakeup_fd, "", 1) == -1 && errno==EINTR )
            ;
    }

    errno = old_errno;
}

void install_sigchld_handler(int fd)
{
    sigchld_wakeup_fd = fd;
    sigchild_handler_installed = true;

    static struct sigaction sa;

    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    sigaction(SIGCHLD, &sa, nullptr);
}

void uninstall_sigchld_handler()
{
    if (sigchild_handler_installed)
    {
        signal(SIGCHLD, SIG_DFL);

        sigchld_wakeup_fd = -1;
        sigchild_handler_installed = false;
    }
}

void block_sigchild()
{
    sigset_t mask;

    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);

    sigprocmask(SIG_BLOCK, &mask, nullptr);
}

void unblock_sigchild()
{
    sigset_t mask;

    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);

    sigprocmask(SIG_UNBLOCK, &mask, nullptr);
}

void atfork_prepare_handler()
{
    if( sigchild_handler_installed )
    {
        block_sigchild();
    }
}

void atfork_parent_handler()
{
    if( sigchild_handler_installed )
    {
        unblock_sigchild();
    }
}

std::set<int> child_fds;

void atfork_child_handler()
{
    // close all fds registered to be closed in the child process
    for( int fd : child_fds )
    {
        close(fd);
    }

    // close fds onlyu once
    child_fds.clear();

    // uninstall sigchild handler
    if( sigchild_handler_installed )
    {
        uninstall_sigchld_handler();
        unblock_sigchild();
    }

    // kill process if parent dies
    prctl(PR_SET_PDEATHSIG, SIGINT);

    // the paren may have died before calling prctl (there is a race condition)
    // in that case, it would be adopted by init, whose pid is 1
    if ( getppid() == 1)
    {
        raise(SIGINT);
    }
}

bool atfork_child_handler_installed = false;

void install_atfork_child_handler()
{
    if( !atfork_child_handler_installed)
    {
        pthread_atfork(atfork_prepare_handler, atfork_parent_handler, atfork_child_handler);
        atfork_child_handler_installed = true;
    }
}

} // namespace

namespace pyabc
{

void atfork_child_add(PyObject *pyfd)
{
    install_atfork_child_handler();

    int fd = Int_AsLong(pyfd);

    child_fds.insert(fd);
}

void atfork_child_remove(PyObject *pyfd)
{
    int fd = Int_AsLong(pyfd);

    child_fds.erase(fd);
}

void install_sigchld_handler(PyObject *pyfd)
{
    int fd = Int_AsLong(pyfd);

    ::install_sigchld_handler(fd);
}

void uninstall_sigchld_handler()
{
    ::uninstall_sigchld_handler();
}

} // namespace pyabc



