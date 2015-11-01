"""
module pyabc.split

Executes python functions and their arguements as separate processes and returns their return values through pickling. This modules offers a single function:

Function: split_all(funcs)

The function returns a generator objects that allowes iteration over the results,

Arguments:

funcs: a list of tuples (f, args) where f is a python function and args is a collection of arguments for f.

Caveats:

1. Global variables in the parent process are not affected by the child processes.
2. The functions can only return simple types, see the pickle module for details

Usage:

Assume you would like to run the function f_1(1), f_2(1,2), f_3(1,2,3) in different processes.

def f_1(i):
    return i+1

def f_2(i,j):
    return i*10+j+1

def f_3(i,j,k):
    return i*100+j*10+k+1

Construct a tuple of the function and arguments for each function

t_1 = (f_1, [1])
t_2 = (f_2, [1,2])
t_3 = (f_3, [1,2,3])

Create a list containing these tuples:

funcs = [t_1, t_2, t_3]

Use the function split_all() to run these functions in separate processes:

for res in split_all(funcs):
    print res

The output will be:

2
13
124

(The order may be different, except that in this case the processes are so fast that they terminate before the next one is created)

Alternatively, you may quite in the middle, say after the first process returns:

for res in split_all(funcs):
    print res
    break

This will kill all processes not yet finished.

To run ABC operations, that required saving the child process state and restoring it at the parent, use abc_split_all().

    import pyabc

    def abc_f(truth):
        import os
        print "pid=%d, abc_f(%s)"%(os.getpid(), truth)
        pyabc.run_command('read_truth %s'%truth)
        pyabc.run_command('strash')

    funcs = [
        defer(abc_f)("1000"),
        defer(abc_f)("0001")
    ]

    for _ in abc_split_all(funcs):
        pyabc.run_command('write_verilog /dev/stdout')

Author: Baruch Sterin <sterin@berkeley.edu>
"""

import os
import fcntl
import errno
import signal
import select
import time
import heapq
import collections

from contextlib import contextmanager

import cStringIO
import pickle

import traceback

import sys
import _pyabc

def _eintr_retry_call(f, *args, **kwds):
    while True:
        try:
            return f(*args, **kwds)
        except (OSError, IOError) as e:
            if e.errno == errno.EINTR:
                continue
            raise

def _eintr_retry_nonblocking(f, *args, **kwds):
    while True:
        try:
            return f(*args, **kwds)
        except (OSError, IOError) as e:
            if e.errno == errno.EINTR:
                continue
            if e.errno in (errno.EAGAIN, errno.EWOULDBLOCK):
                return None
            raise

def _set_non_blocking(fd):
    fcntl.fcntl( fd, fcntl.F_SETFL, fcntl.fcntl(fd, fcntl.F_GETFL) | os.O_NONBLOCK )

def _set_close_on_exec(fd):
    fcntl.fcntl( fd, fcntl.F_SETFD, fcntl.fcntl(fd, fcntl.F_GETFD) | fcntl.FD_CLOEXEC )

def _pipe():

    pr, pw = os.pipe()

    _set_non_blocking(pr)

    _set_close_on_exec(pr)
    _set_close_on_exec(pw)

    return pr, pw

class _unique_ids(object):

    def __init__(self):

        self.next_uid = 0
        self.live = set()

    def allocate(self, live=True):

        uid = self.next_uid
        self.next_uid += 1

        if live:
            self.live.add(uid)

        return uid

    def remove(self, uid):

        self.live.remove(uid)

    def __len__(self):

        return len(self.live)

class _epoll(object):

    def __init__(self):

        self.epoll = select.epoll()
        self.registered_fds = set()

    def register(self, fd, *args):

        self.registered_fds.add(fd)
        return self.epoll.register(fd, *args)

    def unregister(self, fd):

        if fd in self.registered_fds:

            self.epoll.unregister(fd)
            self.registered_fds.remove(fd)

    def poll(self):

        events = _eintr_retry_call( self.epoll.poll )

        filtered_events = []

        for fd, event in events:

            if event & select.EPOLLHUP:

                if fd in self.registered_fds:

                    self.epoll.unregister(fd)
                    self.registered_fds.remove(fd)

            if event != select.EPOLLHUP:

                filtered_events.append( (fd, event) )

        return filtered_events

    def close(self):

        self.epoll.close()


class _splitter(object):

    def __init__(self):

        self.uids = _unique_ids()

        self.pid_to_uid = {}
        self.uid_to_pid = {}

        self.uid_to_fd = {}
        self.fd_to_uid = {}

        self.uid_to_buf = {}
        self.fd_to_buf = {}

        p = _pipe()

        self.sigchld_fd = p[0]
        self.sigchld_fd_write = p[1]

        _pyabc.atfork_child_add(self.sigchld_fd)
        _pyabc.atfork_child_add(self.sigchld_fd_write)

        _pyabc.add_sigchld_fd(self.sigchld_fd_write);

        self.epoll = _epoll()
        self.epoll.register(self.sigchld_fd, select.EPOLLIN)

        self.timers = []
        self.expired_timers = collections.deque()

        signal.signal(signal.SIGALRM, self._timer_callback)

    def add_timer(self, timeout):

        uid = self.uids.allocate(live=False)
        now = time.time()

        heapq.heappush( self.timers, (now+timeout, uid) )

        self._reap_timers(now)

        return uid

    def _reap_timers(self, now):

        while self.timers and self.timers[0][0] <= now:

            t, uid = heapq.heappop(self.timers)
            self.expired_timers.append(uid)

        if self.timers:

            signal.setitimer(signal.ITIMER_REAL, self.timers[0][0] - now)

    def _timer_callback(self, *args):
        _eintr_retry_call(os.write, self.sigchld_fd_write, "T" )

    def cleanup(self):

        # kill all remaining child process
        for pid, uid in self.pid_to_uid.iteritems():
            os.kill(pid, signal.SIGKILL)

        # reap kill child processes
        for _ in self.results():
            pass

        self.epoll.close()

        _pyabc.atfork_child_remove(self.sigchld_fd)
        os.close(self.sigchld_fd)

        _pyabc.atfork_child_remove(self.sigchld_fd_write)
        _pyabc.remove_sigchld_fd(self.sigchld_fd_write)
        os.close(self.sigchld_fd_write)


    def _start_monitoring(self, pid, fd):

        self.epoll.register(fd, select.EPOLLIN)

        uid = self.uids.allocate()

        self.pid_to_uid[pid] = uid
        self.uid_to_pid[uid] = pid

        self.uid_to_fd[uid] = fd
        self.fd_to_uid[fd] = uid

        buf = cStringIO.StringIO()

        self.uid_to_buf[uid] = buf
        self.fd_to_buf[fd] = buf

        return uid

    def _read_by_fd(self, fd):

        data = _eintr_retry_nonblocking(os.read, fd, 16384)

        if data:

            buf = self.fd_to_buf[fd]
            buf.write(data)

    def kill(self, uid):

        pid = self.uid_to_pid[uid]
        os.kill(pid, signal.SIGKILL)

    def _reap(self):

        for pid, uid in self.pid_to_uid.items():

            rc, status = _eintr_retry_call( os.waitpid, pid, os.WNOHANG )

            if rc > 0:

                yield self._reap_one(pid, status)

    def _reap_one(self, pid, status):

        uid = self.pid_to_uid[pid]
        fd = self.uid_to_fd[uid]
        buf = self.uid_to_buf[uid]

        # collect the output of the process
        self._read_by_fd(fd)

        result = None

        if os.WIFEXITED(status):

            # unpickle the return value
            try:
                result = pickle.loads(buf.getvalue())
            except EOFError, pickle.UnpicklingError:
                result = None

        # stop tracking the process
        self.epoll.unregister(fd)

        # close pipe
        os.close(fd)

        # pipe is closed, no need to close it on child processes
        _pyabc.atfork_child_remove(fd)

        self.uids.remove(uid)

        del self.pid_to_uid[pid]
        del self.uid_to_pid[uid]

        del self.uid_to_fd[uid]
        del self.fd_to_uid[fd]

        del self.uid_to_buf[uid]
        del self.fd_to_buf[fd]

        return uid, result

    def _child( self, fd, f, args, kwargs):

        # call function
        try:

            res = f(*args, **kwargs)

            with os.fdopen(fd, "w") as fout:
                pickle.dump(res, fout)

        except:
            traceback.print_exc(file=sys.stderr)
            raise

        return 0

    def fork_one(self, f, *args, **kwargs):

        pr, pw = _pipe()
        _pyabc.atfork_child_add(pr)

        ppid = os.getpid()
        rc = 1

        try:

            pid = os.fork()

            if pid == 0:

                _pyabc.atfork_child_add(pw)
                rc = self._child(pw, f, args, kwargs)
                os._exit(rc)

            else:

                os.close(pw)

                uid = self._start_monitoring(pid, pr)
                return uid

        finally:

            if os.getpid() != ppid:
                os._exit(rc)

    def fork_all(self, funcs):
        return [ self.fork_one(f) for f in funcs ]

    def results(self):

        while self.uids:

            events = _eintr_retry_call( self.epoll.poll )

            for fd, event in events:

                if fd != self.sigchld_fd and event & select.EPOLLIN:

                    self._read_by_fd(fd)

            for fd, event in events:

                if fd == self.sigchld_fd and event & select.EPOLLIN:

                        ch = _eintr_retry_nonblocking(os.read, self.sigchld_fd, 1)

                        if ch == 'T':

                            self._reap_timers(time.time())

                            for uid in self.expired_timers:
                                yield uid, None

                            self.expired_timers.clear()

                        for rc in self._reap():
                            yield rc

    def __iter__(self):
        def iterator():
            for res in self.results():
                yield res
        return iterator()


@contextmanager
def make_splitter(*args, **kwargs):

    s = _splitter(*args, **kwargs)

    try:
        yield s
    finally:
        s.cleanup()


def split_all_full(funcs, timeout=None):
    # provide an iterator for child process result
    with make_splitter() as s:

        timer_uid = None

        if timeout:
            timer_uid = s.add_timer(timeout)

        s.fork_all(funcs)

        for uid, res in s:

            if uid == timer_uid:
                break

            yield uid, res

def defer(f):
    return lambda *args, **kwargs: lambda : f(*args,**kwargs)

def split_all(funcs, *args, **kwargs):
    for _, res in split_all_full( ( defer(f)(*fargs) for f,fargs in funcs ), *args, **kwargs ):
        yield res

import tempfile

@contextmanager
def temp_file_names(suffixes):
    names = []
    try:
        for suffix in suffixes:
            with tempfile.NamedTemporaryFile(delete=False, suffix=suffix) as file:
                names.append( file.name )
        yield names
    finally:
        for name in names:
            os.unlink(name)

class abc_state(object):
    def __init__(self):
        with tempfile.NamedTemporaryFile(delete=False, suffix='.aig') as file:
            self.aig = file.name
        with tempfile.NamedTemporaryFile(delete=False, suffix='.log') as file:
            self.log = file.name
        pyabc.run_command(r'write_status %s'%self.log)
        pyabc.run_command(r'write_aiger %s'%self.aig)

    def __del__(self):
        os.unlink( self.aig )
        os.unlink( self.log )

    def restore(self):
        pyabc.run_command(r'read_aiger %s'%self.aig)
        pyabc.run_command(r'read_status %s'%self.log)

def abc_split_all(funcs):
    import pyabc

    def child(f, aig, log):
        res = f()
        pyabc.run_command(r'write_status %s'%log)
        pyabc.run_command(r'write_aiger %s'%aig)
        return res

    def parent(res, aig, log):
        pyabc.run_command(r'read_aiger %s'%aig)
        pyabc.run_command(r'read_status %s'%log)
        return res

    with temp_file_names( ['.aig','.log']*len(funcs) ) as tmp:

        funcs = [ defer(child)(f, tmp[2*i],tmp[2*i+1]) for i,f in enumerate(funcs) ]

        for i, res in split_all_full(funcs):
            yield i, parent(res, tmp[2*i],tmp[2*i+1])

if __name__ == "__main__":

    # define some functions to run

    def f_1(i):
        import time
        time.sleep(10)
        return i+1

    def f_2(i,j):
        return i*10+j+1

    def f_3(i,j,k):
        import time
        time.sleep(2)
        return i*100+j*10+k+1

    # Construct a tuple of the function and arguments for each function

    t_1 = (f_1, [1])
    t_2 = (f_2, [1,2])
    t_3 = (f_3, [1,2,3])

    # Create a list containing these tuples:

    funcs = [t_1, t_2, t_3]

    # Use the function split_all() to run these functions in separate processes:

    for res in split_all(funcs, timeout=4):
        print res

    # Alternatively, quit after the first process returns:

    for res in split_all(funcs):
        print res
        break

    # For operations with ABC that save and restore status

    import pyabc

    def abc_f(truth):
        import os
        print "pid=%d, abc_f(%s)"%(os.getpid(), truth)
        pyabc.run_command('read_truth %s'%truth)
        pyabc.run_command('strash')
        return 100

    funcs = [
        defer(abc_f)("1000"),
        defer(abc_f)("0001")
    ]

    best = None

    for i, res in abc_split_all(funcs):
        print i, res
        if best is None:\
            # save state
            best = abc_state()
        pyabc.run_command('write_verilog /dev/stdout')

    # if there is a saved state, restore it
    if best is not None:
        best.restore()
        pyabc.run_command('write_verilog /dev/stdout')
