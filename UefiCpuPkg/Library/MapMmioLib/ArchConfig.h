/** @file
  Arch specific configuration interface definitions.

  Copyright (c) 2026, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Base.h>
#include <Uefi.h>

/**
  Configure architecture-specific attributes for a memory range.

  On architectures that require additional memory attribute configuration,
  this function applies those attributes after the range has been added to
  the GCD memory map and configured with GCD attributes.

  @param[in] RegionBase  The base address of the memory range.
  @param[in] RegionSize  The size of the memory range, in bytes.

  @retval EFI_SUCCESS            The memory range was configured successfully,
                                  or no architecture-specific configuration was
                                  required.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_UNSUPPORTED        The requested configuration is not supported.
  @retval Others                 An error occurred while configuring the memory
                                  range.
**/
EFI_STATUS
EFIAPI
ArchConfigureAttributes (
  IN EFI_PHYSICAL_ADDRESS  RegionBase,
  IN UINT64                RegionSize
  );
