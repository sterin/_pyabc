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

import re
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


def eintr_retry_call(f, *args, **kwds):
    while True:
        try:
            return f(*args, **kwds)
        except select.error as (e, msg):
            if e != errno.EINTR:
                raise
        except (OSError, IOError) as e:
            if e.errno != errno.EINTR:
                raise e


def eintr_retry_nonblocking(f, *args, **kwds):
    while True:
        try:
            return f(*args, **kwds)
        except (OSError, IOError) as e:
            if e.errno == errno.EINTR:
                continue
            if e.errno in (errno.EAGAIN, errno.EWOULDBLOCK):
                return None
            raise
        except select.error as (e, msg):
            if e != errno.EINTR:
                raise


def _set_non_blocking(fd):
    fcntl.fcntl( fd, fcntl.F_SETFL, fcntl.fcntl(fd, fcntl.F_GETFL) | os.O_NONBLOCK )


def _set_close_on_exec(fd):
    fcntl.fcntl( fd, fcntl.F_SETFD, fcntl.fcntl(fd, fcntl.F_GETFD) | fcntl.FD_CLOEXEC )


def _pipe(blocking_read=True, blocking_write=True):

    pr, pw = os.pipe()

    if not blocking_read:
        _set_non_blocking(pr)

    if not blocking_write:
        _set_non_blocking(pw)

    _set_close_on_exec(pr)
    _set_close_on_exec(pw)

    return pr, pw


class _unique_ids(object):

    def __init__(self):

        self.next_uid = 0

    def allocate(self):

        uid = self.next_uid
        self.next_uid += 1
        return uid


class base_handler(object):

    def __init__(self, loop):
        self.loop = loop

    def on_data(self, fd, data):
        pass

    def on_ready(self, fd):
        pass

    def on_hangup(self, fd):
        self.loop.unregister(fd)

    def on_error(self, fd):
        self.loop.unregister(fd)

    def kill(self):
        pass


class event_loop(object):

    def __init__(self):

        self.epoll = select.epoll()
        self.fd_to_handler = {}
        self.keep_alive_fds = set()
        self.results = []

    def register(self, h, fd, eventmask = select.EPOLLIN | select.EPOLLOUT, keep_alive=True):
        
        assert fd not in self.fd_to_handler
        self.fd_to_handler[fd] = h

        if keep_alive:
            assert fd not in self.keep_alive_fds
            self.keep_alive_fds.add(fd)

        self.epoll.register(fd, eventmask)

    def unregister(self, fd):

        assert fd in self.fd_to_handler
        del self.fd_to_handler[fd]
        self.keep_alive_fds.discard(fd)

        self.epoll.unregister(fd)

    def add_result(self, res):

        if res is None:
            traceback.print_exc(file=sys.stderr)
        else:
            self.results.append( res )

    def iter_results(self):

        if self.results:

            results = self.results
            self.results = []

            for res in results:
                yield res

    def poll(self):

        for res in self.iter_results():
            yield res

        while self.keep_alive_fds:

            for fd, event in eintr_retry_call( self.epoll.poll ):

                assert fd in self.fd_to_handler
                h = self.fd_to_handler[fd]

                if event & select.EPOLLIN:
                    data = eintr_retry_nonblocking(os.read, fd, 1 << 16)
                    if data:
                        h.on_data(fd, data)

                if event & select.EPOLLOUT:
                    h.on_ready(fd)

                if event & select.EPOLLHUP:
                    h.on_hangup(fd)

                if event & select.EPOLLERR:
                    h.on_error(fd)

            for res in self.iter_results():
                yield res

    def close(self):

        self.epoll.close()


class signal_event_handler(base_handler):

    def __init__(self, loop):

        super(signal_event_handler, self).__init__(loop)

        self.sig_fd_read, self.sig_fd_write = _pipe(blocking_read=False)

        _pyabc.atfork_child_add(self.sig_fd_read)
        _pyabc.atfork_child_add(self.sig_fd_write)

        self._install_signal_handler(self.sig_fd_write)
        self.registered = False

    def register(self, keep_alive=True):
        
        if not self.registered:
            self.loop.register(self, self.sig_fd_read, select.EPOLLIN, keep_alive)
            self.registered = True

    def unregister(self):
        
        if self.registered:
            self.loop.unregister(self.sig_fd_read)
            self.registered = False

    def stop(self):
        
        self._uninstall_signal_handler(self.sig_fd_write)

        self.unregister()
        self.loop = None

        _pyabc.atfork_child_remove(self.sig_fd_read)
        os.close(self.sig_fd_read)

        _pyabc.atfork_child_remove(self.sig_fd_write)
        os.close(self.sig_fd_write)


class timer_manager(signal_event_handler):

    def __init__(self, loop):

        super(timer_manager, self).__init__(loop)
        self.timers = []

    def _install_signal_handler(self, fd):

        def callback(*args):
            eintr_retry_call(os.write, fd, "X" )
        self.old_signal = signal.signal(signal.SIGALRM, callback)

    def _uninstall_signal_handler(self, fd):

        signal.signal(signal.SIGALRM, self.old_signal)

    def stop(self):

        self.timers = []
        super(timer_manager, self).stop()

    def add_timer(self, timeout, token):

        now = time.time()
        heapq.heappush( self.timers, (now+timeout, token) )
        self._reap_timers(now)

    def _reap_timers(self, now):

        while self.timers and self.timers[0][0] <= now:
            _, token = heapq.heappop(self.timers)
            self.loop.add_result( token )

        if self.timers:
            signal.setitimer(signal.ITIMER_REAL, self.timers[0][0] - now)
            self.register(keep_alive=False)
        else:
            self.unregister()

    def on_data(self, fd, data):
        self._reap_timers(time.time())


class process_manager(signal_event_handler):

    def __init__(self, loop):

        super(process_manager, self).__init__(loop)
        self.pid_to_handler = {}

    def _install_signal_handler(self, fd):
        _pyabc.add_sigchld_fd(fd);

    def _uninstall_signal_handler(seld):
        _pyabc.remove_sigchld_fd(fd)

    def kill_all(self):
        
        for pid, h in self.pid_to_handler.iteritems():
            h.kill()
    
    def clean(self):

        super(process_manager, self).stop()

    def fork(self, h):

        h.on_fork(self)

        ppid = os.getpid()
        rc = 1

        try:
            pid = os.fork()
            if pid == 0:
                rc = h.on_child()
                os._exit(rc)
            else:
                self.register()
                self.pid_to_handler[pid] = h
                h.on_parent(pid)
                return h
        finally:
            if os.getpid() != ppid:
                os._exit(rc)


    def _reap(self):

        for pid, h in self.pid_to_handler.items():
            rc, status = eintr_retry_call( os.waitpid, pid, os.WNOHANG )
            if rc > 0:
                del self.pid_to_handler[pid]
                if not self.pid_to_handler:
                    self.unregister()
                h.on_waitpid(status)


    def on_data(self, fd, data):
        self._reap()


class forked_process_handler(base_handler):

    def __init__(self, loop, f):

        super(forked_process_handler, self).__init__(loop)
        self.buf = cStringIO.StringIO()
        
        self.f = f
        self.token = None
        self.pid = None
        self.done_reading = False
        self.done_waiting = False

    def on_fork(self, pm):

        sys.stdout.flush()
        sys.stderr.flush()

        self.pr, self.pw = _pipe(blocking_read=False)
        _pyabc.atfork_child_add(self.pr)

    def on_parent(self, pid):
        
        self.f = None

        os.close(self.pw)
        self.pid = pid
        self.pw = None

        self.loop.register(self, self.pr)
        _pyabc.atfork_child_add(self.pr)

    def on_child(self):

        _pyabc.atfork_child_add(self.pw)
        try:
            res = self.f()
            with os.fdopen(self.pw, "w") as fout:
                pickle.dump(res, fout)
        except:
            traceback.print_exc(file=sys.stderr)
            raise
        return 0

    def on_data(self, fd, data):

        assert fd == self.pr
        self.buf.write(data)
    
    def on_hangup(self, fd):

        assert fd == self.pr
        self.loop.unregister(fd)
        _pyabc.atfork_child_remove(fd)
        os.close(fd)
        self.pr = None

        self.done_reading = True
        self.on_done()

    def on_waitpid(self, status):

        self.pid = None
        self.done_waiting = True
        self.on_done()

    def on_done(self):

        if not self.done_reading or not self.done_waiting:
            return

        try:
            result = pickle.loads(self.buf.getvalue())
        except (EOFError, pickle.UnpicklingError, ValueError):
            result = None
        except:
            import traceback
            traceback.print_exc()
            raise        

        self.loop.add_result((self.token, True, result))

    def kill(self):

        if self.pid is not None:
            os.kill(self.pid, signal.SIGQUIT)

class _splitter(object):

    def __init__(self):

        self.uids = _unique_ids()
        self.uid_to_handler = {}
        self.handler_to_uid = {}

        self.loop = event_loop()
        self.timers = timer_manager(self.loop)
        self.procs = process_manager(self.loop)

    def add_timer(self, timeout):

        uid = self.uids.allocate()
        self.timers.add_timer(timeout, (uid, False, None))
        return uid

    def fork_one(self, f, *args, **kwargs):

        def child():
            return f(*args, **kwargs)

        return self.fork_handler( forked_process_handler(self.loop, child) )

    def fork_handler(self, h):

        uid = self.uids.allocate()
        h.token = uid

        self.procs.fork(h)

        self.uid_to_handler[uid] = h
        self.handler_to_uid[h] = uid

        return uid

    def fork_all(self, funcs):

        return [ self.fork_one(f) for f in funcs ]

    def kill(self, uid):
        
        if uid in self.uid_to_handler:
            self.uid_to_handler[uid].kill()

    def cleanup(self):

        for uid, h in self.uid_to_handler.iteritems():
            h.kill()

        for _ in self.results():
            pass

    def results(self):

        for uid, done, res in self.loop.poll():

            if done and uid in self.uid_to_handler:

                h = self.uid_to_handler[uid]

                del self.uid_to_handler[uid]
                del self.handler_to_uid[h]

            yield uid, res

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


class base_redirect_handler(base_handler):

    def __init__(self, loop, f):

        super(base_redirect_handler, self).__init__(loop)

        self.f = f
        
        self.token = None
        self.pid = None
        self.status = None

        self.done_reading = False
        self.done_writing = False
        self.done_waiting = False

    def on_fork(self, pm):

        sys.stderr.flush()
        self.stdin_read, self.stdin_write = _pipe(blocking_write=False)
        _pyabc.atfork_child_add(self.stdin_write)

        sys.stdout.flush()
        self.stdout_read, self.stdout_write = _pipe(blocking_read=False)
        _pyabc.atfork_child_add(self.stdout_read)

    def on_start(self):
        pass

    def on_parent(self, pid):

        os.close(self.stdout_write)
        self.stdout_write = None

        os.close(self.stdin_read)
        self.stdin_read = None
        self.stdin_buf = collections.deque()
        self.should_close_stdin = False

        self.pid = pid
        self.path = None
        self.args = None

        # self.loop.register(self, self.stdin_write)
        _pyabc.atfork_child_add(self.stdin_write)

        self.loop.register(self, self.stdout_read)
        _pyabc.atfork_child_add(self.stdout_read)

        self.on_start()

    def on_child(self):

        os.close(sys.stdout.fileno())
        os.dup2(self.stdout_write, sys.stdout.fileno())

        os.close(sys.stdin.fileno())
        os.dup2(self.stdin_read, sys.stdin.fileno())

        return self.f()

    def on_ready(self, fd):
        
        assert fd == self.stdin_write

        while self.stdin_buf:

            data = self.stdin_buf[0]
            rc = eintr_retry_nonblocking(os.write, fd, data)

            if rc is None:
                return
            elif rc < len(data):
                self.stdin_buf[0] = data[rc:]
                return

            self.stdin_buf.popleft()

        self.loop.unregister(fd)
        
        if self.should_close_stdin:
            self.close_stdin()

    def write_stdin(self, data):

        if data and not self.stdin_buf:
            self.loop.register(self, self.stdin_write)

        self.stdin_buf.append(data)

    def close_stdin(self):

        if self.stdin_buf:
            self.should_close_stdin = True
            return

        _pyabc.atfork_child_remove(self.stdin_write)
        os.close( self.stdin_write )
        self.done_writing = True

    def on_hangup(self, fd):

        assert fd == self.stdout_read or fd == self.stdin_write
        
        self.loop.unregister(fd)
        _pyabc.atfork_child_remove(fd)
        os.close(fd)

        if fd == self.stdout_read:
            self.done_reading = True
            self.stdout_read = None

        if fd == self.stdin_write:
            self.done_writing = True
            self.stdin_write = None

        self.on_done()            

    def on_waitpid(self, status):

        self.done_waiting = True
        self.status = status
        self.pid = None
        
        self.on_done()

    def on_done(self):

        if not self.done_reading:
            return

        if not self.done_waiting:
            return

        if not self.done_writing:
            self.stdin_buf = None
            self.close_stdin()

        self.loop.add_result((self.token, True, self.status))

    def kill(self):

        if self.pid is not None:
            os.kill(self.pid, signal.SIGQUIT)


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
