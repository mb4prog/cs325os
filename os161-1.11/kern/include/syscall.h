#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include "file.h"

//#include <curthread.h>
/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */

#include <machine/trapframe.h>
#include "thread.h"

int sys_reboot(int code);

int sys_fork(struct trapframe *tf);
int sys_getpid();
int sys__exit(int exitcode);

#endif /* _SYSCALL_H_ */
