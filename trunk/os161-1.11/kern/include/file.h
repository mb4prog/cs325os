/*
 * file.h
 *
 *  Created by: Michael Siegrist
 * Last update: 03/10/2009
 *
 * Contains the system calls necessary to perform user-level file I/O.
 * Performance reflects the descriptions in the man pages unless stated otherwise.
 */


#ifndef _FILE_H_
#define _FILE_H_


#include <filetable.h>


int sys_open(char *path, int  oflag, mode_t mode, int* ret);
int sys_read(int fd, void *buf, size_t nbytes, int* ret);
int sys_write(int fd, void *buf, size_t nbytes, int* ret);
int sys_close(int fd);


#endif
