/*
 * filetable.h
 *
 *  Created by: Michael Siegrist
 * Last update: 03/17/2009
 *
 * File tables are owned by threads and are used to control access to the file handles
 * controlled by the thread/process, making such interaction opaque to the user.
 * System calls such as open() and write() are the way that the user is able to access
 * the data held by the file tables.
 */

#ifndef _FILETABLE_H_
#define _FILETABLE_H_

#include "array.h"
#include "synch.h"
#include "types.h"
#include "vnode.h"


/*
 * Structures for the filetables.
 */

typedef size_t mode_t;		// Modes that the file descriptors can have.

typedef struct
{
	struct vnode* vn;		// Node being referenced.
	unsigned int offset;	// Offset within node.
	mode_t mode;			// Mode for the file descriptor.
	//struct lock* lock;	// Lock for controlling access to the file.
							//      Lock code isn't written yet,
							//      so we'll just have to disable interrupts until then.
} filedesc;

typedef struct
{
	struct array* handles;	// List of all handles controlled by the filetable.
	int next;				// The next index to add handles at.
} filetable;



/*
 * Filetable functions.
 * These work similarly to functions for the array, which can be found in array.h.
 *
 * Note that filetable_create() will also initialize the first 3 elements of the table
 * to STDIN, STDOUT, and STDERR respectively.
 *
 * Also note that filetable_remove should be called when the thread this process belongs
 * to is destroyed for any reason.
 */

filetable* filetable_create();									// Create a new filetable.  Returns NULL on error.
       int filetable_init(filetable* ft);						// Attach ft to STDIN, STDOUT, and STDERR.
filetable* filetable_copy(filetable* ft);						// Returns a copy of ft, with descriptors referencing the same open handles.  NULL on error.
       int filetable_size(filetable* ft);						// Returns the number of file descriptors in ft.
 filedesc* filetable_get(filetable* ft, int i);					// Returns the file descriptor in ft at index i.  NULL on error.
      void filetable_set(filetable* ft, int i, filedesc* val);	// Sets the file descriptor in ft at index i to val.
       int filetable_add(filetable* ft, filedesc* val);			// Adds val to ft and returns the index it was added at (not always the end!).
       int filetable_remove(filetable* ft, int i);				// Removes the file descriptor at i from ft and frees it.  1 if removed from the tail, -1 on error, 0 otherwise.
      void filetable_destroy(filetable* ft);					// Removes and frees memory for all elements of ft and frees ft.


#endif /* _FILETABLE_H_ */
