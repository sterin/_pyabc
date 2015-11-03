#include "util.h"

#ifdef __LINUX__

#include <sys/prctl.h>

namespace pyabc
{

void kill_on_parent_death(int sig)
{
    // kill process if parent dies
    prctl(PR_SET_PDEATHSIG, sig);

    // the parent might have died before calling prctl
    // in that case, it would be adopted by init, whose pid is 1
    if (getppid() == 1)
    {
        raise(sig);
    }
}

} // namespace pyabc

#elseif defined(__APPLE__)

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

namespace pyabc
{

void kill_on_parent_death(int sig)
{
  const int ppid = getppid();

  std::thread monitor_thread([ppid, sig](){

    struct kevent change;
    EV_SET(&change, ppid, EVFILT_PROC, EV_ADD, NOTE_EXIT|NOTE_SIGNAL, 0, nullptr);

    int kq = kqueue();
    assert( kq >= 0 );

    struct kevent event;
    struct timespec ts = {0, 0};

    // start listening, we are guaranteed to receive a notification if ppid is dead
    kevent(kq, &change, 1, &event, 1, &ts);

    // however, if ppid died before the call to kevent, ppid might not be the pid of the parent
    // in that case, the process it would be adopted by init, whose pid is 1
    if( getppid() == 1 )
    {
      raise(sig);
    }

    // now block on kevent until the the parent process dies
    retry_eintr([](){
       return kevent(kq, &change, 1, &event, 1, nullptr);
    });

    raise(sig);
  });

  monitor_thread.detach();
}

} // namespace pyabc

#else // neither linux or OS X

namespace pyabc
{

void kill_on_parent_death(int sig)
{
}

} // namespace pyabc

#endif // #ifdef __APPLE__
