#pragma once

namespace pyabc
{

void atfork_child_add(int fd);
void atfork_child_remove(int fd);

void add_sigchld_fd(int fd);
void remove_sigchld_fd(int fd);

void sys_init();

} // namespace pyabc
