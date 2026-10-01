/** @file
  Variable runtime cache HOB construction interface.

Copyright (c) 2006 - 2024, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Uefi.h>

/**
  Build the variable runtime cache info HOB from a buffer.

  @param  NvAuthFlag  Pointer to the authenticated variable flag. When NULL, the variable store format is unknown, and the cache info will reserve enough space for either supported format.
  @param  Buffer      Pointer to the buffer containing the cache info. This parameter is optional and can be NULL.
  @param  Pages       Pointer to the number of pages in the buffer. On input, it specifies the number of pages
                      allocated for the buffer. On output, it returns the actual number of pages needed by the cache info.

  @retval EFI_SUCCESS           The HOB was built successfully.
  @retval EFI_INVALID_PARAMETER One or more of the input parameters are NULL or invalid.
  @retval EFI_BUFFER_TOO_SMALL  The buffer provided is too small to hold the cache info.
  @retval EFI_DEVICE_ERROR      The HOB could not be built due to a device error.
**/
EFI_STATUS
EFIAPI
BuildVariableRuntimeCacheInfoHobFromBuffer (
  IN       BOOLEAN  *NvAuthFlag OPTIONAL,
  IN       VOID     *Buffer OPTIONAL,
  IN OUT   UINTN    *Pages
  );
