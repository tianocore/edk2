/** @file
  Function declarations for UEFI "system calls".

  Concept derived from NetBSD's unistd.h file.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
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

struct stat;  // Structure declared in <sys/stat.h>

#define STDIN_FILENO  0 /* standard input file descriptor */
#define STDOUT_FILENO 1 /* standard output file descriptor */
#define STDERR_FILENO 2 /* standard error file descriptor */

/* access function */
#define F_OK       0  /* test for existence of file */
#define X_OK    0x01  /* test for execute or search permission */
#define W_OK    0x02  /* test for write permission */
#define R_OK    0x04  /* test for read permission */

/* whence values for lseek(2) */
#define SEEK_SET  0 /* set file offset to offset */
#define SEEK_CUR  1 /* set file offset to current plus offset */
#define SEEK_END  2 /* set file offset to EOF plus offset */

__BEGIN_DECLS

/* EFI versions of BSD system calls used in stdio */
extern int      close     (int fd);
extern ssize_t  read      (int fd, void *buf, size_t n);
extern ssize_t  write     (int fd, const void *buf, size_t n);
extern int      unlink    (const char *name);
extern int      dup2      (int, int);
extern int      rmdir     (const char *);
extern int      isatty    (int);

/* These system calls are also declared in sys/fcntl.h */
#ifndef __FCNTL_SYSCALLS_DECLARED
  #define __FCNTL_SYSCALLS_DECLARED
  extern int      open      (const char *name, int oflags, int mode);
  extern int      creat     (const char *, mode_t);
  extern int      fcntl     (int, int, ...);
#endif  // __FCNTL_SYSCALLS_DECLARED

/* These system calls are also declared in stat.h */
#ifndef __STAT_SYSCALLS_DECLARED
  #define __STAT_SYSCALLS_DECLARED
  extern int      mkdir     (const char *, mode_t);
  extern int      fstat     (int, struct stat *);
  extern int      lstat     (const char *, struct stat *);
  extern int      stat      (const char *, void *);
//  extern int      chmod     (const char *, mode_t);
#endif  // __STAT_SYSCALLS_DECLARED

// These are also declared in sys/types.h
#ifndef __OFF_T_SYSCALLS_DECLARED
  #define __OFF_T_SYSCALLS_DECLARED
  extern off_t    lseek     (int, off_t, int);
  extern int      truncate  (const char *, off_t);
  extern int      ftruncate (int, off_t);   //  IEEE Std 1003.1b-93
#endif /* __OFF_T_SYSCALLS_DECLARED */

/* These system calls don't YET have EFI implementations. */
extern int      access    (const char *path, int amode);
extern int      chdir     (const char *);
extern char    *getcwd    (char *, size_t);
extern int      reboot    (int, char *);

__END_DECLS

#endif  /* _EFI_SYS_CALL_H */
