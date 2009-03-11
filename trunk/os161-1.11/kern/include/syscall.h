#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include "file.h"

/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */

int sys_reboot(int code);


#endif /* _SYSCALL_H_ */
