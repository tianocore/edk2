/** @file
  EFI versions of NetBSD system calls.

  Copyright (c) 2010 - 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include  <Uefi.h>
#include  <Library/UefiLib.h>
#include  <Library/UefiBootServicesTableLib.h>
#include  <Library/BaseLib.h>
#include  <Library/MemoryAllocationLib.h>
#include  <Library/ShellLib.h>

#include  <LibConfig.h>
#include  <sys/EfiCdefs.h>

#include  <sys/ansi.h>
#include  <errno.h>
#include  <stdarg.h>
#include  <stdlib.h>
#include  <string.h>
#include  <wchar.h>
#include  <sys/poll.h>
#include  <sys/fcntl.h>
#include  <sys/stat.h>
#include  <sys/syslimits.h>
#include  <sys/filio.h>
#include  <Efi/SysEfi.h>
#include  <unistd.h>
#include  <kfile.h>
#include  <Device/Device.h>
#include  <Device/IIO.h>
#include  <MainData.h>
#include  <extern.h>

/* EFI versions of BSD system calls used in stdio */

/*  Validate that fd refers to a valid file descriptor.
    IsOpen is interpreted as follows:
      - Positive  fd must be OPEN
      - Zero      fd must be CLOSED
      - Negative  fd may be OPEN or CLOSED

    @retval TRUE  fd is VALID
    @retval FALSE fd is INVALID
*/
BOOLEAN
ValidateFD( int fd, int IsOpen)
{
  struct __filedes    *filp;
  BOOLEAN   retval = FALSE;

  if((fd >= 0) && (fd < OPEN_MAX)) {
    filp = &gMD->fdarray[fd];
    retval = TRUE;
    if(IsOpen >= 0) {
      retval = (BOOLEAN)((filp->f_iflags != 0)  &&    // TRUE if OPEN
                         FILE_IS_USABLE(filp));         // and Usable (not Larval or Closing)
      if(IsOpen == VALID_CLOSED) {
        retval = (BOOLEAN)!retval;                      // We want TRUE if CLOSED
      }
    }
  }
  return retval;
}

/* Find and reserve a free File Descriptor.

  Returns the first free File Descriptor greater than or equal to the,
  already validated, fd specified by Minfd.

  @return   Returns -1 if there are no free FDs.  Otherwise returns the
            found fd.
*/
int
FindFreeFD( int MinFd )
{
  struct __filedes    *Mfd;
  int i;
  int fd = -1;

  Mfd = gMD->fdarray;

  // Get an available fd
  for(i=MinFd; i < OPEN_MAX; ++i) {
    if(Mfd[i].f_iflags == 0) {
      Mfd[i].f_iflags = FIF_LARVAL; // Temporarily mark this fd as reserved
      fd = i;
      break;
    }
  }
  return fd;
}

/* Mark that an open file is to be deleted when closed. */
int
DeleteOnClose(int fd)
{
  int   retval = 0;

  if(ValidateFD( fd, VALID_OPEN)) {
    gMD->fdarray[fd].f_iflags |= FIF_DELCLOSE;
  }
  else {
    errno = EBADF;
    retval = -1;
  }
  return retval;
}

/** The isatty() function tests whether fd, an open file descriptor,
    is associated with a terminal device.

    @param[in]  fd  File Descriptor for the file to be examined.

    @retval   1   fd is associated with a terminal.
    @retval   0   fd is not associated with a terminal.  errno is set to
                  EBADF if fd is not a valid open FD.
**/
int
isatty  (int fd)
{
  int   retval = 0;
  struct __filedes *Fp;

  if(ValidateFD( fd, VALID_OPEN)) {
    Fp = &gMD->fdarray[fd];
    retval =  (Fp->f_iflags & _S_ITTY) ? 1 : 0;
  }
  else {
    errno = EBADF;
  }
  return retval;
}

/** Determine if file descriptor fd is a duplicate of some other fd.

    @param[in]    fd    The file descriptor to check.

    @retval   TRUE    fd is a duplicate of another fd.
    @retval   FALSE   fd is unique.
**/
static BOOLEAN
IsDupFd( int fd)
{
  void * DevData;
  const struct fileops   *FileOps;
  int                   i;
  BOOLEAN               Ret = FALSE;

  if(ValidateFD( fd, VALID_OPEN )) {
    FileOps = gMD->fdarray[fd].f_ops;
    DevData = gMD->fdarray[fd].devdata;
    for(i=0; i < OPEN_MAX; ++i) {
      if(i == fd)   continue;
      if(ValidateFD( i, VALID_OPEN )) {   // TRUE if fd is valid and OPEN
        if((gMD->fdarray[i].f_ops == FileOps)
          &&(gMD->fdarray[i].devdata == DevData )) {
          Ret = TRUE;
          break;
        }
      }
    }
  }
  return Ret;
}

/** Worker function to Close a file and set its fd to the specified state.

    @param[in]    fd          The file descriptor to close.
    @param[in]    NewState    State to set the fd to after the file is closed.

    @retval    0    The operation completed successfully.
    @retval   -1    The operation failed.  Further information is in errno.
                      * EBADF   fd is not a valid or open file descriptor.
**/
static int
_closeX  (int fd, int NewState)
{
  struct __filedes     *Fp;
  int                   retval = 0;

  // Verify my pointers and get my FD.
  if(ValidateFD( fd, VALID_OPEN )) {
    Fp = &gMD->fdarray[fd];
    // Check if there are other users of this FileHandle
    if(Fp->RefCount == 1) { // There should be no other users
    if(! IsDupFd(fd)) {
      // Only do the close if no one else is using the FileHandle
      if(Fp->f_iflags & FIF_DELCLOSE) {
        /* Handle files marked "Delete on Close". */
        if(Fp->f_ops->fo_delete != NULL) {
          retval = Fp->f_ops->fo_delete(Fp);
        }
      }
      else {
          retval = Fp->f_ops->fo_close( Fp);
      }
    }
      Fp->f_iflags = NewState;    // Close this FD or reserve it
      Fp->RefCount = 0;           // No one using this FD
    }
    else {
      --Fp->RefCount;   /* One less user of this FD */
    }
  }
  else {
    // Bad FD
    retval = -1;
    errno = EBADF;
  }
  return retval;
}

/** The close() function deallocates the file descriptor indicated by fd.
    To deallocate means to make the file descriptor available for return by
    subsequent calls to open() or other functions that allocate file
    descriptors. All outstanding record locks owned by the process on the file
    associated with the file descriptor are removed (that is, unlocked).

    @param[in]    fd          Descriptor for the File to close.

    @retval   0     Successful completion.
    @retval   -1    An error occurred and errno is set to identify the error.
**/
int
close  (int fd)
{
  return _closeX(fd, 0);
}

/** Delete the file specified by path.

    @param[in]    path  The MBCS path of the file to delete.

    @retval   -1  Unable to open the file specified by path.
    @retval   -1  If (errno == EPERM), unlink is not permited for this file.
    @retval   -1  Low-level delete filed.  Reason is in errno.
    @retval   0   The file was successfully deleted.
**/
int
unlink (const char *path)
{
  struct __filedes     *Fp;
  int                   fd;
  int                   retval = -1;

  EFIerrno = RETURN_SUCCESS;

  fd = open(path, O_WRONLY, 0);
  if(fd >= 0) {
    Fp = &gMD->fdarray[fd];

    if(Fp->f_ops->fo_delete != NULL) {
      retval = Fp->f_ops->fo_delete(Fp);
  }
    Fp->f_iflags = 0;    // Close this FD
    Fp->RefCount = 0;    // No one using this FD
  }
  return retval;
}

/** The fcntl() function shall perform the operations described below on open
    files. The fildes argument is a file descriptor.

    The available values for cmd are defined in <fcntl.h> and are as follows:
      - F_DUPFD - Return a new file descriptor which shall be the lowest
                  numbered available (that is, not already open) file
                  descriptor greater than or equal to the third argument, arg,
                  taken as an integer of type int. The new file descriptor
                  shall refer to the same open file description as the original
                  file descriptor, and shall share any locks. The FD_CLOEXEC
                  flag associated with the new file descriptor shall be cleared
                  to keep the file open across calls to one of the exec functions.
      - F_GETFD - Get the file descriptor flags defined in <fcntl.h> that are
                  associated with the file descriptor fildes. File descriptor
                  flags are associated with a single file descriptor and do not
                  affect other file descriptors that refer to the same file.
      - F_SETFD - Set the file descriptor flags defined in <fcntl.h>, that are
                  associated with fildes, to the third argument, arg, taken
                  as type int. If the FD_CLOEXEC flag in the third argument
                  is 0, the file shall remain open across the exec
                  functions; otherwise, the file shall be closed upon
                  successful execution of one of the exec functions.
      - F_GETFL - Get the file status flags and file access modes, defined in
                  <fcntl.h>, for the file description associated with fildes.
                  The file access modes can be extracted from the return
                  value using the mask O_ACCMODE, which is defined in
                  <fcntl.h>. File status flags and file access modes are
                  associated with the file description and do not affect
                  other file descriptors that refer to the same file with
                  different open file descriptions.
      - F_SETFL - Set the file status flags, defined in <fcntl.h>, for the file
                  description associated with fildes from the corresponding
                  bits in the third argument, arg, taken as type int. Bits
                  corresponding to the file access mode and the file creation
                  flags, as defined in <fcntl.h>, that are set in arg shall
                  be ignored. If any bits in arg other than those mentioned
                  here are changed by the application, the result is unspecified.
      - F_GETOWN -  If fildes refers to a socket, get the process or process group
                  ID specified to receive SIGURG signals when out-of-band
                  data is available. Positive values indicate a process ID;
                  negative values, other than -1, indicate a process group
                  ID. If fildes does not refer to a socket, the results are
                  unspecified.
      - F_SETOWN -  If fildes refers to a socket, set the process or process
                  group ID specified to receive SIGURG signals when
                  out-of-band data is available, using the value of the third
                  argument, arg, taken as type int. Positive values indicate
                  a process ID; negative values, other than -1, indicate a
                  process group ID. If fildes does not refer to a socket, the
                  results are unspecified.

    The fcntl() function shall fail if:

    [EBADF]       The fildes argument is not a valid open file descriptor.
    [EINVAL]      The cmd argument is invalid, or the cmd argument is F_DUPFD
                  and arg is negative or greater than or equal to {OPEN_MAX}.
    [EMFILE]      The argument cmd is F_DUPFD and {OPEN_MAX} file descriptors
                  are currently open in the calling process, or no file
                  descriptors greater than or equal to arg are available.
    [EOVERFLOW]   One of the values to be returned cannot be represented correctly.

    @param[in]      fildes    Descriptor for the file to be controlled.
    @param[in]      cmd       Command to be acted upon.
    @param[in,out]  ...       Optional additional parameters as required by cmd.

    @return   Upon successful completion, the value returned shall depend on
              cmd as follows:
                - F_DUPFD - A new file descriptor.
                - F_GETFD - Value of flags defined in <fcntl.h>. The return value
                            shall not be negative.
                - F_SETFD - Value other than -1.
                - F_GETFL - Value of file status flags and access modes. The return
                            value is not negative.
                - F_SETFL - Value other than -1.
                - F_GETOWN  - Value of the socket owner process or process group;
                            this will not be -1.
                - F_SETOWN - Value other than -1.
              Otherwise, -1 shall be returned and errno set to indicate the error.

**/
int
fcntl     (int fildes, int cmd, ...)
{
  va_list             p3;
  struct __filedes   *MyFd;
  int                 retval = -1;
  int                 temp;

//Print(L"%a( %d, %d, ...)\n", __func__, fildes, cmd);
  va_start(p3, cmd);

  if(ValidateFD( fildes, VALID_OPEN )) {
    MyFd = &gMD->fdarray[fildes];

    switch(cmd) {
      case F_DUPFD:
        temp = va_arg(p3, int);
        if(ValidateFD( temp, VALID_DONT_CARE )) {
          temp = FindFreeFD( temp );
          if(temp < 0) {
            errno = EMFILE;
            break;
          }
          /* temp is now a valid fd reserved for further use
             so copy fd into temp.
          */
          (void)memcpy(&gMD->fdarray[temp], MyFd, sizeof(struct __filedes));
          retval = temp;
        }
        else {
          errno = EINVAL;
        }
        break;

      case F_SETFL:
        retval = MyFd->Oflags;        // Get original value
        temp = va_arg(p3, int);
        temp &= O_SETMASK;            // Only certain bits can be set
        temp |= retval & O_SETMASK;
        MyFd->Oflags = temp;          // Set new value
        break;

      case F_SETFD:
        retval = MyFd->f_iflags;
        break;
      //case F_SETOWN:
      //  retval = MyFd->SocProc;
      //  MyFd->SocProc = va_arg(p3, int);
      //  break;
      case F_GETFD:
        retval = MyFd->f_iflags;
        break;
      case F_GETFL:
        retval = MyFd->Oflags;
        break;
      //case F_GETOWN:
      //  retval = MyFd->SocProc;
      //  break;
      default:
        errno  = EINVAL;
        break;
    }
  }
  else {
    // Bad FD
    errno = EBADF;
  }
  va_end(p3);
  return retval;;
}

/** The dup() function provides an alternative interface to the
    service provided by fcntl() using the F_DUPFD command. The call:
      - fid = dup(fildes);
    shall be equivalent to:
      - fid = fcntl(fildes, F_DUPFD, 0);

    @param[in]    fildes    Descriptor for the file to be examined.

    @return   Upon successful completion a non-negative integer, namely the
              file descriptor, shall be returned; otherwise, -1 shall be
              returned and errno set to indicate the error.
**/
int
dup   (int fildes)
{
  return fcntl(fildes, F_DUPFD, 0);
}

/** Make fildes2 refer to a duplicate of fildes.

    The dup2() function provides an alternative interface to the
    service provided by fcntl() using the F_DUPFD command. The call:
      - fid = dup2(fildes, fildes2);
    shall be equivalent to:
      - close(fildes2);
      - fid = fcntl(fildes, F_DUPFD, fildes2);
    except for the following:
      - If fildes2 is less than 0 or greater than or equal to {OPEN_MAX},
        dup2() shall return -1 with errno set to [EBADF].
      - If fildes is a valid file descriptor and is equal to fildes2, dup2()
        shall return fildes2 without closing it.
      - If fildes is not a valid file descriptor, dup2() shall return -1 and
        shall not close fildes2.
      - The value returned shall be equal to the value of fildes2 upon
        successful completion, or -1 upon failure.

    @param[in]  fildes    File Descriptor to be duplicated.
    @param[in]  fildes2   File Descriptor to be made a duplicate of fildes.

    @return   Upon successful completion a non-negative integer, namely
              fildes2, shall be returned; otherwise, -1 shall be
              returned and errno set to EBADF indicate the error.
**/
int
dup2    (int fildes, int fildes2)
{
  int retval = -1;

  if(ValidateFD( fildes, VALID_OPEN)) {
    retval = fildes2;
    if( fildes != fildes2) {
      if(ValidateFD( fildes2, VALID_DONT_CARE)) {
        gMD->fdarray[fildes2].f_iflags = FIF_LARVAL;  // Mark the file closed, but reserved
        (void)memcpy(&gMD->fdarray[fildes2],      // Duplicate fildes into fildes2
                     &gMD->fdarray[fildes], sizeof(struct __filedes));
        gMD->fdarray[fildes2].MyFD = (UINT16)fildes2;
      }
      else {
        errno = EBADF;
        retval = -1;
      }
    }
  }
  else {
    errno = EBADF;
  }
  return retval;
}

/** Reposition a file's read/write offset.

    The lseek() function repositions the offset of the file descriptor fildes
    to the argument offset according to the directive how.  The argument
    fildes must be an open file descriptor.  lseek() repositions the file
    pointer fildes as follows:

      - If how is SEEK_SET, the offset is set to offset bytes.

      - If how is SEEK_CUR, the offset is set to its current location
        plus offset bytes.

      - If how is SEEK_END, the offset is set to the size of the file
        plus offset bytes.

    The lseek() function allows the file offset to be set beyond the end of
    the existing end-of-file of the file.  If data is later written at this
    point, subsequent reads of the data in the gap return bytes of zeros
    (until data is actually written into the gap).

    Some devices are incapable of seeking.  The value of the pointer associ-
    ated with such a device is undefined.

    @param[in]  fd        Descriptor for the File to be affected.
    @param[in]  offset    Value to adjust the file position by.
    @param[in]  how       How the file position is to be adjusted.

    @return   Upon successful completion, lseek() returns the resulting offset
              location as measured in bytes from the beginning of the file.
              Otherwise, a value of -1 is returned and errno is set to
              indicate the error.
**/
__off_t
lseek (int fd, __off_t offset, int how)
{
  __off_t             CurPos = -1;
//  RETURN_STATUS       Status = RETURN_SUCCESS;
  struct __filedes   *filp;

  EFIerrno = RETURN_SUCCESS;    // In case of error without an EFI call

  if( how == SEEK_SET || how == SEEK_CUR  || how == SEEK_END) {
    if(ValidateFD( fd, VALID_OPEN)) {
      filp = &gMD->fdarray[fd];
      // Both of our parameters have been verified as valid
      CurPos = filp->f_ops->fo_lseek( filp, offset, how);
      if(CurPos >= 0) {
        filp->f_offset = CurPos;
      }
    }
    else {
      errno = EBADF;  // Bad File Descriptor
    }
  }
  else {
    errno = EINVAL;   // Invalid how argument
  }
  return CurPos;
}

/** The directory path is created with the access permissions specified by
    perms.

    The directory is closed after it is created.

    @param[in]  path    The path to a directory to create.
    @param[in]  perms   Permissions as defined in <sys/stat.h>

    @retval   0   The directory was created successfully.
    @retval  -1   An error occurred and error codes are stored in errno and EFIerrno.
**/
int
mkdir (const char *path, __mode_t perms)
{
  wchar_t            *NewPath;
  DeviceNode         *Node;
  char               *GenI;
  RETURN_STATUS       Status;
  int                 Instance  = 0;
  int                 retval = 0;

  Status = ParsePath(path, &NewPath, &Node, &Instance, NULL);
  if(Status == RETURN_SUCCESS) {
    GenI = Node->InstanceList;
    if(GenI == NULL) {
      errno   = EPERM;
      retval  = -1;
      }
    else {
      //GenI += (Instance * Node->InstanceSize);
      retval = ((GenericInstance *)GenI)->Abstraction.fo_mkdir( path, perms);
      }
    free(NewPath);
    }
  else {
    retval = -1;
  }
  return retval;
}

/** Open a file.
    The open() function establishes the connection between a file and a file
    descriptor.  It creates an open file description that refers to a file
    and a file descriptor that refers to that open file description. The file
    descriptor is used by other I/O functions to refer to that file.

    The open() function returns a file descriptor for the named file that is
    the lowest file descriptor not currently open for that process. The open
    file description is new, and therefore the file descriptor shall not
    share it with any other process in the system.

    The file offset used to mark the current position within the file is set
    to the beginning of the file.

    The EFI ShellOpenFileByName() function is used to perform the low-level
    file open operation.  The primary task of open() is to translate from the
    flags used in the <stdio.h> environment to those used by the EFI function.

    The file status flags and file access modes of the open file description
    are set according to the value of oflags.

    Values for oflags are constructed by a bitwise-inclusive OR of flags from
    the following list, defined in <fcntl.h>. Applications shall specify
    exactly one of { O_RDONLY, O_RDWR, O_WRONLY } in the value of oflags.
    Any combination of { O_NONBLOCK, O_APPEND, O_CREAT, O_TRUNC, O_EXCL } may
    also be specified in oflags.

    The only valid flag combinations for ShellOpenFileByName() are:
      - Read
      - Read/Write
      - Create/Read/Write

    Values for mode specify the access permissions for newly created files.
    The mode value is saved in the FD to indicate permissions for further operations.

    O_RDONLY      -- flags = EFI_FILE_MODE_READ -- this is always done
    O_WRONLY      -- flags |= EFI_FILE_MODE_WRITE
    O_RDWR        -- flags |= EFI_FILE_MODE_WRITE -- READ is already set

    O_NONBLOCK    -- ignored
    O_APPEND      -- Seek to EOF before every write
    O_CREAT       -- flags |= EFI_FILE_MODE_CREATE
    O_TRUNC       -- delete first then create new
    O_EXCL        -- if O_CREAT is also set, open will fail if the file already exists.

    @param[in]    Path      The path argument points to a pathname naming the
                            object to be opened.
    @param[in]    oflags    File status flags and file access modes of the
                            open file description.
    @param[in]    mode      File access permission bits as defined in
                            <sys/stat.h>.  Only used if a file is created
                            as a result of the open.

    @return     Upon successful completion, open() opens the file and returns
                a non-negative integer representing the lowest numbered
                unused file descriptor. Otherwise, open returns -1 and sets
                errno to indicate the error. If a negative value is
                returned, no files are created or modified.
                  - EMFILE - No file descriptors available -- Max number already open.
                  - EINVAL - Bad value specified for oflags or mode.
                  - ENOMEM - Failure allocating memory for internal buffers.
                  - EEXIST - File exists and open attempted with (O_EXCL | O_CREAT) set.
                  - EIO - UEFI failure.  Check value in EFIerrno.
**/
int
open(
  const char *path,
  int oflags,
  int mode
  )
{
  wchar_t              *NewPath;
  wchar_t              *MPath;
  DeviceNode           *Node;
  struct __filedes     *filp;
  struct termios       *Termio;
  int                   Instance  = 0;
  RETURN_STATUS         Status;
  UINT32                OpenMode;
  int                   fd = -1;
  int                   doresult;

  Status = ParsePath(path, &NewPath, &Node, &Instance, &MPath);
  if(Status == RETURN_SUCCESS) {
    if((Node == NULL)               ||
       (Node->InstanceList == NULL))
    {
      errno   = EPERM;
    }
    else {
  // Could add a test to see if the file name begins with a period.
  // If it does, then add the HIDDEN flag to Attributes.

  // Get an available fd
      fd = FindFreeFD( VALID_CLOSED );

  if( fd < 0 ) {
    // All available FDs are in use
    errno = EMFILE;
  }
      else {
      filp = &gMD->fdarray[fd];
      // Save the flags and mode in the File Descriptor
      filp->Oflags = oflags;
      filp->Omode = mode;

        doresult = Node->OpenFunc(Node, filp, Instance, NewPath, MPath);
      if(doresult < 0) {
        filp->f_iflags = 0;   // Release this FD
        fd = -1;              // Indicate an error
      }
      else {
        // Build our final f_iflags value
        OpenMode  = ( mode & S_ACC_READ )  ? S_ACC_READ : 0;
        OpenMode |= ( mode & S_ACC_WRITE ) ? S_ACC_WRITE : 0;

        filp->f_iflags |= OpenMode;

        if((oflags & O_TTY_INIT) && (filp->f_iflags & _S_ITTY) && (filp->devdata != NULL)) {
          // Initialize the device's termios flags to a "sane" value
          Termio = &((cIIO *)filp->devdata)->Termio;
          Termio->c_iflag = ICRNL | IGNSPEC;
          Termio->c_oflag = OPOST | ONLCR | OXTABS | ONOEOT | ONOCR | ONLRET | OCTRL;
          Termio->c_lflag = ECHO | ECHOE | ECHONL | ICANON;
          Termio->c_cc[VERASE]  = 0x08;   // ^H Backspace
          Termio->c_cc[VKILL]   = 0x15;   // ^U
          Termio->c_cc[VINTR]   = 0x03;   // ^C Interrupt character
        }
        ++filp->RefCount;
        FILE_SET_MATURE(filp);
      }
          }
    }
    free(NewPath);
        }
    free(MPath);    // We don't need this any more.

  // return the fd of our now open file
  return fd;
}


/**
  Poll a list of file descriptors.

  The ::poll routine waits for up to timeout milliseconds for an event
  to occur on one or more of the file descriptors listed.  The event
  types of interested are specified for each file descriptor in the events
  field.  The actual event detected is returned in the revents field of
  the array.  The
  <a href="http://pubs.opengroup.org/onlinepubs/9699919799/functions/poll.html">POSIX</a>
  documentation is available online.

  @param[in]  pfd       Address of an array of pollfd structures.

  @param[in]  nfds      Number of elements in the array of pollfd structures.

  @param[in]  timeout   Length of time in milliseconds to wait for the event

  @return     The number of file descriptors with detected events.  Zero
              indicates that the call timed out and -1 indicates an error.

 **/
int
poll (
  struct pollfd * pfd,
  nfds_t nfds,
  int timeout
  )
{
  struct __filedes * pDescriptor;
  struct pollfd * pEnd;
  struct pollfd * pPollFD;
  int SelectedFDs;
  EFI_STATUS Status;
  EFI_EVENT Timer;
  UINT64 TimerTicks;

  //
  //  Create the timer for the timeout
  //
  Timer = NULL;
  Status = EFI_SUCCESS;
  if ( INFTIM != timeout ) {
    Status = gBS->CreateEvent ( EVT_TIMER,
                                TPL_NOTIFY,
                                NULL,
                                NULL,
                                &Timer );
    if ( !EFI_ERROR ( Status )) {
      //
      //  Start the timeout timer
      //
      TimerTicks = timeout;
      TimerTicks *= 1000 * 10;
      Status = gBS->SetTimer ( Timer,
                               TimerRelative,
                               TimerTicks );
    }
    else {
      SelectedFDs = -1;
      errno = ENOMEM;
    }
  }
  if ( !EFI_ERROR ( Status )) {
    //
    //  Poll until an event is detected or the timer fires
    //
    SelectedFDs = 0;
    errno = 0;
    do {
      //
      //  Poll the list of file descriptors
      //
      pPollFD = pfd;
      pEnd = &pPollFD [ nfds ];
      while ( pEnd > pPollFD ) {
        //
        //  Validate the file descriptor
        //
        if ( !ValidateFD ( pPollFD->fd, VALID_OPEN )) {
          errno = EINVAL;
          return -1;
        }

        //
        //  Poll the device or file
        //
        pDescriptor = &gMD->fdarray [ pPollFD->fd ];
        pPollFD->revents = pDescriptor->f_ops->fo_poll ( pDescriptor,
                                                         pPollFD->events );

        //
        //  Determine if this file descriptor detected an event
        //
        if ( 0 != pPollFD->revents ) {
          //
          //  Select this descriptor
          //
          SelectedFDs += 1;
        }

        //
        //  Set the next file descriptor
        //
        pPollFD += 1;
      }

      //
      //  Check for timeout
      //
      if ( NULL != Timer ) {
        Status = gBS->CheckEvent ( Timer );
        if ( EFI_SUCCESS == Status ) {
          //
          //  Timeout
          //
          break;
        }
        else if ( EFI_NOT_READY == Status ) {
          Status = EFI_SUCCESS;
    }
    }
    } while (( 0 == SelectedFDs )
        && ( EFI_SUCCESS == Status ));

    //
    //  Stop the timer
    //
    if ( NULL != Timer ) {
      gBS->SetTimer ( Timer,
                      TimerCancel,
                      0 );
  }
  }
  else {
    SelectedFDs = -1;
    errno = EAGAIN;
  }

  //
  //  Release the timer
  //
  if ( NULL != Timer ) {
    gBS->CloseEvent ( Timer );
  }

  //
  //  Return the number of selected file system descriptors
  //
  return SelectedFDs;
}


/** The rename() function changes the name of a file.
    The From argument points to the pathname of the file to be renamed. The To
    argument points to the new pathname of the file.

    If the From argument points to the pathname of a file that is not a
    directory, the To argument shall not point to the pathname of a
    directory. If the file named by the To argument exists, it shall be
    removed and From renamed to To. Write access permission is required for
    both the directory containing old and the directory containing To.

    If the From argument points to the pathname of a directory, the To
    argument shall not point to the pathname of a file that is not a
    directory. If the directory named by the To argument exists, it shall be
    removed and From renamed to To.

    The To pathname shall not contain a path prefix that names From. Write
    access permission is required for the directory containing From and the
    directory containing To. If the From argument points to the pathname of a
    directory, write access permission may be required for the directory named
    by From, and, if it exists, the directory named by To.

    If the rename() function fails for any reason other than [EIO], any file
    named by To shall be unaffected.

    @param[in]  From    Path to the file to be renamed.
    @param[in]  To      The new name of From.

    @retval   0     Successful completion.
    @retval   -1    An error has occured and errno has been set to further specify the error.
                    Neither the file named by From nor the file named by To are
                    changed or created.
                      - ENXIO: Path specified is not supported by any loaded driver.
                      - ENOMEM: Insufficient memory to calloc a MapName buffer.
                      - EINVAL: The path parameter is not valid.
**/
int
rename(
  const char *From,
  const char *To
  )
{
  wchar_t            *FromPath;
  DeviceNode         *FromNode;
  char               *GenI;
  int                 Instance    = 0;
  RETURN_STATUS       Status;
  int                 retval      = -1;

  Status = ParsePath(From, &FromPath, &FromNode, &Instance, NULL);
  if(Status == RETURN_SUCCESS) {
    GenI = FromNode->InstanceList;
    if(GenI == NULL) {
      errno   = EPERM;
      retval  = -1;
      }
      else {
      //GenI += (Instance * FromNode->InstanceSize);
      retval = ((GenericInstance *)GenI)->Abstraction.fo_rename( From, To);
              }
    free(FromPath);
            }
  return retval;
}

/** Delete a specified directory.

    @param[in]  path    Path to the directory to delete.

    @retval   -1    The directory couldn't be opened (doesn't exist).
    @retval   -1    The directory wasn't empty or an IO error occured.
**/
int
rmdir(
  const char *path
  )
{
  struct __filedes   *filp;
  int                 fd;
  int                 retval = -1;

  fd = open(path, O_RDWR, 0);
  if(fd >= 0) {
    filp = &gMD->fdarray[fd];

    retval = filp->f_ops->fo_rmdir(filp);
    filp->f_iflags = 0;           // Close this FD
    filp->RefCount = 0;           // No one using this FD
  }
  return retval;
}

/** The fstat() function obtains information about an open file associated
    with the file descriptor fd, and writes it to the area pointed to
    by statbuf.

    The statbuf argument is a pointer to a stat structure, as defined
    in <sys/stat.h>, into which information is placed concerning the file.

    The structure members st_mode, st_ino, st_dev, st_uid, st_gid, st_atime,
    st_ctime, and st_mtime shall have meaningful values. The value of the
    member st_nlink shall be set to the number of links to the file.

    The fstat() function shall update any time-related fields before writing
    into the stat structure.

    The fstat() function is implemented using the ShellGetFileInfo()
    function.

    The stat structure members which don't have direct analogs to EFI file
    information are filled in as follows:
      - st_mode     Populated with information from fd
      - st_ino      Set to zero.  (inode)
      - st_dev      Set to zero.
      - st_uid      Set to zero.
      - st_gid      Set to zero.
      - st_nlink    Set to one.

    @param[in]    fd        File descriptor as returned from open().
    @param[out]   statbuf   Buffer in which the file status is put.

    @retval    0  Successful Completion.
    @retval   -1  An error has occurred and errno has been set to
                  identify the error.
**/
int
fstat (int fd, struct stat *statbuf)
{
  int                 retval = -1;
  struct __filedes   *filp;

  if(ValidateFD( fd, VALID_OPEN)) {
    filp = &gMD->fdarray[fd];
    retval = filp->f_ops->fo_stat(filp, statbuf, NULL);
      }
      else {
    errno   =  EBADF;
      }
  return retval;
}

/** Obtains information about the file pointed to by path.

    Opens the file pointed to by path, calls _EFI_FileInfo with the file's handle,
    then closes the file.

    @param[in]    path      Path to the file to obtain information about.
    @param[out]   statbuf   Buffer in which the file status is put.

    @retval    0  Successful Completion.
    @retval   -1  An error has occurred and errno has been set to
                  identify the error.
**/
int
stat   (const char *path, struct stat *statbuf)
{
  int                 fd;
  int                 retval  = -1;
  struct __filedes   *filp;

  fd = open(path, O_RDONLY, 0);
  if(fd >= 0) {
    filp = &gMD->fdarray[fd];
    retval = filp->f_ops->fo_stat( filp, statbuf, NULL);
    close(fd);
  }
  return retval;
}

/**  Same as stat since EFI doesn't have symbolic links.

    @param[in]    path      Path to the file to obtain information about.
    @param[out]   statbuf   Buffer in which the file status is put.

    @retval    0  Successful Completion.
    @retval   -1  An error has occurred and errno has been set to
                  identify the error.
**/
int
lstat (const char *path, struct stat *statbuf)
{
  return stat(path, statbuf);
}

/** Control a device.

    @param[in]        fd        Descriptor for the file to be acted upon.
    @param[in]        request   Specifies the operation to perform.
    @param[in,out]    ...       Zero or more parameters as required for request.

    @retval   >=0   The operation completed successfully.
    @retval   -1    An error occured.  More information is in errno.
**/
int
ioctl(
  int             fd,
  unsigned long   request,
  ...
  )
{
  int                 retval = -1;
  struct __filedes   *filp;
  va_list             argp;

  va_start(argp, request);

  if(ValidateFD( fd, VALID_OPEN)) {
    filp = &gMD->fdarray[fd];

    if(request == FIODLEX) {
      /* set Delete-on-Close */
      filp->f_iflags |= FIF_DELCLOSE;
      retval = 0;
    }
    else if(request == FIONDLEX) {
      /* clear Delete-on-Close */
      filp->f_iflags &= ~FIF_DELCLOSE;
      retval = 0;
    }
    else {
      /* All other requests. */
      retval = filp->f_ops->fo_ioctl(filp, request, argp);
    }
  }
  else {
    errno   =  EBADF;
  }
  va_end(argp);

  return retval;
}

/** Read from a file.

    The read() function shall attempt to read nbyte bytes from the file
    associated with the open file descriptor, fildes, into the buffer pointed
    to by buf.

    Before any action described below is taken, and if nbyte is zero, the
    read() function may detect and return errors as described below. In the
    absence of errors, or if error detection is not performed, the read()
    function shall return zero and have no other results.

    On files that support seeking (for example, a regular file), the read()
    shall start at a position in the file given by the file offset associated
    with fildes. The file offset shall be incremented by the number of bytes
    actually read.

    Files that do not support seeking - for example, terminals - always read
    from the current position. The value of a file offset associated with
    such a file is undefined.

    No data transfer shall occur past the current end-of-file. If the
    starting position is at or after the end-of-file, 0 shall be returned.

    The read() function reads data previously written to a file. If any
    portion of a regular file prior to the end-of-file has not been written,
    read() shall return bytes with value 0. For example, lseek() allows the
    file offset to be set beyond the end of existing data in the file. If data
    is later written at this point, subsequent reads in the gap between the
    previous end of data and the newly written data shall return bytes with
    value 0 until data is written into the gap.

    Upon successful completion, where nbyte is greater than 0, read() shall
    mark for update the st_atime field of the file, and shall return the
    number of bytes read. This number shall never be greater than nbyte. The
    value returned may be less than nbyte if the number of bytes left in the
    file is less than nbyte, if the read() request was interrupted by a
    signal, or if the file is a pipe or FIFO or special file and has fewer
    than nbyte bytes immediately available for reading. For example, a read()
    from a file associated with a terminal may return one typed line of data.

    If fildes does not refer to a directory, the function reads the requested
    number of bytes from the file at the file's current position and returns
    them in buf. If the read goes beyond the end of the file, the read
    length is truncated to the end of the file. The file's current position is
    increased by the number of bytes returned.

    If fildes refers to a directory, the function reads the directory entry at
    the file's current position and returns the entry in buf. If buf
    is not large enough to hold the current directory entry, then
    errno is set to EBUFSIZE, EFIerrno is set to EFI_BUFFER_TOO_SMALL, and the
    current file position is not updated. The size of the buffer needed to read
    the entry will be returned as a negative number. On success, the current
    position is updated to the next directory entry. If there are no more
    directory entries, the read returns a zero-length buffer.
    EFI_FILE_INFO is the structure returned as the directory entry.

    @param[in]    fildes  Descriptor of the file to be read.
    @param[out]   buf     Pointer to location in which to store the read data.
    @param[in]    nbyte   Maximum number of bytes to be read.

    @return   Upon successful completion, read() returns a non-negative integer
              indicating the number of bytes actually read. Otherwise, the
              functions return a negative value and sets errno to indicate the
              error.  If errno is EBUFSIZE, the absolute value of the
              return value indicates the size of the buffer needed to read
              the directory entry.
**/
ssize_t
read   (int fildes, void *buf, size_t nbyte)
{
  struct __filedes *filp;
  cIIO             *IIO;
  ssize_t           BufSize;

  BufSize = (ssize_t)nbyte;
  if(BufSize > 0) {
    if(ValidateFD( fildes, VALID_OPEN)) {
      filp = &gMD->fdarray[fildes];

      IIO = filp->devdata;
      if(isatty(fildes) && (IIO != NULL)) {
        BufSize = IIO->Read(filp, nbyte, buf);
      }
      else {
        BufSize = filp->f_ops->fo_read(filp, &filp->f_offset, nbyte, buf);
      }
    }
    else {
      errno = EBADF;
      BufSize = -1;
    }
  }
  return BufSize;
}

/** Write data to a file.

    This function writes the specified number of bytes to the file at the current
    file position. The current file position is advanced the actual number of bytes
    written, which is returned in BufferSize. Partial writes only occur when there
    has been a data error during the write attempt (such as "volume space full").
    The file is automatically grown to hold the data if required. Direct writes to
    opened directories are not supported.

    If fildes refers to a terminal device, isatty() returns TRUE, a partial write
    will occur if a NULL or EOF character is encountered before n characters have
    been written.  Characters inserted due to line-end translations will not be
    counted.  Unconvertable characters are translated into the UEFI character
    BLOCKELEMENT_LIGHT_SHADE.

    Since the UEFI console device works on wide characters, the buffer is assumed
    to contain a single-byte character stream which is then translated to wide
    characters using the mbtowc() functions.  The resulting wide character stream
    is what is actually sent to the UEFI console.

    @param[in]  fd      Descriptor of file to be written to.
    @param[in]  buf     Pointer to data to write to the file.
    @param[in]  nbyte   Number of bytes to be written to the file.

    @retval   >=0   Number of bytes actually written to the file.
    @retval   <0    An error occurred.  More data is provided by errno.
**/
ssize_t
write  (int fd, const void *buf, size_t nbyte)
{
  struct __filedes *filp;
  cIIO             *IIO;
  ssize_t           BufSize;

  BufSize = (ssize_t)nbyte;

  if(ValidateFD( fd, VALID_OPEN)) {
    filp = &gMD->fdarray[fd];
    if ((filp->Oflags & O_ACCMODE) != 0) {
      // File is open for writing
      IIO = filp->devdata;
      if(isatty(fd) && (IIO != NULL)) {
        // Output to an Interactive I/O device
        BufSize = IIO->Write(filp, buf, nbyte);
      }
      else {
        // Output to a file, socket, pipe, etc.
        BufSize = filp->f_ops->fo_write(filp, &filp->f_offset, nbyte, buf);
      }
    }
    else {
      // File is NOT open for writing
      errno = EINVAL;
      BufSize = -1;
    }
  }
  else {
    // fd is not for a valid open file
    errno = EBADF;
    BufSize = -1;
  }
  return BufSize;
}

/** Gets the current working directory.

  The getcwd() function shall place an absolute pathname of the current
  working directory in the array pointed to by buf, and return buf.The
  size argument is the size in bytes of the character array pointed to
  by the buf argument.

  @param[in,out] buf    The buffer to fill.
  @param[in]     size   The number of bytes in buffer.

  @retval NULL          The function failed.  The value in errno provides
                        further information about the cause of the failure.
                        Values for errno are:
                          - EINVAL: buf is NULL or size is zero.
                          - ENOENT: directory does not exist.
                          - ERANGE: buf size is too small to hold CWD

  @retval buf           The function completed successfully.
**/
char
*getcwd (char *buf, size_t size)
{
  CONST CHAR16 *Cwd;

  if (size == 0 || buf == NULL) {
    errno = EINVAL;
    return NULL;
    }

  Cwd = ShellGetCurrentDir(NULL);
  if (Cwd == NULL) {
    errno = ENOENT;
    return NULL;
  }
  if (size < ((StrLen (Cwd) + 1) * sizeof (CHAR8))) {
    errno = ERANGE;
    return (NULL);
  }
  return (UnicodeStrToAsciiStr(Cwd, buf));
}

/** Change the current working directory.

  The chdir() function shall cause the directory named by the pathname
  pointed to by the path argument to become the current working directory;
  that is, the starting point for path searches for pathnames not beginning
  with '/'.

  @param[in] path   The new path to set.

  @retval   0   Operation completed successfully.
  @retval  -1   Function failed.  The value in errno provides more
                information on the cause of failure:
                  - EPERM: Operation not supported with this Shell version.
                  - ENOMEM: Unable to allocate memory.
                  - ENOENT: Target directory does not exist.

  @todo Add non-NEW-shell CWD changing.
**/
int
chdir (const char *path)
{
  CONST CHAR16 *Cwd;
  EFI_STATUS   Status;
  CHAR16       *UnicodePath;

  /* Old Shell does not support Set Current Dir. */
  if(gEfiShellProtocol != NULL) {
    Cwd = ShellGetCurrentDir(NULL);
    if (Cwd != NULL) {
      /* We have shell support */
      UnicodePath = AllocatePool(((AsciiStrLen (path) + 1) * sizeof (CHAR16)));
      if (UnicodePath == NULL) {
        errno = ENOMEM;
        return -1;
      }
      AsciiStrToUnicodeStr(path, UnicodePath);
      Status = gEfiShellProtocol->SetCurDir(NULL, UnicodePath);
      FreePool(UnicodePath);
      if (EFI_ERROR(Status)) {
        errno = ENOENT;
        return -1;
      } else {
        return 0;
      }
    }
  }
  /* Add here for non-shell */
  errno = EPERM;
  return -1;
}

/** Get the foreground process group ID associated with a terminal.

    Just returns the Image Handle for the requestor since UEFI does not have
    a concept of processes or groups.

    @param[in]    x   Ignored.

    @return   Returns the Image Handle of the application or driver which
              called this function.
**/
pid_t tcgetpgrp (int x)
{
  return ((pid_t)(UINTN)(gImageHandle));
}

/** Get the process group ID of the calling process.

    Just returns the Image Handle for the requestor since UEFI does not have
    a concept of processes or groups.

    @return   Returns the Image Handle of the application or driver which
              called this function.
**/
pid_t getpgrp(void)
{
  return ((pid_t)(UINTN)(gImageHandle));
}

/* Internal worker function for utimes.
    This works around an error produced by GCC when the va_* macros
    are used within a function with a fixed number of arguments.
*/
static
int
EFIAPI
va_Utimes(
  const char   *path,
  ...
  )
{
  struct __filedes   *filp;
  va_list             ap;
  int                 fd;
  int                 retval  = -1;

  va_start(ap, path);
  fd = open(path, O_RDWR, 0);
  if(fd >= 0) {
    filp = &gMD->fdarray[fd];
    retval = filp->f_ops->fo_ioctl( filp, FIOSETIME, ap);
    close(fd);
  }
  va_end(ap);
  return retval;
}

/** Set file access and modification times.

    @param[in]  path    Path to the file to be modified.
    @param[in]  times   Pointer to an array of two timeval structures

    @retval   0     File times successfully set.
    @retval   -1    An error occured.  Error type in errno.
**/
int
utimes(
  const char *path,
  const struct timeval *times
  )
{
  return va_Utimes(path, times);
}
