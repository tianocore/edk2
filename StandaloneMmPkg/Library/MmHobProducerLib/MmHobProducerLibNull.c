/** @file
  HOB Producer Library implementation for Standalone MM Core.

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <PiPei.h>

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
                                    the BufferSize value will be changed the used buffer size.
  @retval RETURN_SUCCESS            The HOB list is created successfully.

**/
EFI_STATUS
EFIAPI
CreateMmCoreHobList (
  IN      VOID   *Buffer,
  IN OUT  UINTN  *BufferSize
  )
{
  if (BufferSize == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  if ((*BufferSize != 0) && (Buffer == NULL)) {
    return RETURN_INVALID_PARAMETER;
  }

  *BufferSize = 0;

  return EFI_SUCCESS;
}
