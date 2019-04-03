/** @file
  Helper functions for SecureBoot configuration module.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SecureBootConfigImpl.h"

/**
  Read file content into BufferPtr, the size of the allocate buffer
  is *FileSize plus AddtionAllocateSize.

  @param[in]       FileHandle            The file to be read.
  @param[in, out]  BufferPtr             Pointers to the pointer of allocated buffer.
  @param[out]      FileSize              Size of input file
  @param[in]       AddtionAllocateSize   Addtion size the buffer need to be allocated.
                                         In case the buffer need to contain others besides the file content.

  @retval   EFI_SUCCESS                  The file was read into the buffer.
  @retval   EFI_INVALID_PARAMETER        A parameter was invalid.
  @retval   EFI_OUT_OF_RESOURCES         A memory allocation failed.
  @retval   others                       Unexpected error.

**/
EFI_STATUS
ReadFileContent (
  IN      EFI_FILE_HANDLE           FileHandle,
  IN OUT  VOID                      **BufferPtr,
     OUT  UINTN                     *FileSize,
  IN      UINTN                     AddtionAllocateSize
  )

{
  UINTN      BufferSize;
  UINT64     SourceFileSize;
  VOID       *Buffer;
  EFI_STATUS Status;

  if ((FileHandle == NULL) || (FileSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Buffer = NULL;

  //
  // Get the file size
  //
  Status = FileHandle->SetPosition (FileHandle, (UINT64) -1);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  Status = FileHandle->GetPosition (FileHandle, &SourceFileSize);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  Status = FileHandle->SetPosition (FileHandle, 0);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  BufferSize = (UINTN) SourceFileSize + AddtionAllocateSize;
  Buffer =  AllocateZeroPool(BufferSize);
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  BufferSize = (UINTN) SourceFileSize;
  *FileSize  = BufferSize;

  Status = FileHandle->Read (FileHandle, &BufferSize, Buffer);
  if (EFI_ERROR (Status) || BufferSize != *FileSize) {
    FreePool (Buffer);
    Buffer = NULL;
    Status  = EFI_BAD_BUFFER_SIZE;
    goto ON_EXIT;
  }

ON_EXIT:

  *BufferPtr = Buffer;
  return Status;
}

/**
  Close an open file handle.

  @param[in] FileHandle           The file handle to close.

**/
VOID
CloseFile (
  IN EFI_FILE_HANDLE   FileHandle
  )
{
  if (FileHandle != NULL) {
    FileHandle->Close (FileHandle);
  }
}

/**
  Convert a nonnegative integer to an octet string of a specified length.

  @param[in]   Integer          Pointer to the nonnegative integer to be converted
  @param[in]   IntSizeInWords   Length of integer buffer in words
  @param[out]  OctetString      Converted octet string of the specified length
  @param[in]   OSSizeInBytes    Intended length of resulting octet string in bytes

Returns:

  @retval   EFI_SUCCESS            Data conversion successfully
  @retval   EFI_BUFFER_TOOL_SMALL  Buffer is too small for output string

**/
EFI_STATUS
EFIAPI
Int2OctStr (
  IN     CONST UINTN                *Integer,
  IN     UINTN                      IntSizeInWords,
     OUT UINT8                      *OctetString,
  IN     UINTN                      OSSizeInBytes
  )
{
  CONST UINT8  *Ptr1;
  UINT8        *Ptr2;

  for (Ptr1 = (CONST UINT8 *)Integer, Ptr2 = OctetString + OSSizeInBytes - 1;
       Ptr1 < (UINT8 *)(Integer + IntSizeInWords) && Ptr2 >= OctetString;
       Ptr1++, Ptr2--) {
    *Ptr2 = *Ptr1;
  }

  for (; Ptr1 < (CONST UINT8 *)(Integer + IntSizeInWords) && *Ptr1 == 0; Ptr1++);

  if (Ptr1 < (CONST UINT8 *)(Integer + IntSizeInWords)) {
    return EFI_BUFFER_TOO_SMALL;
  }

  if (Ptr2 >= OctetString) {
    ZeroMem (OctetString, Ptr2 - OctetString + 1);
  }

  return EFI_SUCCESS;
}

/**
  Worker function that prints an EFI_GUID into specified Buffer.

  @param[in]     Guid          Pointer to GUID to print.
  @param[in]     Buffer        Buffer to print Guid into.
  @param[in]     BufferSize    Size of Buffer.

  @retval    Number of characters printed.

**/
UINTN
GuidToString (
  IN  EFI_GUID  *Guid,
  IN  CHAR16    *Buffer,
  IN  UINTN     BufferSize
  )
{
  UINTN Size;

  Size = UnicodeSPrint (
            Buffer,
            BufferSize,
            L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            (UINTN)Guid->Data1,
            (UINTN)Guid->Data2,
            (UINTN)Guid->Data3,
            (UINTN)Guid->Data4[0],
            (UINTN)Guid->Data4[1],
            (UINTN)Guid->Data4[2],
            (UINTN)Guid->Data4[3],
            (UINTN)Guid->Data4[4],
            (UINTN)Guid->Data4[5],
            (UINTN)Guid->Data4[6],
            (UINTN)Guid->Data4[7]
            );

  //
  // SPrint will null terminate the string. The -1 skips the null
  //
  return Size - 1;
}
