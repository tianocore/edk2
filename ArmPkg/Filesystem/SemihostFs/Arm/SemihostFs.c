/** @file
  Support a Semi Host file system over a debuggers JTAG

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Portions copyright (c) 2011, 2012, ARM Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>

#include <Guid/FileInfo.h>
#include <Guid/FileSystemInfo.h>

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


EFI_SIMPLE_FILE_SYSTEM_PROTOCOL gSemihostFs = {
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION,
  VolumeOpen
};

EFI_FILE gSemihostFsFile = {
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
// Device path for SemiHosting. It contains our autogened Caller ID GUID.
//
typedef struct {
  VENDOR_DEVICE_PATH        Guid;
  EFI_DEVICE_PATH_PROTOCOL  End;
} SEMIHOST_DEVICE_PATH;

SEMIHOST_DEVICE_PATH gDevicePath = {
  {
    { HARDWARE_DEVICE_PATH, HW_VENDOR_DP, sizeof (VENDOR_DEVICE_PATH), 0 },
    EFI_CALLER_ID_GUID
  },
  { END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, sizeof (EFI_DEVICE_PATH_PROTOCOL), 0}
};

typedef struct {
  LIST_ENTRY  Link;
  UINT64      Signature;
  EFI_FILE    File;
  CHAR8       *FileName;
  UINT32      Position;
  UINTN       SemihostHandle;
  BOOLEAN     IsRoot;
} SEMIHOST_FCB;

#define SEMIHOST_FCB_SIGNATURE      SIGNATURE_32( 'S', 'H', 'F', 'C' )
#define SEMIHOST_FCB_FROM_THIS(a)   CR(a, SEMIHOST_FCB, File, SEMIHOST_FCB_SIGNATURE)
#define SEMIHOST_FCB_FROM_LINK(a)   CR(a, SEMIHOST_FCB, Link, SEMIHOST_FCB_SIGNATURE);

EFI_HANDLE  gInstallHandle = NULL;
LIST_ENTRY  gFileList = INITIALIZE_LIST_HEAD_VARIABLE (gFileList);

SEMIHOST_FCB *
AllocateFCB (
  VOID
  )
{
  SEMIHOST_FCB *Fcb = AllocateZeroPool (sizeof (SEMIHOST_FCB));

  if (Fcb != NULL) {
    CopyMem (&Fcb->File, &gSemihostFsFile, sizeof (gSemihostFsFile));
    Fcb->Signature = SEMIHOST_FCB_SIGNATURE;
  }

  return Fcb;
}

VOID
FreeFCB (
  IN SEMIHOST_FCB *Fcb
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
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *This,
  OUT EFI_FILE                        **Root
  )
{
  SEMIHOST_FCB *RootFcb = NULL;
  
  if (Root == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  RootFcb = AllocateFCB ();
  if (RootFcb == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  
  RootFcb->IsRoot = TRUE;

  InsertTailList (&gFileList, &RootFcb->Link);

  *Root = &RootFcb->File;

  return EFI_SUCCESS;
}

EFI_STATUS
FileOpen (
  IN  EFI_FILE    *File,
  OUT EFI_FILE    **NewHandle,
  IN  CHAR16      *FileName,
  IN  UINT64      OpenMode,
  IN  UINT64      Attributes
  )
{
  SEMIHOST_FCB  *FileFcb = NULL;
  EFI_STATUS    Status   = EFI_SUCCESS;
  UINTN         SemihostHandle;
  CHAR8         *AsciiFileName;
  CHAR8         *AsciiPtr;
  UINTN         Length;
  UINT32        SemihostMode;
  BOOLEAN       IsRoot;

  if ((FileName == NULL) || (NewHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // Semihost interface requires ASCII filesnames
  Length = StrSize (FileName);

  AsciiFileName = AllocatePool (Length);
  if (AsciiFileName == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  AsciiPtr = AsciiFileName;

  while (Length--) {
    *AsciiPtr++ = *FileName++ & 0xFF;
  }

  if ((AsciiStrCmp (AsciiFileName, "\\") == 0) ||
      (AsciiStrCmp (AsciiFileName, "/")  == 0) ||
      (AsciiStrCmp (AsciiFileName, "")   == 0) ||
      (AsciiStrCmp (AsciiFileName, ".")  == 0)) {
    // Opening '/', '\', '.', or the NULL pathname is trying to open the root directory
    IsRoot = TRUE;

    // Root directory node doesn't have a name.
    FreePool (AsciiFileName);
    AsciiFileName = NULL;
  } else {
    // Translate EFI_FILE_MODE into Semihosting mode
    if (OpenMode & EFI_FILE_MODE_WRITE) {
      SemihostMode = SEMIHOST_FILE_MODE_WRITE | SEMIHOST_FILE_MODE_BINARY;
    } else if (OpenMode & EFI_FILE_MODE_READ) {
      SemihostMode = SEMIHOST_FILE_MODE_READ  | SEMIHOST_FILE_MODE_BINARY;
    } else {
      return EFI_UNSUPPORTED;
    }

    // Add the creation flag if necessary
    if (OpenMode & EFI_FILE_MODE_CREATE) {
      SemihostMode |= SEMIHOST_FILE_MODE_CREATE;
    }

    // Call the semihosting interface to open the file.
    Status = SemihostFileOpen (AsciiFileName, SemihostMode, &SemihostHandle);
    if (EFI_ERROR(Status)) {
      return Status;
    }
    
    IsRoot = FALSE;
  }

  // Allocate a control block and fill it
  FileFcb = AllocateFCB ();
  if (FileFcb == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  FileFcb->FileName       = AsciiFileName;
  FileFcb->SemihostHandle = SemihostHandle;
  FileFcb->Position       = 0;
  FileFcb->IsRoot         = IsRoot;

  InsertTailList (&gFileList, &FileFcb->Link);

  *NewHandle = &FileFcb->File;

  return Status;
}


EFI_STATUS
FileClose (
  IN EFI_FILE *File
  )
{
  SEMIHOST_FCB *Fcb    = NULL;
  EFI_STATUS   Status  = EFI_SUCCESS;

  Fcb = SEMIHOST_FCB_FROM_THIS(File);

  if (Fcb->IsRoot == TRUE) {
    FreeFCB (Fcb);
    Status = EFI_SUCCESS;
  } else {
    Status = SemihostFileClose (Fcb->SemihostHandle);
    if (!EFI_ERROR(Status)) {
      FreePool (Fcb->FileName);
      FreeFCB (Fcb);
    }
  }
  
  return Status;
}

EFI_STATUS
FileDelete (
  IN EFI_FILE *File
  )
{
  SEMIHOST_FCB *Fcb = NULL;
  EFI_STATUS   Status;
  CHAR8        *FileName;
  UINTN        NameSize;

  Fcb = SEMIHOST_FCB_FROM_THIS(File);

  if (!Fcb->IsRoot) {
    // Get the filename from the Fcb
    NameSize = AsciiStrLen (Fcb->FileName);
    FileName = AllocatePool (NameSize + 1);

    AsciiStrCpy (FileName, Fcb->FileName);

    // Close the file if it's open.  Disregard return status,
    // since it might give an error if the file isn't open.
    File->Close (File);

    // Call the semihost interface to delete the file.
    Status = SemihostFileRemove (FileName);
  } else {
    Status = EFI_UNSUPPORTED;
  }

  return Status;
}

EFI_STATUS
FileRead (
  IN     EFI_FILE *File,
  IN OUT UINTN    *BufferSize,
  OUT    VOID     *Buffer
  )
{
  SEMIHOST_FCB *Fcb = NULL;
  EFI_STATUS   Status;

  Fcb = SEMIHOST_FCB_FROM_THIS(File);

  if (Fcb->IsRoot == TRUE) {
    Status = EFI_UNSUPPORTED;
  } else {
    Status = SemihostFileRead (Fcb->SemihostHandle, BufferSize, Buffer);
    if (!EFI_ERROR (Status)) {
      Fcb->Position += *BufferSize;
    }
  }

  return Status;
}

EFI_STATUS
FileWrite (
  IN     EFI_FILE *File,
  IN OUT UINTN    *BufferSize,
  IN     VOID     *Buffer
  )
{
  SEMIHOST_FCB *Fcb    = NULL;
  EFI_STATUS   Status;
  UINTN        WriteSize = *BufferSize;

  Fcb = SEMIHOST_FCB_FROM_THIS(File);

  Status = SemihostFileWrite (Fcb->SemihostHandle, &WriteSize, Buffer);

  if (!EFI_ERROR(Status)) {
    // Semihost write return the number of bytes *NOT* written.
    *BufferSize -= WriteSize;
    Fcb->Position += *BufferSize;
  }
  
  return Status;
}

EFI_STATUS
FileGetPosition (
  IN  EFI_FILE    *File,
  OUT UINT64      *Position
  )
{
  SEMIHOST_FCB *Fcb = NULL;
    
  if (Position == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Fcb = SEMIHOST_FCB_FROM_THIS(File);

  *Position = Fcb->Position;

  return EFI_SUCCESS;
}

EFI_STATUS
FileSetPosition (
  IN EFI_FILE *File,
  IN UINT64   Position
  )
{
  SEMIHOST_FCB *Fcb    = NULL;
  UINTN        Length;
  EFI_STATUS   Status;

  Fcb = SEMIHOST_FCB_FROM_THIS(File);

  if (!Fcb->IsRoot) {
    Status = SemihostFileLength (Fcb->SemihostHandle, &Length);
    if (!EFI_ERROR(Status) && (Length < Position)) {
      Position = Length;
    }

    Status = SemihostFileSeek (Fcb->SemihostHandle, (UINT32)Position);
    if (!EFI_ERROR(Status)) {
      Fcb->Position = Position;
    }
  } else {
    Fcb->Position = Position;
    Status = EFI_SUCCESS;
  }

  return Status;
}

STATIC
EFI_STATUS
GetFileInfo (
  IN     SEMIHOST_FCB  *Fcb,
  IN OUT UINTN        *BufferSize,
  OUT    VOID         *Buffer
  )
{
  EFI_FILE_INFO   *Info = NULL;
  UINTN           NameSize = 0;
  UINTN           ResultSize;
  UINTN           Index;
  UINTN           Length;
  EFI_STATUS      Status;

  if (Fcb->IsRoot == TRUE) {
    ResultSize = SIZE_OF_EFI_FILE_INFO + sizeof(CHAR16);
  } else {
    NameSize   = AsciiStrLen (Fcb->FileName) + 1;
    ResultSize = SIZE_OF_EFI_FILE_INFO + NameSize * sizeof (CHAR16);
  }

  if (*BufferSize < ResultSize) {
    *BufferSize = ResultSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  Info = Buffer;

  // Zero out the structure
  ZeroMem (Info, SIZE_OF_EFI_FILE_INFO);

  // Fill in the structure
  Info->Size = ResultSize;

  if (Fcb->IsRoot == TRUE) {
    Info->Attribute    = EFI_FILE_READ_ONLY | EFI_FILE_DIRECTORY;
    Info->FileName[0]  = L'\0';
  } else {
    Status = SemihostFileLength (Fcb->SemihostHandle, &Length);
    if (EFI_ERROR(Status)) {
      return Status;
    }

    Info->FileSize     = Length;
    Info->PhysicalSize = Length;

    for (Index = 0; Index < NameSize; Index++) {
      Info->FileName[Index] = Fcb->FileName[Index];            
    }
  }


  *BufferSize = ResultSize;    

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
GetFilesystemInfo (
  IN     SEMIHOST_FCB *Fcb,
  IN OUT UINTN        *BufferSize,
  OUT    VOID         *Buffer
  )
{
  EFI_FILE_SYSTEM_INFO    *Info = NULL;
  EFI_STATUS              Status;
  STATIC CHAR16           Label[] = L"SemihostFs";
  UINTN                   ResultSize = SIZE_OF_EFI_FILE_SYSTEM_INFO + StrSize(Label);
    
  if(*BufferSize >= ResultSize) {
    ZeroMem (Buffer, ResultSize);
    Status = EFI_SUCCESS;
        
    Info = Buffer;

    Info->Size       = ResultSize;
    Info->ReadOnly   = FALSE;
    Info->VolumeSize = 0;
    Info->FreeSpace  = 0;
    Info->BlockSize  = 0;

    StrCpy (Info->VolumeLabel, Label);
  } else {
    Status = EFI_BUFFER_TOO_SMALL;
  }

  *BufferSize = ResultSize;    
  return Status;
}

EFI_STATUS
FileGetInfo (
  IN     EFI_FILE *File,
  IN     EFI_GUID *InformationType,
  IN OUT UINTN    *BufferSize,
  OUT    VOID     *Buffer
  )
{
  SEMIHOST_FCB *Fcb = NULL;
  EFI_STATUS   Status = EFI_UNSUPPORTED;
  
  Fcb = SEMIHOST_FCB_FROM_THIS(File);
  
  if (CompareGuid(InformationType, &gEfiFileSystemInfoGuid) != 0) {
    Status = GetFilesystemInfo(Fcb, BufferSize, Buffer);  
  } else if (CompareGuid(InformationType, &gEfiFileInfoGuid) != 0) {
    Status = GetFileInfo(Fcb, BufferSize, Buffer);  
  }
    
  return Status;
}

EFI_STATUS
FileSetInfo (
  IN EFI_FILE *File,
  IN EFI_GUID *InformationType,
  IN UINTN    BufferSize,
  IN VOID     *Buffer
  )
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
FileFlush (
  IN EFI_FILE *File
  )
{
  return EFI_SUCCESS;
}

EFI_STATUS
SemihostFsEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS    Status = EFI_NOT_FOUND;

  if (SemihostConnectionSupported ()) {
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &gInstallHandle, 
                    &gEfiSimpleFileSystemProtocolGuid, &gSemihostFs, 
                    &gEfiDevicePathProtocolGuid,       &gDevicePath,
                    NULL
                    );
  }
 
  return Status;
}

