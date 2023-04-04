/** @file
  Support a Semi Host file system over a debuggers JTAG

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Portions copyright (c) 2011 - 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Guid/FileInfo.h>
#include <Guid/FileSystemInfo.h>
#include <Guid/FileSystemVolumeLabelInfo.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SemihostLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Protocol/DevicePath.h>
#include <Protocol/SimpleFileSystem.h>

#include "SemihostFs.h"

#define DEFAULT_SEMIHOST_FS_LABEL  L"SemihostFs"

STATIC CHAR16  *mSemihostFsLabel;

EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  gSemihostFs = {
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION,
  VolumeOpen
};

EFI_FILE  gSemihostFsFile = {
  EFI_FILE_PROTOCOL_REVISION,
  FileOpen,
  FileClose,
  FileDelete,
  FileRead,
  FileWrite,
  FileGetPosition,
  FileSetPosition,
  FileGetInfo,
  FileSetInfo,
  FileFlush
};

//
// Device path for semi-hosting. It contains our auto-generated Caller ID GUID.
//
typedef struct {
  VENDOR_DEVICE_PATH          Guid;
  EFI_DEVICE_PATH_PROTOCOL    End;
} SEMIHOST_DEVICE_PATH;

SEMIHOST_DEVICE_PATH  gDevicePath = {
  {
    { HARDWARE_DEVICE_PATH, HW_VENDOR_DP,                   { sizeof (VENDOR_DEVICE_PATH),       0 }
    },
    EFI_CALLER_ID_GUID
  },
  { END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, { sizeof (EFI_DEVICE_PATH_PROTOCOL), 0 }
  }
};

typedef struct {
  LIST_ENTRY       Link;
  UINT64           Signature;
  EFI_FILE         File;
  CHAR8            *FileName;
  UINT64           OpenMode;
  UINT32           Position;
  UINTN            SemihostHandle;
  BOOLEAN          IsRoot;
  EFI_FILE_INFO    Info;
} SEMIHOST_FCB;

#define SEMIHOST_FCB_SIGNATURE  SIGNATURE_32( 'S', 'H', 'F', 'C' )
#define SEMIHOST_FCB_FROM_THIS(a)  CR(a, SEMIHOST_FCB, File, SEMIHOST_FCB_SIGNATURE)
#define SEMIHOST_FCB_FROM_LINK(a)  CR(a, SEMIHOST_FCB, Link, SEMIHOST_FCB_SIGNATURE);

EFI_HANDLE  gInstallHandle = NULL;
LIST_ENTRY  gFileList      = INITIALIZE_LIST_HEAD_VARIABLE (gFileList);

SEMIHOST_FCB *
AllocateFCB (
  VOID
  )
{
  SEMIHOST_FCB  *Fcb;

  Fcb = AllocateZeroPool (sizeof (SEMIHOST_FCB));
  if (Fcb != NULL) {
    CopyMem (&Fcb->File, &gSemihostFsFile, sizeof (gSemihostFsFile));
    Fcb->Signature = SEMIHOST_FCB_SIGNATURE;
  }

  return Fcb;
}

VOID
FreeFCB (
  IN SEMIHOST_FCB  *Fcb
  )
{
  // Remove Fcb from gFileList.
  RemoveEntryList (&Fcb->Link);

  // To help debugging...
  Fcb->Signature = 0;

  FreePool (Fcb);
}

EFI_STATUS
VolumeOpen (
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *This,
  OUT EFI_FILE                         **Root
  )
{
  SEMIHOST_FCB  *RootFcb;

  if (Root == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  RootFcb = AllocateFCB ();
  if (RootFcb == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  RootFcb->IsRoot         = TRUE;
  RootFcb->Info.Attribute = EFI_FILE_READ_ONLY | EFI_FILE_DIRECTORY;

  InsertTailList (&gFileList, &RootFcb->Link);

  *Root = &RootFcb->File;

  return EFI_SUCCESS;
}

/**
  Open a file on the host system by means of the semihosting interface.

  @param[in]   This        A pointer to the EFI_FILE_PROTOCOL instance that is
                           the file handle to source location.
  @param[out]  NewHandle   A pointer to the location to return the opened
                           handle for the new file.
  @param[in]   FileName    The Null-terminated string of the name of the file
                           to be opened.
  @param[in]   OpenMode    The mode to open the file : Read or Read/Write or
                           Read/Write/Create
  @param[in]   Attributes  Only valid for EFI_FILE_MODE_CREATE, in which case these
                           are the attribute bits for the newly created file. The
                           mnemonics of the attribute bits are : EFI_FILE_READ_ONLY,
                           EFI_FILE_HIDDEN, EFI_FILE_SYSTEM, EFI_FILE_RESERVED,
                           EFI_FILE_DIRECTORY and EFI_FILE_ARCHIVE.

  @retval  EFI_SUCCESS            The file was open.
  @retval  EFI_NOT_FOUND          The specified file could not be found.
  @retval  EFI_DEVICE_ERROR       The last issued semi-hosting operation failed.
  @retval  EFI_WRITE_PROTECTED    Attempt to create a directory. This is not possible
                                  with the semi-hosting interface.
  @retval  EFI_OUT_OF_RESOURCES   Not enough resources were available to open the file.
  @retval  EFI_INVALID_PARAMETER  At least one of the parameters is invalid.

**/
EFI_STATUS
FileOpen (
  IN  EFI_FILE  *This,
  OUT EFI_FILE  **NewHandle,
  IN  CHAR16    *FileName,
  IN  UINT64    OpenMode,
  IN  UINT64    Attributes
  )
{
  SEMIHOST_FCB   *FileFcb;
  RETURN_STATUS  Return;
  EFI_STATUS     Status;
  UINTN          SemihostHandle;
  CHAR8          *AsciiFileName;
  UINT32         SemihostMode;
  UINTN          Length;

  if ((FileName == NULL) || (NewHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((OpenMode != EFI_FILE_MODE_READ) &&
      (OpenMode != (EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE)) &&
      (OpenMode != (EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE)))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (((OpenMode & EFI_FILE_MODE_CREATE) != 0) &&
      ((Attributes & EFI_FILE_DIRECTORY) != 0))
  {
    return EFI_WRITE_PROTECTED;
  }

  Length        = StrLen (FileName) + 1;
  AsciiFileName = AllocatePool (Length);
  if (AsciiFileName == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  UnicodeStrToAsciiStrS (FileName, AsciiFileName, Length);

  // Opening '/', '\', '.', or the NULL pathname is trying to open the root directory
  if ((AsciiStrCmp (AsciiFileName, "\\") == 0) ||
      (AsciiStrCmp (AsciiFileName, "/")  == 0) ||
      (AsciiStrCmp (AsciiFileName, "")   == 0) ||
      (AsciiStrCmp (AsciiFileName, ".")  == 0))
  {
    FreePool (AsciiFileName);
    return (VolumeOpen (&gSemihostFs, NewHandle));
  }

  //
  // No control is done here concerning the file path. It is passed
  // as it is to the host operating system through the semi-hosting
  // interface. We first try to open the file in the read or update
  // mode even if the file creation has been asked for. That way, if
  // the file already exists, it is not truncated to zero length. In
  // write mode (bit SEMIHOST_FILE_MODE_WRITE up), if the file already
  // exists, it is reset to an empty file.
  //
  if (OpenMode == EFI_FILE_MODE_READ) {
    SemihostMode = SEMIHOST_FILE_MODE_READ | SEMIHOST_FILE_MODE_BINARY;
  } else {
    SemihostMode = SEMIHOST_FILE_MODE_READ | SEMIHOST_FILE_MODE_BINARY | SEMIHOST_FILE_MODE_UPDATE;
  }

  Return = SemihostFileOpen (AsciiFileName, SemihostMode, &SemihostHandle);

  if (RETURN_ERROR (Return)) {
    if ((OpenMode & EFI_FILE_MODE_CREATE) != 0) {
      //
      // In the create if does not exist case, if the opening in update
      // mode failed, create it and open it in update mode. The update
      // mode allows for both read and write from and to the file.
      //
      Return = SemihostFileOpen (
                 AsciiFileName,
                 SEMIHOST_FILE_MODE_WRITE | SEMIHOST_FILE_MODE_BINARY | SEMIHOST_FILE_MODE_UPDATE,
                 &SemihostHandle
                 );
      if (RETURN_ERROR (Return)) {
        Status = EFI_DEVICE_ERROR;
        goto Error;
      }
    } else {
      Status = EFI_NOT_FOUND;
      goto Error;
    }
  }

  // Allocate a control block and fill it
  FileFcb = AllocateFCB ();
  if (FileFcb == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }

  FileFcb->FileName       = AsciiFileName;
  FileFcb->SemihostHandle = SemihostHandle;
  FileFcb->Position       = 0;
  FileFcb->IsRoot         = 0;
  FileFcb->OpenMode       = OpenMode;

  Return = SemihostFileLength (SemihostHandle, &Length);
  if (RETURN_ERROR (Return)) {
    Status = EFI_DEVICE_ERROR;
    FreeFCB (FileFcb);
    goto Error;
  }

  FileFcb->Info.FileSize     = Length;
  FileFcb->Info.PhysicalSize = Length;
  FileFcb->Info.Attribute    = ((OpenMode & EFI_FILE_MODE_CREATE) != 0) ?
                               Attributes : 0;

  InsertTailList (&gFileList, &FileFcb->Link);

  *NewHandle = &FileFcb->File;

  return EFI_SUCCESS;

Error:

  FreePool (AsciiFileName);

  return Status;
}

/**
  Worker function that truncate a file specified by its name to a given size.

  @param[in]  FileName  The Null-terminated string of the name of the file to be opened.
  @param[in]  Size      The target size for the file.

  @retval  EFI_SUCCESS       The file was truncated.
  @retval  EFI_DEVICE_ERROR  The last issued semi-hosting operation failed.

**/
STATIC
EFI_STATUS
TruncateFile (
  IN CHAR8  *FileName,
  IN UINTN  Size
  )
{
  EFI_STATUS     Status;
  RETURN_STATUS  Return;
  UINTN          FileHandle;
  UINT8          *Buffer;
  UINTN          Remaining;
  UINTN          Read;
  UINTN          ToRead;

  Status     = EFI_DEVICE_ERROR;
  FileHandle = 0;
  Buffer     = NULL;

  Return = SemihostFileOpen (
             FileName,
             SEMIHOST_FILE_MODE_READ | SEMIHOST_FILE_MODE_BINARY,
             &FileHandle
             );
  if (RETURN_ERROR (Return)) {
    goto Error;
  }

  Buffer = AllocatePool (Size);
  if (Buffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }

  Read      = 0;
  Remaining = Size;
  while (Remaining > 0) {
    ToRead = Remaining;
    Return = SemihostFileRead (FileHandle, &ToRead, Buffer + Read);
    if (RETURN_ERROR (Return)) {
      goto Error;
    }

    Remaining -= ToRead;
    Read      += ToRead;
  }

  Return     = SemihostFileClose (FileHandle);
  FileHandle = 0;
  if (RETURN_ERROR (Return)) {
    goto Error;
  }

  Return = SemihostFileOpen (
             FileName,
             SEMIHOST_FILE_MODE_WRITE | SEMIHOST_FILE_MODE_BINARY,
             &FileHandle
             );
  if (RETURN_ERROR (Return)) {
    goto Error;
  }

  if (Size > 0) {
    Return = SemihostFileWrite (FileHandle, &Size, Buffer);
    if (RETURN_ERROR (Return)) {
      goto Error;
    }
  }

  Status = EFI_SUCCESS;

Error:

  if (FileHandle != 0) {
    SemihostFileClose (FileHandle);
  }

  if (Buffer != NULL) {
    FreePool (Buffer);
  }

  return (Status);
}

/**
  Close a specified file handle.

  @param[in]  This  A pointer to the EFI_FILE_PROTOCOL instance that is the file
                    handle to close.

  @retval  EFI_SUCCESS            The file was closed.
  @retval  EFI_INVALID_PARAMETER  The parameter "This" is NULL.

**/
EFI_STATUS
FileClose (
  IN EFI_FILE  *This
  )
{
  SEMIHOST_FCB  *Fcb;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Fcb = SEMIHOST_FCB_FROM_THIS (This);

  if (!Fcb->IsRoot) {
    SemihostFileClose (Fcb->SemihostHandle);
    //
    // The file size might have been reduced from its actual
    // size on the host file system with FileSetInfo(). In
    // that case, the file has to be truncated.
    //
    if (Fcb->Info.FileSize < Fcb->Info.PhysicalSize) {
      TruncateFile (Fcb->FileName, Fcb->Info.FileSize);
    }

    FreePool (Fcb->FileName);
  }

  FreeFCB (Fcb);

  return EFI_SUCCESS;
}

/**
  Close and delete a file.

  @param[in]  This  A pointer to the EFI_FILE_PROTOCOL instance that is the file
                    handle to delete.

  @retval  EFI_SUCCESS              The file was closed and deleted.
  @retval  EFI_WARN_DELETE_FAILURE  The handle was closed, but the file was not deleted.
  @retval  EFI_INVALID_PARAMETER    The parameter "This" is NULL.

**/
EFI_STATUS
FileDelete (
  IN EFI_FILE  *This
  )
{
  SEMIHOST_FCB   *Fcb;
  RETURN_STATUS  Return;
  CHAR8          *FileName;
  UINTN          NameSize;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Fcb = SEMIHOST_FCB_FROM_THIS (This);

  if (!Fcb->IsRoot) {
    // Get the filename from the Fcb
    NameSize = AsciiStrLen (Fcb->FileName);
    FileName = AllocatePool (NameSize + 1);

    AsciiStrCpyS (FileName, NameSize + 1, Fcb->FileName);

    // Close the file if it's open.  Disregard return status,
    // since it might give an error if the file isn't open.
    This->Close (This);

    // Call the semihost interface to delete the file.
    Return = SemihostFileRemove (FileName);
    if (RETURN_ERROR (Return)) {
      return EFI_WARN_DELETE_FAILURE;
    }

    return EFI_SUCCESS;
  } else {
    return EFI_WARN_DELETE_FAILURE;
  }
}

/**
  Read data from an open file.

  @param[in]      This        A pointer to the EFI_FILE_PROTOCOL instance that
                              is the file handle to read data from.
  @param[in out]  BufferSize  On input, the size of the Buffer. On output, the
                              amount of data returned in Buffer. In both cases,
                              the size is measured in bytes.
  @param[out]     Buffer      The buffer into which the data is read.

  @retval  EFI_SUCCESS            The data was read.
  @retval  EFI_DEVICE_ERROR       On entry, the current file position is
                                  beyond the end of the file, or the semi-hosting
                                  interface reported an error while performing the
                                  read operation.
  @retval  EFI_INVALID_PARAMETER  At least one of the three input pointers is NULL.

**/
EFI_STATUS
FileRead (
  IN     EFI_FILE  *This,
  IN OUT UINTN     *BufferSize,
  OUT    VOID      *Buffer
  )
{
  SEMIHOST_FCB   *Fcb;
  EFI_STATUS     Status;
  RETURN_STATUS  Return;

  if ((This == NULL) || (BufferSize == NULL) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Fcb = SEMIHOST_FCB_FROM_THIS (This);

  if (Fcb->IsRoot) {
    // The semi-hosting interface does not allow to list files on the host machine.
    Status = EFI_UNSUPPORTED;
  } else {
    Status = EFI_SUCCESS;
    if (Fcb->Position >= Fcb->Info.FileSize) {
      *BufferSize = 0;
      if (Fcb->Position > Fcb->Info.FileSize) {
        Status = EFI_DEVICE_ERROR;
      }
    } else {
      Return = SemihostFileRead (Fcb->SemihostHandle, BufferSize, Buffer);
      if (RETURN_ERROR (Return)) {
        Status = EFI_DEVICE_ERROR;
      } else {
        Fcb->Position += *BufferSize;
      }
    }
  }

  return Status;
}

/**
  Worker function that extends the size of an open file.

  The extension is filled with zeros.

  @param[in]  Fcb   Internal description of the opened file
  @param[in]  Size  The number of bytes, the file has to be extended.

  @retval  EFI_SUCCESS       The file was extended.
  @retval  EFI_DEVICE_ERROR  The last issued semi-hosting operation failed.

**/
STATIC
EFI_STATUS
ExtendFile (
  IN  SEMIHOST_FCB  *Fcb,
  IN  UINTN         Size
  )
{
  RETURN_STATUS  Return;
  UINTN          Remaining;
  CHAR8          WriteBuffer[128];
  UINTN          WriteNb;
  UINTN          WriteSize;

  Return = SemihostFileSeek (Fcb->SemihostHandle, Fcb->Info.FileSize);
  if (RETURN_ERROR (Return)) {
    return EFI_DEVICE_ERROR;
  }

  Remaining = Size;
  ZeroMem (WriteBuffer, sizeof (WriteBuffer));
  while (Remaining > 0) {
    WriteNb   = MIN (Remaining, sizeof (WriteBuffer));
    WriteSize = WriteNb;
    Return    = SemihostFileWrite (Fcb->SemihostHandle, &WriteSize, WriteBuffer);
    if (RETURN_ERROR (Return)) {
      return EFI_DEVICE_ERROR;
    }

    Remaining -= WriteNb;
  }

  return EFI_SUCCESS;
}

/**
  Write data to an open file.

  @param[in]      This        A pointer to the EFI_FILE_PROTOCOL instance that
                              is the file handle to write data to.
  @param[in out]  BufferSize  On input, the size of the Buffer. On output, the
                              size of the data actually written. In both cases,
                              the size is measured in bytes.
  @param[in]      Buffer      The buffer of data to write.

  @retval  EFI_SUCCESS            The data was written.
  @retval  EFI_ACCESS_DENIED      Attempt to write into a read only file or
                                  in a file opened in read only mode.
  @retval  EFI_DEVICE_ERROR       The last issued semi-hosting operation failed.
  @retval  EFI_INVALID_PARAMETER  At least one of the three input pointers is NULL.

**/
EFI_STATUS
FileWrite (
  IN     EFI_FILE  *This,
  IN OUT UINTN     *BufferSize,
  IN     VOID      *Buffer
  )
{
  SEMIHOST_FCB   *Fcb;
  EFI_STATUS     Status;
  UINTN          WriteSize;
  RETURN_STATUS  Return;
  UINTN          Length;

  if ((This == NULL) || (BufferSize == NULL) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Fcb = SEMIHOST_FCB_FROM_THIS (This);

  // We cannot write a read-only file
  if (  (Fcb->Info.Attribute & EFI_FILE_READ_ONLY)
     || !(Fcb->OpenMode & EFI_FILE_MODE_WRITE))
  {
    return EFI_ACCESS_DENIED;
  }

  //
  // If the position has been set past the end of the file, first grow the
  // file from its current size "Fcb->Info.FileSize" to "Fcb->Position"
  // size, filling the gap with zeros.
  //
  if (Fcb->Position > Fcb->Info.FileSize) {
    Status = ExtendFile (Fcb, Fcb->Position - Fcb->Info.FileSize);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Fcb->Info.FileSize = Fcb->Position;
  }

  WriteSize = *BufferSize;
  Return    = SemihostFileWrite (Fcb->SemihostHandle, &WriteSize, Buffer);
  if (RETURN_ERROR (Return)) {
    return EFI_DEVICE_ERROR;
  }

  Fcb->Position += *BufferSize;
  if (Fcb->Position > Fcb->Info.FileSize) {
    Fcb->Info.FileSize = Fcb->Position;
  }

  Return = SemihostFileLength (Fcb->SemihostHandle, &Length);
  if (RETURN_ERROR (Return)) {
    return EFI_DEVICE_ERROR;
  }

  Fcb->Info.PhysicalSize = Length;

  return EFI_SUCCESS;
}

/**
  Return a file's current position.

  @param[in]   This      A pointer to the EFI_FILE_PROTOCOL instance that is
                         the file handle to get the current position on.
  @param[out]  Position  The address to return the file's current position value.

  @retval  EFI_SUCCESS            The position was returned.
  @retval  EFI_INVALID_PARAMETER  The parameter "This" or "Position" is NULL.

**/
EFI_STATUS
FileGetPosition (
  IN  EFI_FILE  *This,
  OUT UINT64    *Position
  )
{
  SEMIHOST_FCB  *Fcb;

  if ((This == NULL) || (Position == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Fcb = SEMIHOST_FCB_FROM_THIS (This);

  *Position = Fcb->Position;

  return EFI_SUCCESS;
}

/**
  Set a file's current position.

  @param[in]  This      A pointer to the EFI_FILE_PROTOCOL instance that is
                        the file handle to set the requested position on.
  @param[in]  Position  The byte position from the start of the file to set.

  @retval  EFI_SUCCESS       The position was set.
  @retval  EFI_DEVICE_ERROR  The semi-hosting positioning operation failed.
  @retval  EFI_UNSUPPORTED   The seek request for nonzero is not valid on open
                             directories.
  @retval  EFI_INVALID_PARAMETER  The parameter "This" is NULL.

**/
EFI_STATUS
FileSetPosition (
  IN EFI_FILE  *This,
  IN UINT64    Position
  )
{
  SEMIHOST_FCB   *Fcb;
  RETURN_STATUS  Return;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Fcb = SEMIHOST_FCB_FROM_THIS (This);

  if (Fcb->IsRoot) {
    if (Position != 0) {
      return EFI_UNSUPPORTED;
    }
  } else {
    //
    // UEFI Spec section 12.5:
    // "Seeking to position 0xFFFFFFFFFFFFFFFF causes the current position to
    // be set to the end of the file."
    //
    if (Position == 0xFFFFFFFFFFFFFFFF) {
      Position = Fcb->Info.FileSize;
    }

    Return = SemihostFileSeek (Fcb->SemihostHandle, MIN (Position, Fcb->Info.FileSize));
    if (RETURN_ERROR (Return)) {
      return EFI_DEVICE_ERROR;
    }
  }

  Fcb->Position = Position;

  return EFI_SUCCESS;
}

/**
  Return information about a file.

  @param[in]      Fcb         A pointer to the description of an open file.
  @param[in out]  BufferSize  The size, in bytes, of Buffer.
  @param[out]     Buffer      A pointer to the data buffer to return. Not NULL if
                              "*BufferSize" is greater than 0.

  @retval  EFI_SUCCESS            The information was returned.
  @retval  EFI_BUFFER_TOO_SMALL   The BufferSize is too small to return the information.
                                  BufferSize has been updated with the size needed to
                                  complete the request.
**/
STATIC
EFI_STATUS
GetFileInfo (
  IN     SEMIHOST_FCB  *Fcb,
  IN OUT UINTN         *BufferSize,
  OUT    VOID          *Buffer
  )
{
  EFI_FILE_INFO  *Info;
  UINTN          NameSize;
  UINTN          ResultSize;
  UINTN          Index;

  if (Fcb->IsRoot) {
    NameSize   = 0;
    ResultSize = SIZE_OF_EFI_FILE_INFO + sizeof (CHAR16);
  } else {
    NameSize   = AsciiStrLen (Fcb->FileName) + 1;
    ResultSize = SIZE_OF_EFI_FILE_INFO + NameSize * sizeof (CHAR16);
  }

  if (*BufferSize < ResultSize) {
    *BufferSize = ResultSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  Info = Buffer;

  // Copy the current file info
  CopyMem (Info, &Fcb->Info, SIZE_OF_EFI_FILE_INFO);

  // Fill in the structure
  Info->Size = ResultSize;

  if (Fcb->IsRoot) {
    Info->FileName[0] = L'\0';
  } else {
    for (Index = 0; Index < NameSize; Index++) {
      Info->FileName[Index] = Fcb->FileName[Index];
    }
  }

  *BufferSize = ResultSize;

  return EFI_SUCCESS;
}

/**
  Return information about a file system.

  @param[in]      Fcb         A pointer to the description of an open file
                              which belongs to the file system, the information
                              is requested for.
  @param[in out]  BufferSize  The size, in bytes, of Buffer.
  @param[out]     Buffer      A pointer to the data buffer to return. Not NULL if
                              "*BufferSize" is greater than 0.

  @retval  EFI_SUCCESS            The information was returned.
  @retval  EFI_BUFFER_TOO_SMALL   The BufferSize is too small to return the information.
                                  BufferSize has been updated with the size needed to
                                  complete the request.

**/
STATIC
EFI_STATUS
GetFilesystemInfo (
  IN     SEMIHOST_FCB  *Fcb,
  IN OUT UINTN         *BufferSize,
  OUT    VOID          *Buffer
  )
{
  EFI_FILE_SYSTEM_INFO  *Info;
  EFI_STATUS            Status;
  UINTN                 ResultSize;
  UINTN                 StringSize;

  StringSize = StrSize (mSemihostFsLabel);
  ResultSize = SIZE_OF_EFI_FILE_SYSTEM_INFO + StringSize;

  if (*BufferSize >= ResultSize) {
    ZeroMem (Buffer, ResultSize);
    Status = EFI_SUCCESS;

    Info = Buffer;

    Info->Size       = ResultSize;
    Info->ReadOnly   = FALSE;
    Info->VolumeSize = 0;
    Info->FreeSpace  = 0;
    Info->BlockSize  = 0;

    CopyMem (Info->VolumeLabel, mSemihostFsLabel, StringSize);
  } else {
    Status = EFI_BUFFER_TOO_SMALL;
  }

  *BufferSize = ResultSize;
  return Status;
}

/**
  Return information about a file or a file system.

  @param[in]      This             A pointer to the EFI_FILE_PROTOCOL instance that
                                   is the file handle the requested information is for.
  @param[in]      InformationType  The type identifier for the information being requested :
                                   EFI_FILE_INFO_ID or EFI_FILE_SYSTEM_INFO_ID or
                                   EFI_FILE_SYSTEM_VOLUME_LABEL_ID
  @param[in out]  BufferSize       The size, in bytes, of Buffer.
  @param[out]     Buffer           A pointer to the data buffer to return. The type of the
                                   data inside the buffer is indicated by InformationType.

  @retval  EFI_SUCCESS           The information was returned.
  @retval  EFI_UNSUPPORTED       The InformationType is not known.
  @retval  EFI_BUFFER_TOO_SMALL  The BufferSize is too small to return the information.
                                 BufferSize has been updated with the size needed to
                                 complete the request.
  @retval  EFI_INVALID_PARAMETER  The parameter "This" or "InformationType" or "BufferSize"
                                  is NULL or "Buffer" is NULL and "*Buffersize" is greater
                                  than 0.

**/
EFI_STATUS
FileGetInfo (
  IN     EFI_FILE  *This,
  IN     EFI_GUID  *InformationType,
  IN OUT UINTN     *BufferSize,
  OUT    VOID      *Buffer
  )
{
  SEMIHOST_FCB  *Fcb;
  EFI_STATUS    Status;
  UINTN         ResultSize;

  if ((This == NULL)                         ||
      (InformationType == NULL)              ||
      (BufferSize == NULL)                   ||
      ((Buffer == NULL) && (*BufferSize > 0)))
  {
    return EFI_INVALID_PARAMETER;
  }

  Fcb = SEMIHOST_FCB_FROM_THIS (This);

  if (CompareGuid (InformationType, &gEfiFileSystemInfoGuid)) {
    Status = GetFilesystemInfo (Fcb, BufferSize, Buffer);
  } else if (CompareGuid (InformationType, &gEfiFileInfoGuid)) {
    Status = GetFileInfo (Fcb, BufferSize, Buffer);
  } else if (CompareGuid (InformationType, &gEfiFileSystemVolumeLabelInfoIdGuid)) {
    ResultSize = StrSize (mSemihostFsLabel);

    if (*BufferSize >= ResultSize) {
      CopyMem (Buffer, mSemihostFsLabel, ResultSize);
      Status = EFI_SUCCESS;
    } else {
      Status = EFI_BUFFER_TOO_SMALL;
    }

    *BufferSize = ResultSize;
  } else {
    Status = EFI_UNSUPPORTED;
  }

  return Status;
}

/**
  Set information about a file.

  @param[in]  Fcb   A pointer to the description of the open file.
  @param[in]  Info  A pointer to the file information to write.

  @retval  EFI_SUCCESS           The information was set.
  @retval  EFI_ACCESS_DENIED     An attempt is made to change the name of a file
                                 to a file that is already present.
  @retval  EFI_ACCESS_DENIED     An attempt is being made to change the
                                 EFI_FILE_DIRECTORY Attribute.
  @retval  EFI_ACCESS_DENIED     The file is a read-only file or has been
                                 opened in read-only mode and an attempt is
                                 being made to modify a field other than
                                 Attribute.
  @retval  EFI_WRITE_PROTECTED   An attempt is being made to modify a
                                 read-only attribute.
  @retval  EFI_DEVICE_ERROR      The last issued semi-hosting operation failed.
  @retval  EFI_OUT_OF_RESOURCES  A allocation needed to process the request failed.

**/
STATIC
EFI_STATUS
SetFileInfo (
  IN  SEMIHOST_FCB   *Fcb,
  IN  EFI_FILE_INFO  *Info
  )
{
  EFI_STATUS     Status;
  RETURN_STATUS  Return;
  BOOLEAN        FileSizeIsDifferent;
  BOOLEAN        FileNameIsDifferent;
  BOOLEAN        ReadOnlyIsDifferent;
  CHAR8          *AsciiFileName;
  UINTN          FileSize;
  UINTN          Length;
  UINTN          SemihostHandle;

  //
  // A directory can not be changed to a file and a file can
  // not be changed to a directory.
  //
  if (((Info->Attribute & EFI_FILE_DIRECTORY) != 0) != Fcb->IsRoot) {
    return EFI_ACCESS_DENIED;
  }

  Length        = StrLen (Info->FileName) + 1;
  AsciiFileName = AllocatePool (Length);
  if (AsciiFileName == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  UnicodeStrToAsciiStrS (Info->FileName, AsciiFileName, Length);

  FileSizeIsDifferent = (Info->FileSize != Fcb->Info.FileSize);
  FileNameIsDifferent = (AsciiStrCmp (AsciiFileName, Fcb->FileName) != 0);
  ReadOnlyIsDifferent = CompareMem (
                          &Info->CreateTime,
                          &Fcb->Info.CreateTime,
                          3 * sizeof (EFI_TIME)
                          ) != 0;

  //
  // For a read-only file or a file opened in read-only mode, only
  // the Attribute field can be modified. As the root directory is
  // read-only (i.e. VolumeOpen()), this protects the root directory
  // description.
  //
  if ((Fcb->OpenMode == EFI_FILE_MODE_READ)     ||
      (Fcb->Info.Attribute & EFI_FILE_READ_ONLY))
  {
    if (FileSizeIsDifferent || FileNameIsDifferent || ReadOnlyIsDifferent) {
      Status = EFI_ACCESS_DENIED;
      goto Error;
    }
  }

  if (ReadOnlyIsDifferent) {
    Status = EFI_WRITE_PROTECTED;
    goto Error;
  }

  Status = EFI_DEVICE_ERROR;

  if (FileSizeIsDifferent) {
    FileSize = Info->FileSize;
    if (Fcb->Info.FileSize < FileSize) {
      Status = ExtendFile (Fcb, FileSize - Fcb->Info.FileSize);
      if (EFI_ERROR (Status)) {
        goto Error;
      }

      //
      // The read/write position from the host file system point of view
      // is at the end of the file. If the position from this module
      // point of view is smaller than the new file size, then
      // ask the host file system to move to that position.
      //
      if (Fcb->Position < FileSize) {
        FileSetPosition (&Fcb->File, Fcb->Position);
      }
    }

    Fcb->Info.FileSize = FileSize;

    Return = SemihostFileLength (Fcb->SemihostHandle, &Length);
    if (RETURN_ERROR (Return)) {
      goto Error;
    }

    Fcb->Info.PhysicalSize = Length;
  }

  //
  // Note down in RAM the Attribute field but we can not ask
  // for its modification to the host file system as the
  // semi-host interface does not provide this feature.
  //
  Fcb->Info.Attribute = Info->Attribute;

  if (FileNameIsDifferent) {
    Return = SemihostFileOpen (
               AsciiFileName,
               SEMIHOST_FILE_MODE_READ | SEMIHOST_FILE_MODE_BINARY,
               &SemihostHandle
               );
    if (!RETURN_ERROR (Return)) {
      SemihostFileClose (SemihostHandle);
      Status = EFI_ACCESS_DENIED;
      goto Error;
    }

    Return = SemihostFileRename (Fcb->FileName, AsciiFileName);
    if (RETURN_ERROR (Return)) {
      goto Error;
    }

    FreePool (Fcb->FileName);
    Fcb->FileName = AsciiFileName;
    AsciiFileName = NULL;
  }

  Status = EFI_SUCCESS;

Error:
  if (AsciiFileName != NULL) {
    FreePool (AsciiFileName);
  }

  return Status;
}

/**
  Set information about a file or a file system.

  @param[in]  This             A pointer to the EFI_FILE_PROTOCOL instance that
                               is the file handle the information is for.
  @param[in]  InformationType  The type identifier for the information being set :
                               EFI_FILE_INFO_ID or EFI_FILE_SYSTEM_INFO_ID or
                               EFI_FILE_SYSTEM_VOLUME_LABEL_ID
  @param[in]  BufferSize       The size, in bytes, of Buffer.
  @param[in]  Buffer           A pointer to the data buffer to write. The type of the
                               data inside the buffer is indicated by InformationType.

  @retval  EFI_SUCCESS            The information was set.
  @retval  EFI_UNSUPPORTED        The InformationType is not known.
  @retval  EFI_DEVICE_ERROR       The last issued semi-hosting operation failed.
  @retval  EFI_ACCESS_DENIED      An attempt is being made to change the
                                  EFI_FILE_DIRECTORY Attribute.
  @retval  EFI_ACCESS_DENIED      InformationType is EFI_FILE_INFO_ID and
                                  the file is a read-only file or has been
                                  opened in read-only mode and an attempt is
                                  being made to modify a field other than
                                  Attribute.
  @retval  EFI_ACCESS_DENIED      An attempt is made to change the name of a file
                                  to a file that is already present.
  @retval  EFI_WRITE_PROTECTED    An attempt is being made to modify a
                                  read-only attribute.
  @retval  EFI_BAD_BUFFER_SIZE    The size of the buffer is lower than that indicated by
                                  the data inside the buffer.
  @retval  EFI_OUT_OF_RESOURCES   An allocation needed to process the request failed.
  @retval  EFI_INVALID_PARAMETER  At least one of the parameters is invalid.

**/
EFI_STATUS
FileSetInfo (
  IN EFI_FILE  *This,
  IN EFI_GUID  *InformationType,
  IN UINTN     BufferSize,
  IN VOID      *Buffer
  )
{
  SEMIHOST_FCB          *Fcb;
  EFI_FILE_INFO         *Info;
  EFI_FILE_SYSTEM_INFO  *SystemInfo;
  CHAR16                *VolumeLabel;

  if ((This == NULL) || (InformationType == NULL) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Fcb = SEMIHOST_FCB_FROM_THIS (This);

  if (CompareGuid (InformationType, &gEfiFileInfoGuid)) {
    Info = Buffer;
    if (Info->Size < (SIZE_OF_EFI_FILE_INFO + StrSize (Info->FileName))) {
      return EFI_INVALID_PARAMETER;
    }

    if (BufferSize < Info->Size) {
      return EFI_BAD_BUFFER_SIZE;
    }

    return SetFileInfo (Fcb, Info);
  } else if (CompareGuid (InformationType, &gEfiFileSystemInfoGuid)) {
    SystemInfo = Buffer;
    if (SystemInfo->Size <
        (SIZE_OF_EFI_FILE_SYSTEM_INFO + StrSize (SystemInfo->VolumeLabel)))
    {
      return EFI_INVALID_PARAMETER;
    }

    if (BufferSize < SystemInfo->Size) {
      return EFI_BAD_BUFFER_SIZE;
    }

    Buffer = SystemInfo->VolumeLabel;

    if (StrSize (Buffer) > 0) {
      VolumeLabel = AllocateCopyPool (StrSize (Buffer), Buffer);
      if (VolumeLabel != NULL) {
        FreePool (mSemihostFsLabel);
        mSemihostFsLabel = VolumeLabel;
        return EFI_SUCCESS;
      } else {
        return EFI_OUT_OF_RESOURCES;
      }
    } else {
      return EFI_INVALID_PARAMETER;
    }
  } else if (!CompareGuid (InformationType, &gEfiFileSystemVolumeLabelInfoIdGuid)) {
    return EFI_UNSUPPORTED;
  } else {
    return EFI_UNSUPPORTED;
  }
}

EFI_STATUS
FileFlush (
  IN EFI_FILE  *File
  )
{
  SEMIHOST_FCB  *Fcb;

  Fcb = SEMIHOST_FCB_FROM_THIS (File);

  if (Fcb->IsRoot) {
    return EFI_SUCCESS;
  } else {
    if (  (Fcb->Info.Attribute & EFI_FILE_READ_ONLY)
       || !(Fcb->OpenMode & EFI_FILE_MODE_WRITE))
    {
      return EFI_ACCESS_DENIED;
    } else {
      return EFI_SUCCESS;
    }
  }
}

EFI_STATUS
SemihostFsEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = EFI_NOT_FOUND;

  if (SemihostConnectionSupported ()) {
    mSemihostFsLabel = AllocateCopyPool (StrSize (DEFAULT_SEMIHOST_FS_LABEL), DEFAULT_SEMIHOST_FS_LABEL);
    if (mSemihostFsLabel == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Status = gBS->InstallMultipleProtocolInterfaces (
                    &gInstallHandle,
                    &gEfiSimpleFileSystemProtocolGuid,
                    &gSemihostFs,
                    &gEfiDevicePathProtocolGuid,
                    &gDevicePath,
                    NULL
                    );

    if (EFI_ERROR (Status)) {
      FreePool (mSemihostFsLabel);
    }
  }

  return Status;
}
