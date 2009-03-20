#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <machine/pcb.h>
#include <machine/spl.h>
#include <machine/trapframe.h>
#include <kern/callno.h>
#include <syscall.h>
#include <curthread.h>

/*
 * System call handler.
 *
 * A pointer to the trapframe created during exception entry (in
 * exception.S) is passed in.
 *
 * The calling conventions for syscalls are as follows: Like ordinary
 * function calls, the first 4 32-bit arguments are passed in the 4
 * argument registers a0-a3. In addition, the system call number is
 * passed in the v0 register.
 *
 * On successful return, the return value is passed back in the v0
 * register, like an ordinary function call, and the a3 register is
 * also set to 0 to indicate success.
 *
 * On an error return, the error code is passed back in the v0
 * register, and the a3 register is set to 1 to indicate failure.
 * (Userlevel code takes care of storing the error code in errno and
 * returning the value -1 from the actual userlevel syscall function.
 * See src/lib/libc/syscalls.S and related files.)
 *
 * Upon syscall return the program counter stored in the trapframe
 * must be incremented by one instruction; otherwise the exception
 * return code will restart the "syscall" instruction and the system
 * call will repeat forever.
 *
 * Since none of the OS/161 system calls have more than 4 arguments,
 * there should be no need to fetch additional arguments from the
 * user-level stack.
 *
 * Watch out: if you make system calls that have 64-bit quantities as
 * arguments, they will get passed in pairs of registers, and not
 * necessarily in the way you expect. We recommend you don't do it.
 * (In fact, we recommend you don't use 64-bit quantities at all. See
 * arch/mips/include/types.h.)
 */

void
mips_syscall(struct trapframe *tf)
{
	int callno;
	int32_t retval;
	int err;

	assert(curspl==0);

	callno = tf->tf_v0;

	/*
	 * Initialize retval to 0. Many of the system calls don't
	 * really return a value, just 0 for success and -1 on
	 * error. Since retval is the value returned on success,
	 * initialize it to 0 by default; thus it's not necessary to
	 * deal with it except for calls that return other values, 
	 * like write.
	 */

	retval = 0;

	switch (callno) {
	    case SYS_reboot:
			err = sys_reboot(tf->tf_a0);
			break;

	    /* Add stuff here */
	    case SYS_fork:
	    	retval = sys_fork(tf);
		break;

	    case SYS__exit:
	      err = sys__exit(tf->tf_a0);
	      break;
	      
	    case SYS_getpid:
	      retval = sys_getpid();
	      break;
		case SYS_open:
			// Send retval to get the index on success.
			err = sys_open((char*) tf->tf_a0, tf->tf_a1, tf->tf_a2, &retval);
			break;
		case SYS_read:
			// Send retval to get the number of bytes read.
			err = sys_read(tf->tf_a0, (void*) tf->tf_a1, tf->tf_a2, &retval);
			break;
		case SYS_write:
			// Send retval to get the number of bytes written.
			err = sys_write(tf->tf_a0, (void*) tf->tf_a1, tf->tf_a2, &retval);
			break;
		case SYS_close:
			err = sys_close(tf->tf_a0);
			break;
		case SYS_dup2:
			err = sys_dup2(tf->tf_a0, tf->tf_a1);
			break;
		case SYS_lseek:
			err = sys_lseek(tf->tf_a0, tf->tf_a1, tf->tf_a2);
			break;

	    default:
			kprintf("Unknown syscall %d\n", callno);
			err = ENOSYS;
			break;
	}


	if (err) {
		/*
		 * Return the error code. This gets converted at
		 * userlevel to a return value of -1 and the error
		 * code in errno.
		 */
		tf->tf_v0 = err;
		tf->tf_a3 = 1;      /* signal an error */
	}
	else {
		/* Success. */
		tf->tf_v0 = retval;
		tf->tf_a3 = 0;      /* signal no error */
	}
	
	/*
	 * Now, advance the program counter, to avoid restarting
	 * the syscall over and over again.
	 */
	
	tf->tf_epc += 4;

	/* Make sure the syscall code didn't forget to lower spl */
	assert(curspl==0);
}

void md_forkentry(void *data1, unsigned long data2)
{
	/*
	 * This function is provided as a reminder. You need to write
	 * both it and the code that calls it.
	 *
	 * Thus, you can trash it and do things another way if you prefer.
	 */

	struct trapframe tf = *((struct trapframe*)data1);
	kfree(data1);

	/* Set the return value to 0 for the child process
	   and advance the program counter to avoid restarting the syscall */
	tf.tf_v0 = 0;
	tf.tf_epc += 4;
	mips_usermode(&tf);
	kprintf("Returning to childproc\n");
}
int sys__exit(int exitcode) {
	kprintf("got _exit syscall from proc: %d\n", *(curthread->id));
	thread_exit();
	return 0;
}

int sys_getpid() {
	return *(curthread->id);
}

int sys_fork(struct trapframe *tf) {
	struct thread **child_thread = NULL;
	//Make a copy of the parent's trap frame on the kernel heap
	struct rapframe *tf_copy = kmalloc(sizeof(struct trapframe));

	if (tf_copy == NULL) {
		//handle the kmalloc error here
		kfree(tf_copy);
		return ENOMEM;
	}

	memcpy(tf_copy,tf,sizeof(struct trapframe));
	thread_fork("childproc", tf_copy, 0, md_forkentry, child_thread);
	//filetable_init((*child_thread)->ft);
	return *((*child_thread)->id);
}
