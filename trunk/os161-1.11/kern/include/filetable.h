/*
 * filetable.h
 *
 *  Created by: Michael Siegrist
 * Last update: 03/07/2009
 *
 * File tables are owned by threads and are used to control access to the file handles
 * controlled by the thread/process, making such interaction opaque to the user.
 * System calls such as open() and write() are the way that the user is able to access
 * the data held by the file tables.
 */

#ifndef _FILETABLE_H_
#define _FILETABLE_H_

#include "array.h"
#include "types.h"
#include "vnode.h"


/*
 * Structures for the filetables.
 */
typedef struct
{
	struct vnode* vn;		// Node being referenced.
	unsigned int offset;		// Offset within node.
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
filetable* filetable_create();
filetable* filetable_copy(filetable* ft);
int filetable_size(filetable* ft);
filedesc* filetable_get(filetable* ft, int i);
void filetable_set(filetable* ft, int i, filedesc* val);
int filetable_add(filetable* ft, filedesc* val);
int filetable_remove(filetable* ft, int i);
void filetable_destroy(filetable* ft);


#endif /* _FILETABLE_H_ */
