/** @file
  MM Platform HOB Producer Library Class.

  CreateMmPlatformHob() function is for StandaloneMm IPL to create any
  Platform HOBs that required by StandaloneMm Core.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MM_HOB_PRODUCER_LIB_H_
#define MM_HOB_PRODUCER_LIB_H_

/**
  Create the platform specific HOB list which StandaloneMm Core needs.

  This function build the platform specific HOB list needed by StandaloneMm Core
  based on the PEI HOB list.

  @param[in]      Buffer            The free buffer to be used for HOB creation.
  @param[in, out] BufferSize        The buffer size.
                                    On return, the expected/used size.

  @retval RETURN_INVALID_PARAMETER  BufferSize is NULL.
  @retval RETURN_INVALID_PARAMETER  Buffer is NULL and BufferSize is not 0.
  @retval RETURN_BUFFER_TOO_SMALL   The buffer is too small for HOB creation.
                                    BufferSize is updated to indicate the expected buffer size.
                                    When the input BufferSize is bigger than the expected buffer size,
                                    the BufferSize value will be changed to the used buffer size.
  @retval RETURN_SUCCESS            The HOB list is created successfully.

**/
EFI_STATUS
EFIAPI
CreateMmPlatformHob (
  IN      VOID   *Buffer,
  IN  OUT UINTN  *BufferSize
  );

#endif
