/*
 * File table for processes.  See filetable.h.
 */

#include <kern/unistd.h>
#include <types.h>
#include <lib.h>
#include <array.h>
#include <vfs.h>
#include <vnode.h>
#include <filetable.h>



filetable* filetable_create()
{
	filetable* ft;		// File table created.
	struct vnode* v;	// Used to attach to console and keyboard.
	filedesc** fd;		// Used to initialize STDIN etc.
	char name[16];				// Name for attaching handles.
	int result;					// Result of opening console handles.

	// Create a new filetable.
	ft = kmalloc(sizeof(filetable));
	if(ft == NULL)
	{
		return(NULL);
	}

	ft->next = 0;
	ft->handles = array_create();
	if(ft->handles == NULL)
	{
		kfree(ft);
		return(NULL);
	}

	// Initialize the handles to STDIN, STDOUT, and STDERR.
	fd = kmalloc(sizeof(struct filedesc*) * 3);
	if(fd == NULL)
	{
		filetable_destroy(ft);
		return(NULL);
	}

	// STDIN
	strcpy(name, "con:");
	result = vfs_open(name, O_WRONLY, &v);
	if(result)
	{
		kprintf("Unable to attach to stdin: %s\n", strerror(result));
		vfs_close(v);
		filetable_destroy(ft);
		return(NULL);
	}
	fd[0]->vn = v;
	fd[0]->offset = 0;
	filetable_add(ft, (void*) fd[0]);

	// STDOUT
	strcpy(name, "con:");
	result = vfs_open(name, O_RDONLY, &v);
	if(result)
	{
		kprintf("Unable to attach to stdout: %s\n", strerror(result));
		vfs_close(v);
		filetable_destroy(ft);
		return(NULL);
	}
	fd[1]->vn = v;
	fd[1]->offset = 0;
	filetable_add(ft, (void*) fd[1]);

	// STDERR
	strcpy(name, "con:");
	result = vfs_open(name, O_WRONLY, &v);
	if(result)
	{
		kprintf("Unable to attach to stderr: %s\n", strerror(result));
		vfs_close(v);
		filetable_destroy(ft);
		return(NULL);
	}
	fd[2]->vn = v;
	fd[2]->offset = 0;
	filetable_add(ft, (void*) fd[2]);

	return(ft);
}

filetable* filetable_copy(filetable* ft)
{
	filetable* copy;		// Copy of ft returned.
	int size;					// Number of handles in ft.
	int i;						// Index into handles of ft.

	// Create a new filetable to return.
	assert(ft);
	copy = filetable_create();
	if(copy == NULL)
	{
		return(NULL);
	}

	// Copy all of the handles in ft.
	// Note that the handles will point to the same objects,
	// meaning that changes made in ft will be reflected in copy
	// and vice versa.
	copy->next = ft->next;
	size = filetable_size(ft);
	for(i = 0; i < size; i++)
	{
		filetable_add(copy, filetable_get(ft, i));
	}

	return(copy);
}

int filetable_size(filetable* ft)
{
	assert(ft);
	return(array_getnum(ft->handles));
}

filedesc* filetable_get(filetable* ft, int i)
{
	assert(ft);
	return((filedesc*) array_getguy(ft->handles, i));
}

void filetable_set(filetable* ft, int i, filedesc* val)
{
	assert(ft);
	assert(val);
	assert(i >=0 && i < filetable_size(ft));

	// NULL values are candidates for next to replace.
	if(val == NULL && ft->next > i)
	{
		ft->next = i;
	}
	return(array_setguy(ft->handles, i, (void*) val));
}

int filetable_add(filetable* ft, filedesc* val)
{
	int i;
	int ret;
	int size;

	assert(ft);
	assert(val);

	// Go through the list to add at next.
	if(ft->next < filetable_size(ft))
	{
		filetable_set(ft, ft->next, val);

		// Determine the next value for next.
		size = filetable_size(ft);
		for(i = ft->next; i < size && filetable_get(ft, i); i++);
		ft->next = i;
		ret = 0;
	}
	else
	{
		// Add to the end of the list.
		ret = array_add(ft->handles, (void*) val);
		ft->next = filetable_size(ft);
	}

	return(ret);
}

int filetable_remove(filetable* ft, int i)
{
	int ret;
	filedesc* fd;

	assert(ft);
	assert(i >= 0 && i < filetable_size(ft));

	// Close the item at i.
	fd = filetable_get(ft, i);
	if(fd)
	{
		VOP_CLOSE(fd->vn);
		kfree(fd);
		filetable_set(ft, i, NULL);
	}
	
	// Only remove the item from the array if it is at the tail.
	if(i == filetable_size(ft) - 1)
	{
		array_remove(ft->handles, i);
		ret = 1;
	}
	else
	{
		ret = 0;
	}
	
	return(ret);
}

void filetable_destroy(filetable* ft)
{
	int i;
	int size;

	assert(ft);

	// Delete all filedescs attached to ft.
	size = filetable_size(ft);
	for(i = 0; i < size; i++)
	{
		filetable_remove(ft, i);
	}

	if(ft->handles)
	{
		array_destroy(ft->handles);
	}
	kfree(ft);
}

