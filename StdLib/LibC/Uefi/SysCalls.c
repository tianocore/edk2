/** @file
  EFI versions of NetBSD system calls.

  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include  <Uefi.h>
#include  <Library/UefiLib.h>
#include  <Library/BaseLib.h>
#include  <Library/MemoryAllocationLib.h>
#include  <Library/ShellLib.h>

#include  <LibConfig.h>
#include  <sys/EfiCdefs.h>

#include  <sys/ansi.h>
#include  <errno.h>
#include  <stdarg.h>
#include  <string.h>
#include  <wchar.h>
#include  <sys/fcntl.h>
#include  <sys/stat.h>
#include  <sys/syslimits.h>
#include  "SysEfi.h"
#include  <MainData.h>
#include  <extern.h>      // Library/include/extern.h: Private to implementation
#include  <Efi/Console.h>

/* Macros only used in this file. */
// Parameters for the ValidateFD function.
#define VALID_OPEN         1
#define VALID_CLOSED       0
#define VALID_DONT_CARE   -1


/* EFI versions of BSD system calls used in stdio */

/* Normalize path so that forward slashes are replaced with backslashes.
    Backslashes are required for UEFI.
*/
static void
NormalizePath( const CHAR16 *path)
{
  CHAR16  *temp;

  for( temp = (CHAR16 *)path; *temp; ++temp) {
    if(*temp == L'/') {
      *temp = L'\\';
    }
  }
}

/*  Validate that fd refers to a valid file descriptor.
    IsOpen is interpreted as follows:
      - Positive  fd must be OPEN
      - Zero      fd must be CLOSED
      - Negative  fd may be OPEN or CLOSED

    @retval TRUE  fd is VALID
    @retval FALSE fd is INVALID
*/
static BOOLEAN
ValidateFD( int fd, int IsOpen)
{
  BOOLEAN   retval = FALSE;

  if((fd >= 0) && (fd < OPEN_MAX)) {
    retval = TRUE;
    if(IsOpen >= 0) {
      retval = (BOOLEAN)(gMD->fdarray[fd].State != 0);   // TRUE if OPEN
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
static int
FindFreeFD( int MinFd )
{
  struct __filedes    *Mfd;
  int i;
  int fd = -1;

  Mfd = gMD->fdarray;

  // Get an available fd
  for(i=MinFd; i < OPEN_MAX; ++i) {
    if(Mfd[i].State == 0) {
      Mfd[i].State = S_ISYSTEM; // Temporarily mark this fd as reserved
      fd = i;
      break;
    }
  }
  return fd;
}

/** The isatty() function tests whether fildes, an open file descriptor,
    is associated with a terminal device.

    @retval   1   fildes is associated with a terminal.
    @retval   0   fildes is not associated with a terminal.  errno is set to
                  EBADF if fildes is not a valid open FD.
**/
int
isatty  (int fildes)
{
  int   retval = 0;
  EFI_FILE_HANDLE   FileHandle;

  if(ValidateFD( fildes, VALID_OPEN)) {
    FileHandle = gMD->fdarray[fildes].FileHandle;
    retval =  (FileHandle >= &gMD->StdIo[0].Abstraction) &&
              (FileHandle <= &gMD->StdIo[2].Abstraction);
  }
  else {
    errno = EBADF;
  }
  return retval;
}

static BOOLEAN
IsDupFd( int fd)
{
  EFI_FILE_HANDLE       FileHandle;
  int                   i;
  BOOLEAN               Ret = FALSE;

  if(ValidateFD( fd, VALID_OPEN )) {
    FileHandle = gMD->fdarray[fd].FileHandle;
    for(i=0; i < OPEN_MAX; ++i) {
      if(i == fd)   continue;
      if(gMD->fdarray[i].State != 0) {   // TRUE if fd is OPEN
        if(gMD->fdarray[i].FileHandle == FileHandle) {
          Ret = TRUE;
          break;
        }
      }
    }
  }
  return Ret;
}

static int
_closeX  (int fd, int NewState)
{
  struct __filedes     *Mfd;
  RETURN_STATUS         Status;
  int                   retval = 0;

  Status = EFIerrno = RETURN_SUCCESS;  // In case of error before the EFI call.

  // Verify my pointers and get my FD.
  if(ValidateFD( fd, VALID_OPEN )) {
    Mfd = &gMD->fdarray[fd];
    // Check if there are duplicates using this FileHandle
    if(! IsDupFd(fd)) {
      // Only do the close if no one else is using the FileHandle
      if(isatty(fd)) {
        Status = Mfd->FileHandle->Close( Mfd->FileHandle);
      }
      else {
        Status = ShellCloseFile( (SHELL_FILE_HANDLE *)&Mfd->FileHandle);
      }
    }
    Mfd->State = NewState;   // Close this FD or reserve it
    if(Status != RETURN_SUCCESS) {
      errno = EFI2errno(Status);
      EFIerrno = Status;
      retval = -1;
    }
  }
  else {
    // Bad FD
    errno = EBADF;
    retval = -1;
  }
  return retval;
}

/** The close() function shall deallocate the file descriptor indicated by fd.
    To deallocate means to make the file descriptor available for return by
    subsequent calls to open() or other functions that allocate file
    descriptors. All outstanding record locks owned by the process on the file
    associated with the file descriptor shall be removed (that is, unlocked).

    @return   Upon successful completion, 0 shall be returned; otherwise,
              -1 shall be returned and errno set to indicate the error.
**/
int
close  (int fd)
{
  //Print(L"Closing fd %d\n", fd);
  return _closeX(fd, 0);
}

/* Wide character version of unlink */
int
Uunlink (const wchar_t *Path)
{
  EFI_FILE_HANDLE       FileHandle;
  RETURN_STATUS         Status;

  EFIerrno = RETURN_SUCCESS;

  NormalizePath( Path);
  // We can only delete open files.
  Status = ShellOpenFileByName( Path, (SHELL_FILE_HANDLE *)&FileHandle, 3, 0);
  if(Status != RETURN_SUCCESS) {
    errno = EFI2errno(Status);
    EFIerrno = Status;
    return -1;
  }
  Status = ShellDeleteFile( (SHELL_FILE_HANDLE *)&FileHandle);
  if(Status != RETURN_SUCCESS) {
    errno = EFI2errno(Status);
    EFIerrno = Status;
    return -1;
  }
  return 0;
}

/**
**/
int
unlink (const char *path)
{
  // Convert path from MBCS to WCS
  (void)AsciiStrToUnicodeStr( path, gMD->UString);

  return Uunlink(gMD->UString);
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
      //case F_SETFD:
      case F_SETFL:
        retval = MyFd->Oflags;        // Get original value
        temp = va_arg(p3, int);
        temp &= O_SETMASK;            // Only certain bits can be set
        temp |= retval & O_SETMASK;
        MyFd->Oflags = temp;          // Set new value
        break;
      //case F_SETFL:
      case F_SETFD:
        retval = MyFd->State;
        break;
      case F_SETOWN:
        retval = MyFd->SocProc;
        MyFd->SocProc = va_arg(p3, int);
        break;
      case F_GETFD:
        //retval = MyFd->Oflags;
        retval = MyFd->State;
        break;
      case F_GETFL:
        //retval = MyFd->State;
        retval = MyFd->Oflags;
        break;
      case F_GETOWN:
        retval = MyFd->SocProc;
        break;
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

    @return   Upon successful completion a non-negative integer, namely the
              file descriptor, shall be returned; otherwise, -1 shall be
              returned and errno set to indicate the error.
**/
int
dup   (int fildes)
{
  return fcntl(fildes, F_DUPFD, 0);
}

/** The dup2() function provides an alternative interface to the
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
        gMD->fdarray[fildes2].State = S_ISYSTEM;  // Mark the file closed, but reserved
        (void)memcpy(&gMD->fdarray[fildes2],      // Duplicate fildes into fildes2
                     &gMD->fdarray[fildes], sizeof(struct __filedes));
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

         If how is SEEK_SET, the offset is set to offset bytes.

         If how is SEEK_CUR, the offset is set to its current location
         plus offset bytes.

         If how is SEEK_END, the offset is set to the size of the file
         plus offset bytes.

    The lseek() function allows the file offset to be set beyond the end of
    the existing end-of-file of the file.  If data is later written at this
    point, subsequent reads of the data in the gap return bytes of zeros
    (until data is actually written into the gap).

    Some devices are incapable of seeking.  The value of the pointer associ-
    ated with such a device is undefined.

    @return   Upon successful completion, lseek() returns the resulting offset
              location as measured in bytes from the beginning of the file.
              Otherwise, a value of -1 is returned and errno is set to
              indicate the error.
**/
__off_t
lseek (int fildes, __off_t offset, int how)
{
  __off_t             CurPos = -1;
  RETURN_STATUS       Status = RETURN_SUCCESS;
  EFI_FILE_HANDLE     FileHandle;

  EFIerrno = RETURN_SUCCESS;    // In case of error without an EFI call

  if( how == SEEK_SET || how == SEEK_CUR  || how == SEEK_END) {
    if(ValidateFD( fildes, VALID_OPEN)) {
      // Both of our parameters have been verified as valid
      FileHandle = gMD->fdarray[fildes].FileHandle;
      CurPos = 0;
      if(isatty(fildes)) {
        Status = FileHandle->SetPosition( FileHandle, offset);
        CurPos = offset;
      }
      else {
        if(how != SEEK_SET) {
          // We are doing a relative seek
          if(how == SEEK_END) {
            // seeking relative to EOF, so position there first.
            Status = ShellSetFilePosition( (SHELL_FILE_HANDLE)FileHandle, 0xFFFFFFFFFFFFFFFFULL);
          }
          if(Status == RETURN_SUCCESS) {
            // Now, determine our current position.
            Status = ShellGetFilePosition( (SHELL_FILE_HANDLE)FileHandle, (UINT64 *)&CurPos);
          }
        }
        if(Status == RETURN_SUCCESS) {
          /* CurPos now indicates the point we are seeking from, so seek... */
          Status = ShellSetFilePosition( (SHELL_FILE_HANDLE)FileHandle, (UINT64)(CurPos + offset));
          if(Status == RETURN_SUCCESS) {
            // Now, determine our final position.
            Status = ShellGetFilePosition( (SHELL_FILE_HANDLE)FileHandle, (UINT64 *)&CurPos);
          }
        }
        if(Status != RETURN_SUCCESS) {
          EFIerrno = Status;
          CurPos = -1;
          if(Status == EFI_UNSUPPORTED) {
            errno = EISDIR;
          }
          else {
            errno = EFI2errno(Status);
          }
        }
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

    @retval   0   The directory was created successfully.
    @retval  -1   An error occurred and an error code is stored in errno.
**/
int
mkdir (const char *path, __mode_t perms)
{
  EFI_FILE_HANDLE   FileHandle;
  RETURN_STATUS     Status;
  EFI_FILE_INFO     *FileInfo;

  // Convert name from MBCS to WCS
  (void)AsciiStrToUnicodeStr( path, gMD->UString);
  NormalizePath( gMD->UString);

//Print(L"%a( \"%s\", 0x%8X)\n", __func__, gMD->UString, perms);
  Status = ShellCreateDirectory( gMD->UString, (SHELL_FILE_HANDLE *)&FileHandle);
  if(Status == RETURN_SUCCESS) {
    FileInfo = ShellGetFileInfo( FileHandle);
    if(FileInfo != NULL) {
      FileInfo->Attribute = Omode2EFI(perms);
      Status = ShellSetFileInfo( FileHandle, FileInfo);
      FreePool(FileInfo);
      if(Status == RETURN_SUCCESS) {
        (void)ShellCloseFile((SHELL_FILE_HANDLE *)&FileHandle);
        return 0;
      }
    }
  }
  errno = EFI2errno(Status);
  EFIerrno = Status;

  return -1;
}

/** Open a file.

    The EFI ShellOpenFileByName() function is used to perform the low-level
    file open operation.  The primary task of open() is to translate from the
    flags used in the <stdio.h> environment to those used by the EFI function.

    The only valid flag combinations for ShellOpenFileByName() are:
      - Read
      - Read/Write
      - Create/Read/Write

    The mode value is saved in the FD to indicate permissions for further operations.

    O_RDONLY      -- flags = EFI_FILE_MODE_READ -- this is always done
    O_WRONLY      -- flags |= EFI_FILE_MODE_WRITE
    O_RDWR        -- flags |= EFI_FILE_MODE_WRITE -- READ is already set

    O_NONBLOCK    -- ignored
    O_APPEND      -- Seek to EOF before every write
    O_CREAT       -- flags |= EFI_FILE_MODE_CREATE
    O_TRUNC       -- delete first then create new
    O_EXCL        -- if O_CREAT is also set, open will fail if the file already exists.
**/
int
open   (const char *name, int oflags, int mode)
{
  EFI_FILE_HANDLE       FileHandle;
  struct __filedes     *Mfd;
  RETURN_STATUS         Status;
  UINT64                OpenMode;
  UINT64                Attributes;
  int                   fd = -1;
  UINT32                NewState;

  EFIerrno = RETURN_SUCCESS;
  Mfd = gMD->fdarray;

  // Convert name from MBCS to WCS
  (void)AsciiStrToUnicodeStr( name, gMD->UString);
  NormalizePath( gMD->UString);

  // Convert oflags to Attributes
  OpenMode = Oflags2EFI(oflags);
  if(OpenMode == 0) {
    errno = EINVAL;
    return -1;
  }

  //Attributes = Omode2EFI(mode);
  Attributes = 0;

  // Could add a test to see if the file name begins with a period.
  // If it does, then add the HIDDEN flag to Attributes.

  // Get an available fd
  fd = FindFreeFD( 0 );

  if( fd < 0 ) {
    // All available FDs are in use
    errno = EMFILE;
    return -1;
  }

  Status = ConOpen( NULL, &FileHandle, gMD->UString, OpenMode, Attributes);
  if(Status == RETURN_NO_MAPPING) {
    // Not a console device, how about a regular file device?

    /* Do we care if the file already exists?
       If O_TRUNC, then delete the file.  It will be created anew subsequently.
       If O_EXCL, then error if the file exists and O_CREAT is set.

    !!!!!!!!! Change this to use ShellSetFileInfo() to actually truncate the file
    !!!!!!!!! instead of deleting and re-creating it.
    */
    if((oflags & O_TRUNC) || ((oflags & (O_EXCL | O_CREAT)) == (O_EXCL | O_CREAT))) {
      Status = ShellIsFile( gMD->UString );
      if(Status == RETURN_SUCCESS) {
        // The file exists
        if(oflags & O_TRUNC) {
          // We do a truncate by deleting the existing file and creating a new one.
          if(Uunlink(gMD->UString) != 0) {
            Mfd[fd].State = 0;    // Release our reservation on this FD
            return -1;  // errno and EFIerrno are already set.
          }
        }
        else if(oflags & (O_EXCL | O_CREAT)) {
          errno = EEXIST;
          EFIerrno = Status;
          Mfd[fd].State = 0;    // Release our reservation on this FD
          return -1;
        }
      }
    }
    // Call the EFI Shell's Open function
    Status = ShellOpenFileByName( gMD->UString, (SHELL_FILE_HANDLE *)&FileHandle, OpenMode, Attributes);
    if(RETURN_ERROR(Status)) {
      Mfd[fd].State = 0;    // Release our reservation on this FD
      // Set errno based upon Status
      errno = EFI2errno(Status);
      EFIerrno = Status;
      return -1;
    }
    // Successfully got a regular File
    NewState = S_IFREG;
  }
  else if(Status != RETURN_SUCCESS) {
    // Set errno based upon Status
    errno = EFI2errno(Status);
    EFIerrno = Status;
    return -1;
  }
  else {
    // Succesfully got a Console stream
    NewState = S_IFREG | _S_ITTY | _S_IFCHR;
  }

  // Update the info in the fd
  Mfd[fd].FileHandle = FileHandle;
  Mfd[fd].Oflags = oflags;
  Mfd[fd].Omode = mode;

  // Re-use OpenMode in order to build our final State value
  OpenMode  = ( mode & S_ACC_READ )  ? S_ACC_READ : 0;
  OpenMode |= ( mode & S_ACC_WRITE ) ? S_ACC_WRITE : 0;

  Mfd[fd].State = NewState | (UINT32)OpenMode;

  // return the fd of our now open file
  return fd;
}

/** The rename() function changes the name of a file.
    The old argument points to the pathname of the file to be renamed. The new
    argument points to the new pathname of the file.

    If the old argument points to the pathname of a file that is not a
    directory, the new argument shall not point to the pathname of a
    directory. If the file named by the new argument exists, it shall be
    removed and old renamed to new. Write access permission is required for
    both the directory containing old and the directory containing new.

    If the old argument points to the pathname of a directory, the new
    argument shall not point to the pathname of a file that is not a
    directory. If the directory named by the new argument exists, it shall be
    removed and old renamed to new.

    The new pathname shall not contain a path prefix that names old. Write
    access permission is required for the directory containing old and the
    directory containing new. If the old argument points to the pathname of a
    directory, write access permission may be required for the directory named
    by old, and, if it exists, the directory named by new.

    If the rename() function fails for any reason other than [EIO], any file
    named by new shall be unaffected.

    @return   Upon successful completion, rename() shall return 0; otherwise,
              -1 shall be returned, errno shall be set to indicate the error,
              and neither the file named by old nor the file named by new
              shall be changed or created.
**/
int
rename    (const char *old, const char *new)
{
 // UINT64            InfoSize;
 // RETURN_STATUS     Status;
 // EFI_FILE_INFO     *NewFileInfo = NULL;
 // EFI_FILE_INFO     *OldFileInfo;
 // char              *Newfn;
 // int                OldFd;

 //// Open old file
 // OldFd = open(old, O_RDONLY, 0);
 // if(OldFd >= 0) {
 //   NewFileInfo = malloc(sizeof(EFI_FILE_INFO) + PATH_MAX);
 //   if(NewFileInfo != NULL) {
 //     OldFileInfo = ShellGetFileInfo( FileHandle);
 //     if(OldFileInfo != NULL) {
 //       // Copy the Old file info into our new buffer, and free the old.
 //       memcpy(OldFileInfo, NewFileInfo, sizeof(EFI_FILE_INFO));
 //       FreePool(OldFileInfo);
 //       // Strip off all but the file name portion of new
 //       NewFn = strrchr(new, '/');
 //       if(NewFn == NULL) {
 //         NewFn = strrchr(new '\\');
 //         if(NewFn == NULL) {
 //           NewFn = new;
 //         }
 //       }
 //       // Convert new name from MBCS to WCS
 //       (void)AsciiStrToUnicodeStr( NewFn, gMD->UString);
 //       // Copy the new file name into our new file info buffer
 //       wcsncpy(NewFileInfo->FileName, gMD->UString, wcslen(gMD->UString)+1);
 //       // Apply the new file name
 //       Status = ShellSetFileInfo(FileHandle);
 //       if(Status == EFI_SUCCESS) {
 //         // File has been successfully renamed.  We are DONE!
 //         return 0;
 //       }
 //       errno = EFI2errno( Status );
 //       EFIerrno = Status;
 //     }
 //     else {
 //       errno = EIO;
 //     }
 //   }
 //   else {
 //     errno = ENOMEM;
 //   }
 // }
  return -1;
}

/**
**/
int
rmdir     (const char *path)
{
  EFI_FILE_HANDLE   FileHandle;
  RETURN_STATUS     Status;
  EFI_FILE_INFO     *FileInfo = NULL;
  int               Count = 0;
  BOOLEAN           NoFile = FALSE;

  errno = 0;    // Make it easier to see if we have an error later

  // Convert name from MBCS to WCS
  (void)AsciiStrToUnicodeStr( path, gMD->UString);
  NormalizePath( gMD->UString);

//Print(L"%a( \"%s\")\n", __func__, gMD->UString);
  Status = ShellOpenFileByName( gMD->UString, (SHELL_FILE_HANDLE *)&FileHandle,
                               (EFI_FILE_MODE_READ || EFI_FILE_MODE_WRITE), 0);
  if(Status == RETURN_SUCCESS) {
    FileInfo = ShellGetFileInfo( (SHELL_FILE_HANDLE)FileHandle);
    if(FileInfo != NULL) {
      if((FileInfo->Attribute & EFI_FILE_DIRECTORY) == 0) {
        errno = ENOTDIR;
      }
      else {
        // See if the directory has any entries other than ".." and ".".
        FreePool(FileInfo);  // Free up the buffer from ShellGetFileInfo()
        Status = ShellFindFirstFile( (SHELL_FILE_HANDLE)FileHandle, &FileInfo);
        if(Status == RETURN_SUCCESS) {
          ++Count;
          while(Count < 3) {
            Status = ShellFindNextFile( (SHELL_FILE_HANDLE)FileHandle, FileInfo, &NoFile);
            if(Status == RETURN_SUCCESS) {
              if(NoFile) {
                break;
              }
              ++Count;
            }
            else {
              Count = 99;
            }
          }
          FreePool(FileInfo);   // Free buffer from ShellFindFirstFile()
          if(Count < 3) {
            // Directory is empty
            Status = ShellDeleteFile( (SHELL_FILE_HANDLE *)&FileHandle);
            if(Status == RETURN_SUCCESS) {
              EFIerrno = RETURN_SUCCESS;
              return 0;
              /* ######## SUCCESSFUL RETURN ######## */
            }
          }
          else {
            if(Count == 99) {
              errno = EIO;
            }
            else {
              errno = ENOTEMPTY;
            }
          }
        }
      }
    }
    else {
      errno = EIO;
    }
  }
  EFIerrno = Status;
  if(errno == 0) {
    errno = EFI2errno( Status );
  }
  return -1;
}

/* Internal File Info. worker function for stat and fstat. */
static
EFI_STATUS
_EFI_FileInfo( EFI_FILE_INFO *FileInfo, struct stat *statbuf)
{
  UINT64            Attributes;
  RETURN_STATUS     Status;
  mode_t            newmode;

  if(FileInfo != NULL) {
    // Got the info, now populate statbuf with it
    statbuf->st_blksize   = S_BLKSIZE;
    statbuf->st_size      = FileInfo->Size;
    statbuf->st_physsize  = FileInfo->PhysicalSize;
    statbuf->st_birthtime = Efi2Time( &FileInfo->CreateTime);
    statbuf->st_atime     = Efi2Time( &FileInfo->LastAccessTime);
    statbuf->st_mtime     = Efi2Time( &FileInfo->ModificationTime);
    Attributes = FileInfo->Attribute;
    newmode               = (mode_t)(Attributes << S_EFISHIFT) | S_ACC_READ;
    if((Attributes & EFI_FILE_DIRECTORY) == 0) {
      newmode |= _S_IFREG;
      if((Attributes & EFI_FILE_READ_ONLY) == 0) {
        statbuf->st_mode |= S_ACC_WRITE;
      }
    }
    else {
      newmode |= _S_IFDIR;
    }
    statbuf->st_mode      = newmode;
    Status = RETURN_SUCCESS;
  }
  else {
    Status = RETURN_DEVICE_ERROR;
  }
  return Status;
}

/** The fstat() function obtains information about an open file associated
    with the file descriptor fildes, and shall write it to the area pointed to
    by buf.

    The buf argument is a pointer to a stat structure, as defined
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
      - st_mode     Populated with information from fildes
      - st_ino      Set to zero.  (inode)
      - st_dev      Set to zero.
      - st_uid      Set to zero.
      - st_gid      Set to zero.
      - st_nlink    Set to one.

    @param[in]    fildes    File descriptor as returned from open().
    @param[out]   statbuf   Buffer in which the file status is put.

    @retval    0  Successful Completion.
    @retval   -1  An error has occurred and errno has been set to
                  identify the error.
**/
int
fstat (int fildes, struct stat *statbuf)
{
  EFI_FILE_HANDLE   FileHandle;
  RETURN_STATUS     Status = RETURN_SUCCESS;
  EFI_FILE_INFO     *FileInfo = NULL;
  UINTN             FinfoSize = sizeof(EFI_FILE_INFO);

  if(ValidateFD( fildes, VALID_OPEN)) {
    FileHandle = gMD->fdarray[fildes].FileHandle;
    if(isatty(fildes)) {
      FileInfo = AllocateZeroPool(FinfoSize);
      if(FileInfo != NULL) {
        Status = FileHandle->GetInfo( FileHandle, 0, &FinfoSize, FileInfo);
      }
      else {
        Status = RETURN_OUT_OF_RESOURCES;
      }
    }
    else {
      FileInfo = ShellGetFileInfo( FileHandle);
    }
    Status = _EFI_FileInfo( FileInfo, statbuf);
  }
  errno     = EFI2errno(Status);
  EFIerrno  = Status;

  if(FileInfo != NULL) {
    FreePool(FileInfo);     // Release the buffer allocated by the GetInfo function
  }

  return errno? -1 : 0;
}

/** Obtains information about the file pointed to by path.

    Opens the file pointed to by path, calls _EFI_FileInfo with the file's handle,
    then closes the file.

    @retval    0  Successful Completion.
    @retval   -1  An error has occurred and errno has been set to
                  identify the error.
**/
int
stat   (const char *path, void *statbuf)
{
  EFI_FILE_HANDLE   FileHandle;
  RETURN_STATUS     Status;
  EFI_FILE_INFO     *FileInfo;

  errno = 0;    // Make it easier to see if we have an error later

  // Convert name from MBCS to WCS
  (void)AsciiStrToUnicodeStr( path, gMD->UString);
  NormalizePath( gMD->UString);

  Status = ShellOpenFileByName( gMD->UString, (SHELL_FILE_HANDLE *)&FileHandle, EFI_FILE_MODE_READ, 0ULL);
  if(Status == RETURN_SUCCESS) {
    FileInfo = ShellGetFileInfo( FileHandle);
    Status = _EFI_FileInfo( FileInfo, (struct stat *)statbuf);
    (void)ShellCloseFile( (SHELL_FILE_HANDLE *)&FileHandle);
  }
  errno     = EFI2errno(Status);
  EFIerrno  = Status;

  return errno? -1 : 0;
}

/**  Same as stat since EFI doesn't have symbolic links.  **/
int
lstat (const char *path, struct stat *statbuf)
{
  return stat(path, statbuf);
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
    number of bytes from the file at the file’s current position and returns
    them in buf. If the read goes beyond the end of the file, the read
    length is truncated to the end of the file. The file’s current position is
    increased by the number of bytes returned.

    If fildes refers to a directory, the function reads the directory entry at
    the file’s current position and returns the entry in buf. If buf
    is not large enough to hold the current directory entry, then
    errno is set to EBUFSIZE, EFIerrno is set to EFI_BUFFER_TOO_SMALL, and the
    current file position is not updated. The size of the buffer needed to read
    the entry will be returned as a negative number. On success, the current
    position is updated to the next directory entry. If there are no more
    directory entries, the read returns a zero-length buffer.
    EFI_FILE_INFO is the structure returned as the directory entry.

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
  ssize_t           BufSize;
  EFI_FILE_HANDLE   FileHandle;
  RETURN_STATUS     Status;

  BufSize = (ssize_t)nbyte;
  if(ValidateFD( fildes, VALID_OPEN)) {
    FileHandle = gMD->fdarray[fildes].FileHandle;
    if(isatty(fildes)) {
      Status = FileHandle->Read( FileHandle, (UINTN *)&BufSize, buf);
    }
    else {
      Status = ShellReadFile( FileHandle, (UINTN *)&BufSize, buf);
    }
    if(Status != RETURN_SUCCESS) {
      EFIerrno = Status;
      errno = EFI2errno(Status);
      if(Status == RETURN_BUFFER_TOO_SMALL) {
        BufSize = -BufSize;
      }
      else {
      BufSize = -1;
      }
    }
  }
  else {
    errno = EBADF;
  }
  return BufSize;
}

ssize_t
WideTtyCvt( CHAR16 *dest, const char *buf, size_t n)
{
  UINTN   i;
  wint_t  wc;

  for(i = 0; i < n; ++i) {
    wc = btowc(*buf++);
    if( wc == 0) {
      break;
    };
    if(wc < 0) {
      wc = BLOCKELEMENT_LIGHT_SHADE;
    }
    if(wc == L'\n') {
      *dest++ = L'\r';
    }
    *dest++ = (CHAR16)wc;
  }
  *dest = 0;
  return (ssize_t)i;
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
  characters using the btowc() functions.  The resulting wide character stream
  is what is actually sent to the UEFI console.

  QUESTION:  Should writes to stdout or stderr always succeed?
**/
ssize_t
write  (int fildes, const void *buf, size_t n)
{
  ssize_t           BufSize;
  EFI_FILE_HANDLE   FileHandle;
  RETURN_STATUS     Status = RETURN_SUCCESS;
  ssize_t           UniBufSz;

  BufSize = (ssize_t)n;

  if(ValidateFD( fildes, VALID_OPEN)) {
    FileHandle = gMD->fdarray[fildes].FileHandle;
    if(isatty(fildes)) {
      // Convert string from MBCS to WCS and translate \n to \r\n.
      UniBufSz = WideTtyCvt(gMD->UString, (const char *)buf, n);
      if(UniBufSz > 0) {
        BufSize = (ssize_t)(UniBufSz * sizeof(CHAR16));
        Status = FileHandle->Write( FileHandle, (UINTN *)&BufSize, (void *)gMD->UString);
        BufSize = (ssize_t)n;   // Always pretend all was output
      }
    }
    else {
      Status = ShellWriteFile( FileHandle, (UINTN *)&BufSize, (void *)buf);
    }
    if(Status != RETURN_SUCCESS) {
      EFIerrno = Status;
      errno = EFI2errno(Status);
      if(Status == EFI_UNSUPPORTED) {
        errno = EISDIR;
      }
      BufSize = -1;
    }
  }
  else {
    errno = EBADF;
  }
  return BufSize;
}
