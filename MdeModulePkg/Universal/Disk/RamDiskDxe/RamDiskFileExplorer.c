/** @file
  Internal file explorer helper functions for RamDiskDxe driver.

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "RamDiskImpl.h"

/**
  Helper function called as part of the code needed to allocate the proper
  sized buffer for various EFI interfaces.

  @param[in, out] Status     Current status.
  @param[in, out] Buffer     Current allocated buffer, or NULL.
  @param[in]      BufferSize Current buffer size needed.

  @retval  TRUE         If the buffer was reallocated and the caller should
                        try the API again.
  @retval  FALSE        The caller should not call this function again.

**/
BOOLEAN
GrowBuffer (
  IN OUT EFI_STATUS  *Status,
  IN OUT VOID        **Buffer,
  IN UINTN           BufferSize
  )
{
  BOOLEAN  TryAgain;

  //
  // If this is an initial request, buffer will be null with a new buffer size
  //
  if ((*Buffer == NULL) && (BufferSize != 0)) {
    *Status = EFI_BUFFER_TOO_SMALL;
  }

  //
  // If the status code is "buffer too small", resize the buffer
  //
  TryAgain = FALSE;
  if (*Status == EFI_BUFFER_TOO_SMALL) {
    if (*Buffer != NULL) {
      FreePool (*Buffer);
    }

    *Buffer = AllocateZeroPool (BufferSize);

    if (*Buffer != NULL) {
      TryAgain = TRUE;
    } else {
      *Status = EFI_OUT_OF_RESOURCES;
    }
  }

  //
  // If there's an error, free the buffer
  //
  if (!TryAgain && EFI_ERROR (*Status) && (*Buffer != NULL)) {
    FreePool (*Buffer);
    *Buffer = NULL;
  }

  return TryAgain;
}

/**
  This function gets the file information from an open file descriptor,
  and stores it in a buffer allocated from pool.

  @param[in] FHand           File Handle.

  @return    A pointer to a buffer with file information or NULL is returned.

**/
EFI_FILE_INFO *
FileInfo (
  IN EFI_FILE_HANDLE  FHand
  )
{
  EFI_STATUS     Status;
  EFI_FILE_INFO  *Buffer;
  UINTN          BufferSize;

  //
  // Initialize for GrowBuffer loop
  //
  Buffer     = NULL;
  BufferSize = SIZE_OF_EFI_FILE_INFO + 200;

  //
  // Call the real function
  //
  while (GrowBuffer (&Status, (VOID **)&Buffer, BufferSize)) {
    Status = FHand->GetInfo (
                      FHand,
                      &gEfiFileInfoGuid,
                      &BufferSize,
                      Buffer
                      );
  }

  return Buffer;
}
