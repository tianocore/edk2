/** @file
  Function declarations for UEFI "system calls".

  Concept derived from NetBSD's unistd.h file.

  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef _EFI_SYS_CALL_H
#define _EFI_SYS_CALL_H

#include  <sys/EfiCdefs.h>
#include  <sys/types.h>

struct stat;  /* Structure declared in <sys/stat.h> */

#define STDIN_FILENO  0 /* standard input file descriptor */
#define STDOUT_FILENO 1 /* standard output file descriptor */
#define STDERR_FILENO 2 /* standard error file descriptor */

/* access function */
#define F_OK       0  /* test for existence of file */
#define X_OK    0x01  /* test for execute or search permission */
#define W_OK    0x02  /* test for write permission */
#define R_OK    0x04  /* test for read permission */

/* whence values for lseek(2)
   Always ensure that these are consistent with <stdio.h> and <unistd.h>!
*/
#ifndef SEEK_SET
  #define SEEK_SET  0 /* set file offset to offset */
#endif
#ifndef SEEK_CUR
  #define SEEK_CUR  1 /* set file offset to current plus offset */
#endif
#ifndef SEEK_END
  #define SEEK_END  2 /* set file offset to EOF plus offset */
#endif

// Parameters for the ValidateFD function.
#define VALID_OPEN         1
#define VALID_CLOSED       0
#define VALID_DONT_CARE   -1

__BEGIN_DECLS

/* EFI versions of BSD system calls used in stdio */
int       close     (int fd);
ssize_t   read      (int fd, void *buf, size_t n);
ssize_t   write     (int fd, const void *buf, size_t n);
int       unlink    (const char *name);
int       dup2      (int, int);
int       rmdir     (const char *);
int       isatty    (int);

/* These system calls are also declared in sys/fcntl.h */
#ifndef __FCNTL_SYSCALLS_DECLARED
  #define __FCNTL_SYSCALLS_DECLARED
  int     open      (const char *name, int oflags, int mode);
  int     creat     (const char *, mode_t);
  int     fcntl     (int, int, ...);
#endif  // __FCNTL_SYSCALLS_DECLARED

/* These system calls are also declared in stat.h */
#ifndef __STAT_SYSCALLS_DECLARED
  #define __STAT_SYSCALLS_DECLARED
  int     mkdir     (const char *, mode_t);
  int     fstat     (int, struct stat *);
  int     lstat     (const char *, struct stat *);
  int     stat      (const char *, void *);
  int     chmod     (const char *, mode_t);
#endif  // __STAT_SYSCALLS_DECLARED

// These are also declared in sys/types.h
#ifndef __OFF_T_SYSCALLS_DECLARED
  #define __OFF_T_SYSCALLS_DECLARED
  off_t   lseek     (int, off_t, int);
  int     truncate  (const char *, off_t);
  int     ftruncate (int, off_t);   //  IEEE Std 1003.1b-93
#endif /* __OFF_T_SYSCALLS_DECLARED */

/* EFI-specific Functions. */
int       DeleteOnClose(int fd);    /* Mark an open file to be deleted when closed. */

/* Find and reserve a free File Descriptor.

  Returns the first free File Descriptor greater than or equal to the,
  already validated, fd specified by Minfd.

  @return   Returns -1 if there are no free FDs.  Otherwise returns the
            found fd.
*/
int       FindFreeFD  (int MinFd);

/*  Validate that fd refers to a valid file descriptor.
    IsOpen is interpreted as follows:
      - Positive  fd must be OPEN
      - Zero      fd must be CLOSED
      - Negative  fd may be OPEN or CLOSED

    @retval TRUE  fd is VALID
    @retval FALSE fd is INVALID
*/
BOOLEAN   ValidateFD (int fd, int IsOpen);

char     *getcwd    (char *, size_t);
int       chdir     (const char *);

/* These system calls don't YET have EFI implementations. */
int       access    (const char *path, int amode);
int       reboot    (int, char *);

__END_DECLS

#endif  /* _EFI_SYS_CALL_H */
