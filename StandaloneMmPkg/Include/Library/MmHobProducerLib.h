/** @file
  HOB Producer Library implementation for Standalone MM Core.

  The MM Hob Producer Library provides function for creating the HOB list
  which StandaloneMm Core needed.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _MM_HOB_PRODUCER_LIB_H_
#define _MM_HOB_PRODUCER_LIB_H_

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
  );

#endif
