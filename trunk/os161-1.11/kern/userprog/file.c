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
#include <dev.h>


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

int sys_dup2(int oldfd, int newfd)
{
	filedesc* file_d_old;
	filedesc* file_d_check;
	filedesc* file_d_new; //file descriptors

	//see if file_d_old does not exist, or if newfd is a negative value
	file_d_old = filetable_get(curthread->ft, oldfd);
	if (file_d_old == NULL || newfd < 0) 
		return (EBADF);

	//if the filetable is full, this should return an EMFILE error
	//however, I don't know if the table CAN be completely full
	//see if filetable_size is just current size or maximum size

	//if newfd is an open file descriptor, close the file
	file_d_check = filetable_get(curthread->ft, newfd);
	if (file_d_check != NULL) 
		sys_close(newfd);

	//copy the values from the old handle into the new
	file_d_new->vn = file_d_old->vn;
	file_d_new->offset = file_d_old->offset;
	file_d_new->mode = file_d_old->mode;

	//put the new handle and file descriptor into the file table
	filetable_set(curthread->ft, newfd, file_d_new);
	return(newfd);
}

int sys_lseek(int fd, off_t pos, int whence)
{
	filedesc* file_d;
	int tryseek_error;
	struct device *d;
	//retrieve file handle specified by fd
	file_d = filetable_get(curthread->ft, fd);
	if(file_d == NULL) return(EBADF);http://mail.google.com/mail/?shva=1#inbox

	//get information about the vnode in order to obtain the size of the file
	d = file_d->vn->vn_data;
	//switch statement based on whence
	switch (whence)
	{
	case SEEK_SET://new position is pos
		break;
	case SEEK_CUR://new position is current+pos
		pos += file_d->offset;
		break;
	case SEEK_END://new position is EOF + pos 
		pos += (d->d_blocksize + d->d_blocks);
		break;
	default: return(EINVAL);
	}

	//see if seeking to the given position is legal
	tryseek_error = VOP_TRYSEEK(file_d->vn, pos);
	if (tryseek_error != 0) return(tryseek_error);

	//the actual seek
	//in theory, set file_d->offset to pos
	//may need to do special work for pos > EOF case; make sure
	file_d->offset = pos;

	return(pos);	
}
