#ifndef pyabc_util__H
#define pyabc_util__H

#include <initializer_list>

#include <signal.h>
#include <errno.h>
#include <sys/prctl.h>


namespace pyabc
{

template<typename C, typename... T>
inline int retry_eintr(C c, T... t)
{
    int rc;

    do
    {
        rc = c(t...);
    }
    while ( rc==-1 && errno==EINTR);

    return rc;
}

inline void block_signals(std::initializer_list<int> signals, sigset_t* old=nullptr)
{
    sigset_t mask;
    sigemptyset(&mask);

    for( int sig : signals )
    {
        sigaddset(&mask, sig);
    }

    sigprocmask(SIG_BLOCK, &mask, old);
}

inline void unblock_signals(std::initializer_list<int> signals)
{
    sigset_t mask;
    sigemptyset(&mask);

    for( int sig : signals )
    {
        sigaddset(&mask, sig);
    }

    sigprocmask(SIG_UNBLOCK, &mask, nullptr);
}

class block_signals_scope
{
public:

    explicit block_signals_scope(std::initializer_list<int> signals)
    {
        block_signals(signals, &_old);
    }

    ~block_signals_scope()
    {
        sigprocmask(SIG_SETMASK, &_old, nullptr);
    }

private:

    sigset_t _old;
};

class save_restore_errno
{
public:

    save_restore_errno() :
        _errno(errno)
    {

    }

    ~save_restore_errno()
    {
        errno = _errno;
    }

private:

    int _errno;
};

inline void kill_on_parent_death(int sig)
{
    // kill process if parent dies
    prctl(PR_SET_PDEATHSIG, sig);

    // the parent may have died before calling prctl (there is a race condition)
    // in that case, it would be adopted by init, whose pid is 1
    if (getppid() == 1)
    {
        raise(sig);
    }
}

inline void install_signal_handler(std::initializer_list<int> signals, void (*handler)(int))
{
    struct sigaction sa;

    sa.sa_handler = handler;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);

    for( int sig : signals )
    {
        sigaction(sig,  &sa, nullptr);
    }
}

inline void uninstall_signal_handler(std::initializer_list<int> signals)
{
    for( int sig : signals )
    {
        signal(sig, SIG_DFL);
    }
}

} // namespace pyabc

#endif // ifndef pyabc_util__H
