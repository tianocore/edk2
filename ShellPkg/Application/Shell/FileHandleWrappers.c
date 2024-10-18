/** @file
  EFI_FILE_PROTOCOL wrappers for other items (Like Environment Variables,
  StdIn, StdOut, StdErr, etc...).

  Copyright 2016 Dell Inc.
  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2013 Hewlett-Packard Development Company, L.P.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Shell.h"

#define MEM_WRITE_REALLOC_OVERHEAD  1024

//
// This is identical to EFI_FILE_PROTOCOL except for the additional members
// for the buffer, size, and position.
//

typedef struct {
  UINT64                   Revision;
  EFI_FILE_OPEN            Open;
  EFI_FILE_CLOSE           Close;
  EFI_FILE_DELETE          Delete;
  EFI_FILE_READ            Read;
  EFI_FILE_WRITE           Write;
  EFI_FILE_GET_POSITION    GetPosition;
  EFI_FILE_SET_POSITION    SetPosition;
  EFI_FILE_GET_INFO        GetInfo;
  EFI_FILE_SET_INFO        SetInfo;
  EFI_FILE_FLUSH           Flush;
  VOID                     *Buffer;
  UINT64                   Position;
  UINT64                   BufferSize;
  BOOLEAN                  Unicode;
  UINT64                   FileSize;
} EFI_FILE_PROTOCOL_MEM;

/**
  File style interface for Mem (SetPosition).

  @param[in] This       The pointer to the EFI_FILE_PROTOCOL object.
  @param[out] Position  The position to set.

  @retval EFI_SUCCESS             The position was successfully changed.
  @retval EFI_INVALID_PARAMETER   The Position was invalid.
**/
EFI_STATUS
EFIAPI
FileInterfaceMemSetPosition (
  IN EFI_FILE_PROTOCOL  *This,
  OUT UINT64            Position
  )
{
  if (Position <= ((EFI_FILE_PROTOCOL_MEM *)This)->FileSize) {
    ((EFI_FILE_PROTOCOL_MEM *)This)->Position = Position;
    return (EFI_SUCCESS);
  } else {
    return (EFI_INVALID_PARAMETER);
  }
}

/**
  File style interface for Mem (GetPosition).

  @param[in] This       The pointer to the EFI_FILE_PROTOCOL object.
  @param[out] Position  The pointer to the position.

  @retval EFI_SUCCESS   The position was retrieved.
**/
EFI_STATUS
EFIAPI
FileInterfaceMemGetPosition (
  IN EFI_FILE_PROTOCOL  *This,
  OUT UINT64            *Position
  )
{
  *Position = ((EFI_FILE_PROTOCOL_MEM *)This)->Position;
  return (EFI_SUCCESS);
}

/**
  File style interface for Mem (GetInfo).

  @param  This            Protocol instance pointer.
  @param  InformationType Type of information to return in Buffer.
  @param  BufferSize      On input size of buffer, on output amount of data in buffer.
  @param  Buffer          The buffer to return data.

  @retval EFI_SUCCESS          Data was returned.
  @retval EFI_UNSUPPORT        InformationType is not supported.
  @retval EFI_NO_MEDIA         The device has no media.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED  The device is write protected.
  @retval EFI_ACCESS_DENIED    The file was open for read only.
  @retval EFI_BUFFER_TOO_SMALL Buffer was too small; required size returned in BufferSize.

**/
EFI_STATUS
EFIAPI
FileInterfaceMemGetInfo (
  IN EFI_FILE_PROTOCOL  *This,
  IN EFI_GUID           *InformationType,
  IN OUT UINTN          *BufferSize,
  OUT VOID              *Buffer
  )
{
  EFI_FILE_INFO  *FileInfo;

  if (CompareGuid (InformationType, &gEfiFileInfoGuid)) {
    if (*BufferSize < sizeof (EFI_FILE_INFO)) {
      *BufferSize = sizeof (EFI_FILE_INFO);
      return EFI_BUFFER_TOO_SMALL;
    }

    if (Buffer == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    FileInfo       = (EFI_FILE_INFO *)Buffer;
    FileInfo->Size = sizeof (*FileInfo);
    ZeroMem (FileInfo, sizeof (*FileInfo));
    FileInfo->FileSize     = ((EFI_FILE_PROTOCOL_MEM *)This)->FileSize;
    FileInfo->PhysicalSize = FileInfo->FileSize;
    return EFI_SUCCESS;
  }

  return EFI_UNSUPPORTED;
}

/**
  File style interface for Mem (Write).

  @param[in] This              The pointer to the EFI_FILE_PROTOCOL object.
  @param[in, out] BufferSize   Size in bytes of Buffer.
  @param[in] Buffer            The pointer to the buffer to write.

  @retval EFI_OUT_OF_RESOURCES The operation failed due to lack of resources.
  @retval EFI_SUCCESS          The data was written.
**/
EFI_STATUS
EFIAPI
FileInterfaceMemWrite (
  IN EFI_FILE_PROTOCOL  *This,
  IN OUT UINTN          *BufferSize,
  IN VOID               *Buffer
  )
{
  CHAR8                  *AsciiBuffer;
  EFI_FILE_PROTOCOL_MEM  *MemFile;

  MemFile = (EFI_FILE_PROTOCOL_MEM *)This;
  if (MemFile->Unicode) {
    //
    // Unicode
    //
    if ((UINTN)(MemFile->Position + (*BufferSize)) > (UINTN)(MemFile->BufferSize)) {
      MemFile->Buffer = ReallocatePool ((UINTN)(MemFile->BufferSize), (UINTN)(MemFile->BufferSize) + (*BufferSize) + MEM_WRITE_REALLOC_OVERHEAD, MemFile->Buffer);
      if (MemFile->Buffer == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      MemFile->BufferSize += (*BufferSize) + MEM_WRITE_REALLOC_OVERHEAD;
    }

    CopyMem (((UINT8 *)MemFile->Buffer) + MemFile->Position, Buffer, *BufferSize);
    MemFile->Position += (*BufferSize);
    MemFile->FileSize  = MemFile->Position;
    return (EFI_SUCCESS);
  } else {
    //
    // Ascii
    //
    AsciiBuffer = AllocateZeroPool (*BufferSize);
    if (AsciiBuffer == NULL) {
      return (EFI_OUT_OF_RESOURCES);
    }

    AsciiSPrint (AsciiBuffer, *BufferSize, "%S", Buffer);
    if ((UINTN)(MemFile->Position + AsciiStrSize (AsciiBuffer)) > (UINTN)(MemFile->BufferSize)) {
      MemFile->Buffer = ReallocatePool ((UINTN)(MemFile->BufferSize), (UINTN)(MemFile->BufferSize) + AsciiStrSize (AsciiBuffer) + MEM_WRITE_REALLOC_OVERHEAD, MemFile->Buffer);
      if (MemFile->Buffer == NULL) {
        FreePool (AsciiBuffer);
        return EFI_OUT_OF_RESOURCES;
      }

      MemFile->BufferSize += AsciiStrSize (AsciiBuffer) + MEM_WRITE_REALLOC_OVERHEAD;
    }

    CopyMem (((UINT8 *)MemFile->Buffer) + MemFile->Position, AsciiBuffer, AsciiStrSize (AsciiBuffer));
    MemFile->Position += (*BufferSize / sizeof (CHAR16));
    MemFile->FileSize  = MemFile->Position;
    FreePool (AsciiBuffer);
    return (EFI_SUCCESS);
  }
}

/**
  File style interface for Mem (Read).

  @param[in] This              The pointer to the EFI_FILE_PROTOCOL object.
  @param[in, out] BufferSize   Size in bytes of Buffer.
  @param[in] Buffer            The pointer to the buffer to fill.

  @retval EFI_SUCCESS   The data was read.
**/
EFI_STATUS
EFIAPI
FileInterfaceMemRead (
  IN EFI_FILE_PROTOCOL  *This,
  IN OUT UINTN          *BufferSize,
  IN VOID               *Buffer
  )
{
  EFI_FILE_PROTOCOL_MEM  *MemFile;

  MemFile = (EFI_FILE_PROTOCOL_MEM *)This;
  if (*BufferSize > (UINTN)((MemFile->FileSize) - (UINTN)(MemFile->Position))) {
    (*BufferSize) = (UINTN)((MemFile->FileSize) - (UINTN)(MemFile->Position));
  }

  CopyMem (Buffer, ((UINT8 *)MemFile->Buffer) + MemFile->Position, (*BufferSize));
  MemFile->Position = MemFile->Position + (*BufferSize);
  return (EFI_SUCCESS);
}

/**
  File style interface for Mem (Close).

  Frees all memory associated with this object.

  @param[in] This       The pointer to the EFI_FILE_PROTOCOL object.

  @retval EFI_SUCCESS   The 'file' was closed.
**/
EFI_STATUS
EFIAPI
FileInterfaceMemClose (
  IN EFI_FILE_PROTOCOL  *This
  )
{
  SHELL_FREE_NON_NULL (((EFI_FILE_PROTOCOL_MEM *)This)->Buffer);
  SHELL_FREE_NON_NULL (This);
  return (EFI_SUCCESS);
}

/**
  Creates a EFI_FILE_PROTOCOL (almost) object for using to access
  a file entirely in memory through file operations.

  @param[in] Unicode Boolean value with TRUE for Unicode and FALSE for Ascii.

  @retval NULL      Memory could not be allocated.
  @return other     A pointer to an EFI_FILE_PROTOCOL structure.
**/
EFI_FILE_PROTOCOL *
CreateFileInterfaceMem (
  IN CONST BOOLEAN  Unicode
  )
{
  EFI_FILE_PROTOCOL_MEM  *FileInterface;

  //
  // Get some memory
  //
  FileInterface = AllocateZeroPool (sizeof (EFI_FILE_PROTOCOL_MEM));
  if (FileInterface == NULL) {
    return (NULL);
  }

  //
  // Assign the generic members
  //
  FileInterface->Revision    = EFI_FILE_REVISION;
  FileInterface->Open        = FileInterfaceOpenNotFound;
  FileInterface->Close       = FileInterfaceMemClose;
  FileInterface->GetPosition = FileInterfaceMemGetPosition;
  FileInterface->SetPosition = FileInterfaceMemSetPosition;
  FileInterface->GetInfo     = FileInterfaceMemGetInfo;
  FileInterface->SetInfo     = FileInterfaceNopSetInfo;
  FileInterface->Flush       = FileInterfaceNopGeneric;
  FileInterface->Delete      = FileInterfaceNopGeneric;
  FileInterface->Read        = FileInterfaceMemRead;
  FileInterface->Write       = FileInterfaceMemWrite;
  FileInterface->Unicode     = Unicode;

  ASSERT (FileInterface->Buffer      == NULL);
  ASSERT (FileInterface->BufferSize  == 0);
  ASSERT (FileInterface->Position    == 0);

  if (Unicode) {
    FileInterface->Buffer = AllocateZeroPool (sizeof (gUnicodeFileTag));
    if (FileInterface->Buffer == NULL) {
      FreePool (FileInterface);
      return NULL;
    }

    *((CHAR16 *)(FileInterface->Buffer)) = EFI_UNICODE_BYTE_ORDER_MARK;
    FileInterface->BufferSize            = 2;
    FileInterface->Position              = 2;
  }

  return ((EFI_FILE_PROTOCOL *)FileInterface);
}
