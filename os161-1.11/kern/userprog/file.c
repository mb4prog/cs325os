/*
 * Implementations of system calls for user-level file I/O.  See file.h.
 */

#include <../arch/mips/include/spl.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <curthread.h>
#include <file.h>
#include <filetable.h>
#include <lib.h>
#include <thread.h>
#include <uio.h>
#include <vfs.h>


int sys_open(char *path, int  oflag, mode_t mode, int* ret)
{
	filedesc* fd;	// Handle resulting from open.


	// Try to get a handle according to path.
	fd = kmalloc(sizeof(filedesc));
	*ret = vfs_open(path, oflag, &fd->vn);
	if(*ret) return(*ret);

	// Add the vnode to the current thread's filetable.
	fd->offset = 0;
	fd->mode = mode;
	*ret = filetable_add(curthread->ft, fd);
	
	// Free the filedescriptor if we weren't able to add it.
	if(*ret == -1)
	{
		kfree(fd);
		return(*ret);
	}
	
	// No error, index of the fd should be passed back in ret.
	return(0);
}


int sys_read(int fd, void *buf, size_t nbytes, int* ret)
{
	filedesc* file_d;	// File descriptor to read from.
	struct uio u;		// Used to hold data read from the file.
	int spl;			// Value of spl to restore after disabling interrupts.


	// Make sure that buf is valid.
	if(!buf) return(EFAULT);

	// Try to get a handle to the file descriptor based on fd.
	file_d = filetable_get(curthread->ft, fd);
	if(file_d == NULL) return(EBADF);

	// Make sure that the file is opened for reading.
	switch(file_d->mode & O_ACCMODE)
	{
	case O_RDONLY:
	case O_RDWR:
		break;
	default:
		return(EBADF);
	}

	// Set up the uio for reading.
	u.uio_iovec.iov_un.un_ubase = buf;
	u.uio_iovec.iov_len = nbytes;
	u.uio_offset = file_d->offset;
	u.uio_resid = nbytes;
	u.uio_segflg = UIO_USERSPACE;
	u.uio_rw = UIO_READ;
	u.uio_space = curthread->t_vmspace;

	// Make this read atomic for the file.
	spl = splhigh();

	// Try to read the data using the uio.
	*ret = VOP_READ(file_d->vn, &u);

	// Turn interrupts back on and check the return value.
	splx(spl);
	if(*ret) return(*ret);

	// Set ret to the number of bytes read.
	*ret = nbytes - u.uio_resid;
	return(0);
}


int sys_write(int fd, void *buf, size_t nbytes, int* ret)
{
	filedesc* file_d;	// File descriptor to read from.
	struct uio u;		// Used to hold data read from the file.
	int spl;			// Value of spl to restore after disabling interrupts.


	// Make sure that buf is valid.
	if(!buf) return(EFAULT);

	// Try to get a handle to the file descriptor based on fd.
	file_d = filetable_get(curthread->ft, fd);
	if(file_d == NULL) return(EBADF);

	// Make sure that the file is opened for reading.
	switch(file_d->mode & O_ACCMODE)
	{
	case O_WRONLY:
	case O_RDWR:
		break;
	default:
		return(EBADF);
	}

	// Set up the uio for writing.
	u.uio_iovec.iov_un.un_ubase = buf;
	u.uio_iovec.iov_len = nbytes;
	u.uio_offset = file_d->offset;
	u.uio_resid = nbytes;
	u.uio_segflg = UIO_SYSSPACE;
	u.uio_rw = UIO_WRITE;
	u.uio_space = NULL;

	// Make this write atomic for the file.
	spl = splhigh();

	// Try to write the data using the uio.
	*ret = VOP_WRITE(file_d->vn, &u);

	// Turn interrupts back on and check the return value.
	splx(spl);
	if(*ret) return(*ret);

	// Set ret to the number of bytes written.
	*ret = nbytes - u.uio_resid;
	return(0);
}


int sys_close(int fd)
{
	// Remove the file descriptor from the file table.
	// This will handle closing vnodes etc. as needed.
	if(filetable_remove(curthread->ft, fd == -1)) return(EBADF);

	/*
	 * NOTE:
	 * There doesn't seem to be a way for the file system to return a hard I/O error here
	 * so there isn't a mechanism for returning EIO.
	 * Instead, the vnode will print an error if one occurs.
	 */

	return(0);
}
