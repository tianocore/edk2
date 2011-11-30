/** @file
  Abstract device driver for the UEFI Shell-hosted environment.

  In a Shell-hosted environment, this is the driver that is called
  when no other driver matches.

  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include  <Uefi.h>
#include  <Library/BaseLib.h>
#include  <Library/MemoryAllocationLib.h>
#include  <Library/UefiBootServicesTableLib.h>
#include  <Library/ShellLib.h>

#include  <LibConfig.h>

#include  <errno.h>
#include  <string.h>
#include  <stdlib.h>
#include  <stdarg.h>
#include  <wctype.h>
#include  <wchar.h>
#include  <sys/fcntl.h>
#include  <sys/filio.h>
#include  <sys/syslimits.h>
#include  <unistd.h>
#include  <kfile.h>
#include  <Device/Device.h>
#include  <MainData.h>
#include  <Efi/SysEfi.h>

/** EFI Shell specific operations for close().

    @param[in]    Fp    Pointer to a file descriptor structure.

    @retval      0      Successful completion.
    @retval     -1      Operation failed.  Further information is specified by errno.
**/
static
int
EFIAPI
da_ShellClose(
  IN      struct __filedes   *Fp
)
{
  EFIerrno = ShellCloseFile( (SHELL_FILE_HANDLE *)&Fp->devdata);
  if(RETURN_ERROR(EFIerrno)) {
    return -1;
  }
  return 0;
}

/** EFI Shell specific operations for deleting a file or directory.

    @param[in]    filp    Pointer to a file descriptor structure.

    @retval      0      Successful completion.
    @retval     -1      Operation failed.  Further information is specified by errno.
**/
static
int
EFIAPI
da_ShellDelete(
  struct __filedes   *filp
  )
{
  RETURN_STATUS         Status;

  Status = ShellDeleteFile( (SHELL_FILE_HANDLE *)&filp->devdata);
  if(Status != RETURN_SUCCESS) {
    errno = EFI2errno(Status);
    EFIerrno = Status;
    return -1;
  }
  return 0;
}

/** EFI Shell specific operations for setting the position within a file.

    @param[in]    filp    Pointer to a file descriptor structure.
    @param[in]    offset  Relative position to move to.
    @param[in]    whence  Specifies the location offset is relative to: Beginning, Current, End.

    @return     Returns the new file position or EOF if the seek failed.
**/
static
off_t
EFIAPI
da_ShellSeek(
  struct __filedes   *filp,
  off_t               offset,
  int                 whence
)
{
  __off_t             CurPos = -1;
  RETURN_STATUS       Status = RETURN_SUCCESS;
  SHELL_FILE_HANDLE   FileHandle;

  FileHandle = (SHELL_FILE_HANDLE)filp->devdata;

  if(whence != SEEK_SET) {
    // We are doing a relative seek
    if(whence == SEEK_END) {
      // seeking relative to EOF, so position there first.
      Status = ShellSetFilePosition( FileHandle, 0xFFFFFFFFFFFFFFFFULL);
    }
    if(Status == RETURN_SUCCESS) {
      // Now, determine our current position.
      Status = ShellGetFilePosition( FileHandle, (UINT64 *)&CurPos);
    }
  }
  else {
    CurPos = 0;   // offset is an absolute position for SEEK_SET
    if(offset < 0) {
      Status = RETURN_INVALID_PARAMETER;
    }
  }
  if(Status == RETURN_SUCCESS) {
    /* CurPos now indicates the point we are seeking from, so seek... */
    Status = ShellSetFilePosition( FileHandle, (UINT64)(CurPos + offset));
    if(Status == RETURN_SUCCESS) {
      // Now, determine our final position.
      Status = ShellGetFilePosition( FileHandle, (UINT64 *)&CurPos);
    }
  }
  if(Status != RETURN_SUCCESS) {
    if(Status == EFI_UNSUPPORTED) {
      errno = EISDIR;
    }
    else {
      errno = EFI2errno(Status);
    }
    EFIerrno = Status;
    CurPos = EOF;
  }
  return CurPos;
}

/** The directory path is created with the access permissions specified by
    perms.

    The directory is closed after it is created.

    @param[in]    path      The directory to be created.
    @param[in]    perms     Access permissions for the new directory.

    @retval   0   The directory was created successfully.
    @retval  -1   An error occurred and an error code is stored in errno.
**/
static
int
EFIAPI
da_ShellMkdir(
  const char   *path,
  __mode_t      perms
  )
{
  UINT64            TempAttr;
  SHELL_FILE_HANDLE FileHandle;
  RETURN_STATUS     Status;
  EFI_FILE_INFO    *FileInfo;
  wchar_t          *NewPath;
  int               retval = -1;

  // Convert name from MBCS to WCS and change '/' to '\\'
  NewPath = NormalizePath( path);

  if(NewPath != NULL) {
    Status = ShellCreateDirectory( NewPath, &FileHandle);
    if(Status == RETURN_SUCCESS) {
      FileInfo = ShellGetFileInfo( FileHandle);
      Status = RETURN_ABORTED;  // In case ShellGetFileInfo() failed
      if(FileInfo != NULL) {
        TempAttr  = FileInfo->Attribute & (EFI_FILE_RESERVED | EFI_FILE_DIRECTORY);
        FileInfo->Attribute = TempAttr | Omode2EFI(perms);
        Status = ShellSetFileInfo( FileHandle, FileInfo);
        FreePool(FileInfo);
        if(Status == RETURN_SUCCESS) {
          (void)ShellCloseFile(&FileHandle);
          retval = 0;
        }
      }
    }
    errno = EFI2errno(Status);
    EFIerrno = Status;
    free(NewPath);
  }
  return retval;
}

/** EFI Shell specific operations for reading from a file.

    @param[in]    filp        Pointer to a file descriptor structure.
    @param[in]    offset      Offset into the file to begin reading at, or NULL.
    @param[in]    BufferSize  Number of bytes in Buffer.  Max number of bytes to read.
    @param[in]    Buffer      Pointer to a buffer to receive the read data.

    @return     Returns the number of bytes successfully read,
                or -1 if the operation failed.  Further information is specified by errno.
**/
static
ssize_t
EFIAPI
da_ShellRead(
  IN OUT  struct __filedes   *filp,
  IN OUT  off_t              *offset,
  IN      size_t              BufferSize,
     OUT  VOID               *Buffer
)
{
  ssize_t           BufSize;
  SHELL_FILE_HANDLE FileHandle;
  RETURN_STATUS     Status;

  if(offset != NULL) {
    BufSize = (ssize_t)da_ShellSeek(filp, *offset, SEEK_SET);
    if(BufSize >= 0) {
      filp->f_offset = BufSize;
    }
  }

  BufSize = (ssize_t)BufferSize;
  FileHandle = (SHELL_FILE_HANDLE)filp->devdata;

  Status = ShellReadFile( FileHandle, (UINTN *)&BufSize, Buffer);
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
  else {
    filp->f_offset += BufSize;  // Advance to where we want to read next.
  }
  return BufSize;
}

/** EFI Shell specific operations for writing to a file.

    @param[in]    filp        Pointer to a file descriptor structure.
    @param[in]    offset      Offset into the file to begin writing at, or NULL.
    @param[in]    BufferSize  Number of bytes in Buffer.  Max number of bytes to write.
    @param[in]    Buffer      Pointer to a buffer containing the data to be written.

    @return     Returns the number of bytes successfully written,
                or -1 if the operation failed.  Further information is specified by errno.
**/
static
ssize_t
EFIAPI
da_ShellWrite(
  IN  struct __filedes     *filp,
  IN  off_t                *offset,
  IN  size_t                BufferSize,
  IN  const void           *Buffer
  )
{
  ssize_t           BufSize;
  SHELL_FILE_HANDLE FileHandle;
  RETURN_STATUS     Status;
  off_t             Position  = 0;
  int               How       = SEEK_SET;


  if((offset != NULL) || (filp->Oflags & O_APPEND)) {
    if(filp->Oflags & O_APPEND) {
      Position  = 0;
      How       = SEEK_END;
    }
    else {
      Position  = *offset;
      How       = SEEK_SET;
    }
    BufSize = (ssize_t)da_ShellSeek(filp, Position, How);
    if(BufSize >= 0) {
      filp->f_offset = BufSize;
    }
  }

  BufSize = (ssize_t)BufferSize;
  FileHandle = (SHELL_FILE_HANDLE)filp->devdata;

  Status = ShellWriteFile( FileHandle, (UINTN *)&BufSize, (void *)Buffer);

  if(Status != RETURN_SUCCESS) {
    EFIerrno = Status;
    errno = EFI2errno(Status);
    if(Status == EFI_UNSUPPORTED) {
      errno = EISDIR;
    }
    BufSize = -1;
  }
  else {
    filp->f_offset += BufSize;  // Advance to where we want to write next.
  }

  return BufSize;
}

/** EFI Shell specific operations for getting information about an open file.

    @param[in]    filp        Pointer to a file descriptor structure.
    @param[out]   statbuf     Buffer in which to store the file status.
    @param[in]    Something   This parameter is not used by this device.

    @retval      0      Successful completion.
    @retval     -1      Operation failed.  Further information is specified by errno.
**/
static
int
EFIAPI
da_ShellStat(
  struct __filedes   *filp,
  struct stat        *statbuf,
  void               *Something
  )
{
  SHELL_FILE_HANDLE FileHandle;
  EFI_FILE_INFO    *FileInfo      = NULL;
  UINT64            Attributes;
  RETURN_STATUS     Status;
  mode_t            newmode;

  FileHandle = (SHELL_FILE_HANDLE)filp->devdata;

  FileInfo = ShellGetFileInfo( FileHandle);

  if(FileInfo != NULL) {
    // Got the info, now populate statbuf with it
    statbuf->st_blksize   = S_BLKSIZE;
    statbuf->st_size      = FileInfo->FileSize;
    statbuf->st_physsize  = FileInfo->PhysicalSize;
    statbuf->st_birthtime = Efi2Time( &FileInfo->CreateTime);
    statbuf->st_atime     = Efi2Time( &FileInfo->LastAccessTime);
    statbuf->st_mtime     = Efi2Time( &FileInfo->ModificationTime);
    Attributes = FileInfo->Attribute;
    newmode               = (mode_t)(Attributes << S_EFISHIFT) | S_ACC_READ;
    if((Attributes & EFI_FILE_DIRECTORY) == 0) {
      newmode |= _S_IFREG;
      if((Attributes & EFI_FILE_READ_ONLY) == 0) {
        newmode |= S_ACC_WRITE;
      }
    }
    else {
      newmode |= _S_IFDIR;
    }
    statbuf->st_mode = newmode;
    Status = RETURN_SUCCESS;
  }
  else {
    Status = RETURN_DEVICE_ERROR;
    errno  = EIO;
  }
  EFIerrno  = Status;

  if(FileInfo != NULL) {
    FreePool(FileInfo);     // Release the buffer allocated by the GetInfo function
  }
  return (Status == RETURN_SUCCESS)? 0 : -1;
}

/** EFI Shell specific operations for low-level control of a file or device.

    @param[in]      filp    Pointer to a file descriptor structure.
    @param[in]      cmd     The command this ioctl is to perform.
    @param[in,out]  argp    Zero or more arguments as needed by the command.

    @retval      0      Successful completion.
    @retval     -1      Operation failed.  Further information is specified by errno.
**/
static
int
EFIAPI
da_ShellIoctl(
  struct __filedes   *filp,
  ULONGN              cmd,
  va_list             argp
  )
{
  EFI_FILE_INFO    *FileInfo      = NULL;
  SHELL_FILE_HANDLE FileHandle;
  RETURN_STATUS     Status        = RETURN_SUCCESS;
  int               retval        = 0;

  FileHandle = (SHELL_FILE_HANDLE)filp->devdata;

  FileInfo = ShellGetFileInfo( FileHandle);

  if(FileInfo != NULL) {
    if( cmd == (ULONGN)FIOSETIME) {
      struct timeval  *TV;
      EFI_TIME        *ET;
      int              mod = 0;

      TV = va_arg(argp, struct timeval*);
      if(TV[0].tv_sec != 0) {
        ET = Time2Efi(TV[0].tv_sec);
        if(ET != NULL) {
          (void) memcpy(&FileInfo->LastAccessTime, ET, sizeof(EFI_TIME));
          FileInfo->LastAccessTime.Nanosecond = TV[0].tv_usec * 1000;
          free(ET);
          ++mod;
        }
      }
      if(TV[1].tv_sec != 0) {
        ET = Time2Efi(TV[1].tv_sec);
        if(ET != NULL) {
          (void) memcpy(&FileInfo->ModificationTime, ET, sizeof(EFI_TIME));
          FileInfo->ModificationTime.Nanosecond = TV[1].tv_usec * 1000;
          free(ET);
          ++mod;
        }
      }
      /* Set access and modification times */
      Status = ShellSetFileInfo(FileHandle, FileInfo);
      errno = EFI2errno(Status);
    }
  }
  else {
    Status = RETURN_DEVICE_ERROR;
    errno  = EIO;
  }
  if(RETURN_ERROR(Status)) {
    retval = -1;
  }
  EFIerrno  = Status;

  if(FileInfo != NULL) {
    FreePool(FileInfo);     // Release the buffer allocated by the GetInfo function
  }
  return retval;
}

/** EFI Shell specific operations for opening a file or directory.

    @param[in]    DevNode   Pointer to a device descriptor
    @param[in]    filp      Pointer to a file descriptor structure.
    @param[in]    DevInstance   Not used by this device.
    @param[in]    Path          File-system path to the file or directory.
    @param[in]    MPath         Device or Map name on which Path resides.

    @return     Returns a file descriptor for the newly opened file,
                or -1 if the Operation failed.  Further information is specified by errno.
**/
int
EFIAPI
da_ShellOpen(
  DeviceNode         *DevNode,
  struct __filedes   *filp,
  int                 DevInstance,    /* Not used by Shell */
  wchar_t            *Path,
  wchar_t            *MPath
  )
{
  UINT64                OpenMode;
  UINT64                Attributes;
  SHELL_FILE_HANDLE     FileHandle;
  GenericInstance      *Gip;
  char                 *NPath;
  wchar_t              *WPath;
  RETURN_STATUS         Status;
  int                   oflags;
  int                   retval;

  EFIerrno = RETURN_SUCCESS;

  //Attributes = Omode2EFI(mode);
  Attributes = 0;

  // Convert oflags to Attributes
  oflags = filp->Oflags;
  OpenMode = Oflags2EFI(oflags);
  if(OpenMode == 0) {
    errno = EINVAL;
    return -1;
  }

  /* Re-create the full mapped path for the shell. */
  if(MPath != NULL) {
    WPath = AllocateZeroPool(PATH_MAX * sizeof(wchar_t) + 1);
    if(WPath == NULL) {
      errno = ENOMEM;
      EFIerrno = RETURN_OUT_OF_RESOURCES;
      return -1;
    }
    wcsncpy(WPath, MPath, NAME_MAX);                /* Get the Map Name */
    wcsncat(WPath, Path, (PATH_MAX - NAME_MAX));    /* Append the path */
  }
  else {
    WPath = Path;
  }

  retval = -1;    /* Initially assume failure.  */

  /* Do we care if the file already exists?
     If O_TRUNC, then delete the file.  It will be created anew subsequently.
     If O_EXCL, then error if the file exists and O_CREAT is set.

  !!!!!!!!! Change this to use ShellSetFileInfo() to actually truncate the file
  !!!!!!!!! instead of deleting and re-creating it.
  */
  do {  /* Do fake exception handling */
  if((oflags & O_TRUNC) || ((oflags & (O_EXCL | O_CREAT)) == (O_EXCL | O_CREAT))) {
      Status = ShellIsFile( WPath );
    if(Status == RETURN_SUCCESS) {
      // The file exists
      if(oflags & O_TRUNC) {
          NPath = AllocateZeroPool(PATH_MAX);
        if(NPath == NULL) {
          errno = ENOMEM;
          EFIerrno = RETURN_OUT_OF_RESOURCES;
            break;
        }
          wcstombs(NPath, WPath, PATH_MAX);
        // We do a truncate by deleting the existing file and creating a new one.
        if(unlink(NPath) != 0) {
          filp->f_iflags = 0;    // Release our reservation on this FD
          FreePool(NPath);
            break;
        }
        FreePool(NPath);
      }
      else if((oflags & (O_EXCL | O_CREAT)) == (O_EXCL | O_CREAT)) {
        errno = EEXIST;
        EFIerrno = RETURN_ACCESS_DENIED;
        filp->f_iflags = 0;    // Release our reservation on this FD
          break;
      }
    }
  }

  // Call the EFI Shell's Open function
    Status = ShellOpenFileByName( WPath, &FileHandle, OpenMode, Attributes);
  if(RETURN_ERROR(Status)) {
    filp->f_iflags = 0;    // Release our reservation on this FD
    // Set errno based upon Status
    errno = EFI2errno(Status);
    EFIerrno = Status;
      break;
  }
    retval = 0;
  // Successfully got a regular File
  filp->f_iflags |= S_IFREG;

  // Update the info in the fd
  filp->devdata = (void *)FileHandle;

    Gip = (GenericInstance *)DevNode->InstanceList;
  filp->f_offset = 0;
  filp->f_ops = &Gip->Abstraction;
  //  filp->devdata = FileHandle;
  } while(FALSE);

  /* If we get this far, WPath is not NULL.
     If MPath is not NULL, then WPath was allocated so we need to free it.
  */
  if(MPath != NULL) {
    FreePool(WPath);
  }
  return retval;
}

#include  <sys/poll.h>
/** Returns a bit mask describing which operations could be completed immediately.

    For now, assume the file system, via the shell, is always ready.

    (POLLIN | POLLRDNORM)   The file system is ready to be read.
    (POLLOUT)               The file system is ready for output.

    @param[in]    filp    Pointer to a file descriptor structure.
    @param[in]    events  Bit mask describing which operations to check.

    @return     The returned value is a bit mask describing which operations
                could be completed immediately, without blocking.
**/
static
short
EFIAPI
da_ShellPoll(
  struct __filedes   *filp,
  short               events
  )
{
  UINT32      RdyMask;
  short       retval = 0;

  RdyMask = (UINT32)filp->Oflags;

  switch(RdyMask & O_ACCMODE) {
    case O_RDONLY:
      retval = (POLLIN | POLLRDNORM);
      break;

    case O_WRONLY:
      retval = POLLOUT;
      break;

    case O_RDWR:
      retval = (POLLIN | POLLRDNORM | POLLOUT);
      break;

    default:
      retval = POLLERR;
      break;
  }
  return (retval & (events | POLL_RETONLY));
}

/** EFI Shell specific operations for renaming a file.

    @param[in]    from    Name of the file to be renamed.
    @param[in]    to      New name for the file.

    @retval      0      Successful completion.
    @retval     -1      Operation failed.  Further information is specified by errno.
**/
static
int
EFIAPI
da_ShellRename(
  const char   *from,
  const char   *to
  )
{
  RETURN_STATUS       Status;
  EFI_FILE_INFO      *NewFileInfo;
  EFI_FILE_INFO      *OldFileInfo;
  wchar_t            *NewFn;
  int                 OldFd;
  SHELL_FILE_HANDLE   FileHandle;
  wchar_t            *NormalizedPath;

  // Open old file
  OldFd = open(from, O_RDWR, 0);
  if(OldFd >= 0) {
    FileHandle = (SHELL_FILE_HANDLE)gMD->fdarray[OldFd].devdata;

    NewFileInfo = malloc(sizeof(EFI_FILE_INFO) + PATH_MAX);
    if(NewFileInfo != NULL) {
      OldFileInfo = ShellGetFileInfo( FileHandle);
      if(OldFileInfo != NULL) {
        // Copy the Old file info into our new buffer, and free the old.
        memcpy(NewFileInfo, OldFileInfo, sizeof(EFI_FILE_INFO));
        FreePool(OldFileInfo);
        // Normalize path and convert to WCS.
        NormalizedPath = NormalizePath(to);
        if (NormalizedPath != NULL) {
        // Strip off all but the file name portion of new
          NewFn = GetFileNameFromPath(NormalizedPath);
        // Copy the new file name into our new file info buffer
          wcsncpy(NewFileInfo->FileName, NewFn, wcslen(NewFn) + 1);
          // Update the size of the structure.
          NewFileInfo->Size = sizeof(EFI_FILE_INFO) + StrSize(NewFn);
        // Apply the new file name
        Status = ShellSetFileInfo(FileHandle, NewFileInfo);
          free(NormalizedPath);
        free(NewFileInfo);
        if(Status == EFI_SUCCESS) {
          // File has been successfully renamed.  We are DONE!
          return 0;
        }
        errno = EFI2errno( Status );
        EFIerrno = Status;
      }
      else {
          free(NewFileInfo);
          errno = ENOMEM;
        }
      }
      else {
        free(NewFileInfo);
        errno = EIO;
      }
    }
    else {
      errno = ENOMEM;
    }
  }
  return -1;
}

/** EFI Shell specific operations for deleting directories.

    @param[in]    filp    Pointer to a file descriptor structure.

    @retval      0      Successful completion.
    @retval     -1      Operation failed.  Further information is specified by errno.
**/
static
int
EFIAPI
da_ShellRmdir(
  struct __filedes   *filp
  )
{
  SHELL_FILE_HANDLE FileHandle;
  RETURN_STATUS     Status = RETURN_SUCCESS;
  EFI_FILE_INFO     *FileInfo;
  int               OldErrno;
  int               Count = 0;
  BOOLEAN           NoFile = FALSE;

  OldErrno  = errno;  // Save the original value
  errno = 0;    // Make it easier to see if we have an error later

  FileHandle = (SHELL_FILE_HANDLE)filp->devdata;

  FileInfo = ShellGetFileInfo(FileHandle);
  if(FileInfo != NULL) {
    if((FileInfo->Attribute & EFI_FILE_DIRECTORY) == 0) {
      errno = ENOTDIR;
    }
    else {
      FreePool(FileInfo);  // Free up the buffer from ShellGetFileInfo()
      // See if the directory has any entries other than ".." and ".".
      Status = ShellFindFirstFile( FileHandle, &FileInfo);
      if(Status == RETURN_SUCCESS) {
        ++Count;
        while(Count < 3) {
          Status = ShellFindNextFile( FileHandle, FileInfo, &NoFile);
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
        /*  Count == 99 and FileInfo is allocated if ShellFindNextFile failed.
            ShellFindNextFile has freed FileInfo itself if it sets NoFile TRUE.
        */
        if((! NoFile) || (Count == 99)) {
          free(FileInfo);   // Free buffer from ShellFindFirstFile()
        }
        if(Count < 3) {
          // Directory is empty
          Status = ShellDeleteFile( &FileHandle);
          if(Status == RETURN_SUCCESS) {
            EFIerrno = RETURN_SUCCESS;
            errno    = OldErrno;    // Restore the original value
            return 0;
            /* ######## SUCCESSFUL RETURN ######## */
          }
          /*  FileInfo is freed and FileHandle closed. */
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
  ShellCloseFile( &FileHandle);
  EFIerrno = Status;
  if(errno == 0) {
    errno = EFI2errno( Status );
  }
  return -1;
}

/** Construct an instance of the abstract Shell device.

    Allocate the instance structure and populate it with the information for
    the device.

    @param[in]    ImageHandle   This application's image handle.
    @param[in]    SystemTable   Pointer to the UEFI System Table.

    @retval     RETURN_SUCCESS            Successful completion.
    @retval     RETURN_OUT_OF_RESOURCES   Failed to allocate memory for new device.
    @retval     RETURN_INVALID_PARAMETER  A default device has already been created.
**/
RETURN_STATUS
EFIAPI
__ctor_DevShell(
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
)
{
  GenericInstance    *Stream;
  DeviceNode     *Node;
  RETURN_STATUS   Status;

  Stream = (GenericInstance *)AllocateZeroPool(sizeof(GenericInstance));
  if(Stream == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }

  Stream->Cookie      = CON_COOKIE;
  Stream->InstanceNum = 1;
  Stream->Dev = NULL;
  Stream->Abstraction.fo_close    = &da_ShellClose;
  Stream->Abstraction.fo_read     = &da_ShellRead;
  Stream->Abstraction.fo_write    = &da_ShellWrite;
  Stream->Abstraction.fo_fcntl    = &fnullop_fcntl;
  Stream->Abstraction.fo_poll     = &da_ShellPoll;
  Stream->Abstraction.fo_flush    = &fnullop_flush;
  Stream->Abstraction.fo_stat     = &da_ShellStat;
  Stream->Abstraction.fo_ioctl    = &da_ShellIoctl;
  Stream->Abstraction.fo_delete   = &da_ShellDelete;
  Stream->Abstraction.fo_rmdir    = &da_ShellRmdir;
  Stream->Abstraction.fo_mkdir    = &da_ShellMkdir;
  Stream->Abstraction.fo_rename   = &da_ShellRename;
  Stream->Abstraction.fo_lseek    = &da_ShellSeek;

  Node = __DevRegister(NULL, NULL, &da_ShellOpen, Stream, 1, sizeof(GenericInstance), O_RDWR);
  Status = EFIerrno;
  Stream->Parent = Node;

  return  Status;
}

/** Destructor for previously constructed EFI Shell device instances.

    @param[in]    ImageHandle   This application's image handle.
    @param[in]    SystemTable   Pointer to the UEFI System Table.

    @retval      0      Successful completion is always returned.
**/
RETURN_STATUS
EFIAPI
__dtor_DevShell(
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
)
{
  if(daDefaultDevice != NULL) {
    if(daDefaultDevice->InstanceList != NULL) {
      FreePool(daDefaultDevice->InstanceList);
    }
    FreePool(daDefaultDevice);
  }
  return RETURN_SUCCESS;
}
