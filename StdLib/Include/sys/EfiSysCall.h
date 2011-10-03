/** @file
  Function declarations for UEFI "system calls".

    The following macros are defined in this file:<BR>
@verbatim
      STDIN_FILENO      0     standard input file descriptor
      STDOUT_FILENO     1     standard output file descriptor
      STDERR_FILENO     2     standard error file descriptor
      F_OK              0     test for existence of file
      X_OK           0x01     test for execute or search permission
      W_OK           0x02     test for write permission
      R_OK           0x04     test for read permission
      SEEK_SET          0     set file offset to offset
      SEEK_CUR          1     set file offset to current plus offset
      SEEK_END          2     set file offset to EOF plus offset
      VALID_OPEN        1
      VALID_CLOSED      0
      VALID_DONT_CARE  -1
@endverbatim

    The following types are defined in this file:<BR>
@verbatim
      struct stat;    Structure declared in <sys/stat.h>
@endverbatim

    The following functions are declared in this file:<BR>
@verbatim
      ###############  System Calls used in stdio.
      int       close     (int fd);
      ssize_t   read      (int fd, void *buf, size_t n);
      ssize_t   write     (int fd, const void *buf, size_t n);
      int       unlink    (const char *name);
      int       dup2      (int, int);
      int       rmdir     (const char *);
      int       isatty    (int);

      ###############  System Calls which are also declared in sys/fcntl.h.
      int       open      (const char *name, int oflags, int mode);
      int       creat     (const char *, mode_t);
      int       fcntl     (int, int, ...);

      ###############  System Calls which are also declared in stat.h.
      int       mkdir     (const char *, mode_t);
      int       fstat     (int, struct stat *);
      int       lstat     (const char *, struct stat *);
      int       stat      (const char *, void *);
      int       chmod     (const char *, mode_t);

      ###############  System Calls which are also declared in sys/types.h.
      off_t     lseek     (int, off_t, int);
      int       truncate  (const char *, off_t);
      int       ftruncate (int, off_t);   //  IEEE Std 1003.1b-93

      ###############  EFI-specific Functions.
      int       DeleteOnClose (int fd);    Mark an open file to be deleted when closed.
      int       FindFreeFD    (int MinFd);
      BOOLEAN   ValidateFD    (int fd, int IsOpen);

      ###############  Functions added for compatibility.
      char     *getcwd    (char *, size_t);
      int       chdir     (const char *);
@endverbatim

  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#ifndef _EFI_SYS_CALL_H
#define _EFI_SYS_CALL_H

#include  <sys/EfiCdefs.h>
#include  <sys/types.h>

struct stat;  /* Structure declared in <sys/stat.h> */

#define STDIN_FILENO  0 /**< standard input file descriptor */
#define STDOUT_FILENO 1 /**< standard output file descriptor */
#define STDERR_FILENO 2 /**< standard error file descriptor */

/* access function */
#define F_OK       0  /**< test for existence of file */
#define X_OK    0x01  /**< test for execute or search permission */
#define W_OK    0x02  /**< test for write permission */
#define R_OK    0x04  /**< test for read permission */

/* whence values for lseek(2)
   Always ensure that these are consistent with <stdio.h> and <unistd.h>!
*/
#ifndef SEEK_SET
  #define SEEK_SET  0 /**< set file offset to offset */
#endif
#ifndef SEEK_CUR
  #define SEEK_CUR  1 /**< set file offset to current plus offset */
#endif
#ifndef SEEK_END
  #define SEEK_END  2 /**< set file offset to EOF plus offset */
#endif

// Parameters for the ValidateFD function.
#define VALID_OPEN         1
#define VALID_CLOSED       0
#define VALID_DONT_CARE   -1

__BEGIN_DECLS
/* EFI versions of BSD system calls used in stdio */

  /** Close a file or device.

    @param[in]  fd    File Descriptor for the file or device to close.

    @retval   0   Successful completion.
    @retval  -1   An error occurred, identified by errno.
                    - EBADF fd is not a valid File Descriptor.
                    - EINTR The function was interrupted by a signal.
                    - EIO An I/O error occurred.
  **/
  int       close     (int fd);

  /** Read from a file or device.

    @param[in]  fd    File Descriptor for the file or device to read.
    @param[in]  buf   Buffer to read data into.
    @param[in]  N     Maximum number of bytes to read.

    @return   On successful completion, read returns a non-negative integer
              indicating the number of bytes actually read.  Otherwise, it
              returns -1 and sets errno as follows:
                - EAGAIN
                - EWOULDBLOCK
                - EBADF
                - EBADMSG
                - EINTR
                - EINVAL
                - EIO
                - EISDIR
                - EOVERFLOW
                - ECONNRESET
                - ENOTCONN
                - ETIMEDOUT
                - ENOBUFS
                - ENOMEM
                - ENXIO
  **/
  ssize_t   read      (int fd, void *buf, size_t n);

  /** Write to a file or device.

    @param[in]  fd    File Descriptor for the file or device to write.
    @param[in]  buf   Buffer to write data from.
    @param[in]  N     Maximum number of bytes to write.

    @return   On successful completion, write returns a non-negative integer
              indicating the number of bytes actually written.  Otherwise, it
              returns -1 and sets errno as follows:
                - EAGAIN
                - EWOULDBLOCK
                - EBADF
                - EFBIG
                - EINTR
                - EINVAL
                - EIO
                - ENOSPC
                - EPIPE
                - ERANGE
                - ECONNRESET
                - ENOBUFS
                - ENXIO
                - ENETDOWN
                - ENETUNREACH
  **/
  ssize_t   write     (int fd, const void *buf, size_t n);

  /** Unlink (delete) a file.

    @param[in]  name    The name of the file to be deleted.

    @retval   0   Successful completion.
    @retval  -1   Unable to perform operation, errno contains further
                  information.  The file name is unchanged.
  **/
  int       unlink    (const char *name);

  /** Make file descriptor Fd2 a duplicate of file descriptor Fd1.

    @param[in]  Fd1   File descriptor to be duplicated
    @param[in]  Fd2   File descriptor to become a duplicate of Fd1.

    @retval   0   Successful completion.
    @retval  -1   Unable to perform operation, errno contains further
                  information.
  **/
  int       dup2      (int Fd1, int Fd2);

  /** Remove a directory.

    @param[in]  Path    Path to the directory to be deleted.

    @retval   0   Successful completion.
    @retval  -1   Unable to perform operation, errno contains further
                  information.  The named directory remains unchanged.
  **/
  int       rmdir     (const char *Path);

  /** Determine if fd refers to an interactive terminal device.

    @param[in]  fd    The file descriptor to be tested.

    @retval   0   The file descriptor, fd, is not for a terminal.  errno is set
                  indicating the cause for failure.
                    - EBADF   fd is not a valid open file descriptor.
                    - ENOTTY  fd does not refer to a terminal.
    @retval   1   The file descriptor, fd, is for a terminal.
  **/
  int       isatty    (int fd);

/* These system calls are also declared in sys/fcntl.h */
#ifndef __FCNTL_SYSCALLS_DECLARED
  #define __FCNTL_SYSCALLS_DECLARED

  /** Open or create a file named by name.

      The file name may be one of:
        - An absolute path beginning with '/'.
        - A relative path beginning with "." or ".." or a directory name
        - A file name
        - A mapped path which begins with a name followed by a colon, ':'.

      Mapped paths are use to refer to specific mass storage volumes or devices.
      In a Shell-hosted environment, the map command will list valid map names
      for both file system and block devices.  Mapped paths can also refer to
      devices such as the UEFI console.  Supported UEFI console mapped paths are:
        - stdin:        Standard Input        (from the System Table)
        - stdout:       Standard Output       (from the System Table)
        - stderr:       Standard Error Output (from the System Table)

    @param[in]  name
    @param[in]  oflags
    @param[in]  mode

    @return
  **/
  int     open      (const char *name, int oflags, int mode);

  /**
    @param[in]

    @return
  **/
  int     creat     (const char *, mode_t);

  /**
    @param[in]

    @return
  **/
  int     fcntl     (int, int, ...);
#endif  // __FCNTL_SYSCALLS_DECLARED

/* These system calls are also declared in stat.h */
#ifndef __STAT_SYSCALLS_DECLARED
  #define __STAT_SYSCALLS_DECLARED

  /**
    @param[in]

    @return
  **/
  int     mkdir     (const char *, mode_t);

  /**
    @param[in]

    @return
  **/
  int     fstat     (int, struct stat *);

  /**
    @param[in]

    @return
  **/
  int     lstat     (const char *, struct stat *);

  /**
    @param[in]

    @return
  **/
  int     stat      (const char *, struct stat *);

  /**
    @param[in]

    @return
  **/
  int     chmod     (const char *, mode_t);
#endif  // __STAT_SYSCALLS_DECLARED

// These are also declared in sys/types.h
#ifndef __OFF_T_SYSCALLS_DECLARED
  #define __OFF_T_SYSCALLS_DECLARED

  /**
    @param[in]

    @return
  **/
  off_t   lseek     (int, off_t, int);

  /**
    @param[in]

    @return
  **/
  int     truncate  (const char *, off_t);

  /**
    @param[in]

    @return
  **/
  int     ftruncate (int, off_t);   //  IEEE Std 1003.1b-93
#endif /* __OFF_T_SYSCALLS_DECLARED */

/* EFI-specific Functions. */

  /**
    @param[in]

    @return
  **/
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


  /**
    @param[in]

    @return
  **/
  char     *getcwd    (char *, size_t);

  /**
    @param[in]

    @return
  **/
  int       chdir     (const char *);

/* These system calls don't YET have EFI implementations. */
  int       access    (const char *path, int amode);
  int       reboot    (int, char *);
__END_DECLS

#endif  /* _EFI_SYS_CALL_H */
