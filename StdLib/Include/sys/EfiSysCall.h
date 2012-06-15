/** @file
  Function declarations for UEFI "system calls".

    The following macros are defined in this file:<BR>
@verbatim
      STDIN_FILENO      0     standard input file descriptor
      STDOUT_FILENO     1     standard output file descriptor
      STDERR_FILENO     2     standard error file descriptor
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
@endverbatim

  Copyright (c) 2010 - 2012, Intel Corporation. All rights reserved.<BR>
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

    @param[in]  name      Name of file to open.
    @param[in]  oflags    Flags as defined in fcntl.h.
    @param[in]  mode      Access mode to use if creating the file.

    @return   Returns -1 on failure, otherwise the file descriptor for the open file.
  **/
  int     open      (const char *name, int oflags, int mode);

  /** Create a new file or rewrite an existing one.

      The creat() function behaves as if it is implemented as follows:

        int creat(const char *path, mode_t mode)
        {
            return open(path, O_WRONLY|O_CREAT|O_TRUNC, mode);
        }

    @param[in]    Path    The name of the file to create.
    @param[in]    Mode    Access mode (permissions) for the new file.

    @return   Returns -1 on failure, otherwise the file descriptor for the open file.
  **/
  int     creat     (const char *Path, mode_t Mode);

  /** File control

      This function performs the operations described below and defined in <fcntl.h>.

        - F_DUPFD: Return the lowest numbered file descriptor available that is >= the third argument.
                   The new file descriptor refers to the same open file as Fd.

        - F_SETFD: Set the file descriptor flags to the value specified by the third argument.
        - F_GETFD: Get the file descriptor flags associated with Fd.
        - F_SETFL: Set the file status flags based upon the value of the third argument.
        - F_GETFL: Get the file status flags and access modes for file Fd.

    @param[in]  Fd      File descriptor associated with the file to be controlled.
    @param[in]  Cmd     Command to execute.
    @param[in]  ...     Additional arguments, as needed by Cmd.

    @return   A -1 is returned to indicate failure, otherwise the value
              returned is positive and depends upon Cmd as follows:
                - F_DUPFD: A new file descriptor.
                - F_SETFD: files previous file descriptor flags.
                - F_GETFD: The files file descriptor flags.
                - F_SETFL: The old status flags and access mode of the file.
                - F_GETFL: The status flags and access mode of the file.
  **/
  int     fcntl     (int Fd, int Cmd, ...);
#endif  // __FCNTL_SYSCALLS_DECLARED

/* These system calls are also declared in stat.h */
#ifndef __STAT_SYSCALLS_DECLARED
  #define __STAT_SYSCALLS_DECLARED

  int     mkdir     (const char *, mode_t);
  int     fstat     (int, struct stat *);
  int     lstat     (const char *, struct stat *);
  int     stat      (const char *, struct stat *);
  int     chmod     (const char *, mode_t);
  mode_t  umask     (mode_t cmask);

#endif  // __STAT_SYSCALLS_DECLARED

// These are also declared in sys/types.h
#ifndef __OFF_T_SYSCALLS_DECLARED
  #define __OFF_T_SYSCALLS_DECLARED
  off_t   lseek     (int, off_t, int);
  int     truncate  (const char *, off_t);
  int     ftruncate (int, off_t);   //  IEEE Std 1003.1b-93
#endif /* __OFF_T_SYSCALLS_DECLARED */

/* EFI-specific Functions. */

  /** Mark an open file to be deleted when it is closed.

    @param[in]  fd    File descriptor for the open file.

    @retval   0   The flag was set successfully.
    @retval  -1   An invalid fd was specified.
  **/
  int       DeleteOnClose(int fd);

  /** Find and reserve a free File Descriptor.

  Returns the first free File Descriptor greater than or equal to the,
  already validated, fd specified by Minfd.

  @return   Returns -1 if there are no free FDs.  Otherwise returns the
            found fd.
  */
  int       FindFreeFD  (int MinFd);

  /** Validate that fd refers to a valid file descriptor.
    IsOpen is interpreted as follows:
      - Positive  fd must be OPEN
      - Zero      fd must be CLOSED
      - Negative  fd may be OPEN or CLOSED

    @retval TRUE  fd is VALID
    @retval FALSE fd is INVALID
  */
  BOOLEAN   ValidateFD (int fd, int IsOpen);


/* These system calls don't YET have EFI implementations. */
  int       reboot    (int, char *);
__END_DECLS

/*  The console output stream, stdout, supports cursor positioning via the
    lseek() function call.  The following entities facilitate packing the
    X and Y coordinates into the offset parameter of the lseek call.
*/
typedef struct {
  UINT32    Column;
  UINT32    Row;
} CURSOR_XY;

typedef union {
  UINT64      Offset;
  CURSOR_XY   XYpos;
} XY_OFFSET;

#endif  /* _EFI_SYS_CALL_H */
