/*
 * file.h
 *
 *  Created by: Michael Siegrist
 * Last update: 03/17/2009 by Bradley Brown
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
int sys_lseek(int fd, off_t pos, int whence);
int sys_dup2(int oldfd, int newfd);


#endif
