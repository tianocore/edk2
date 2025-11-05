/** @file
  This file will report some MMIO/IO resources to dxe core.

  Copyright (c) 2014 - 2021, Intel Corporation. All rights reserved.<BR>
  Copyright 2025 Google LLC
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "BlSupportDxe.h"

/**
  Architecture level additional operation which needs to be performed before
  launching payload.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
BlArchAdditionalOps (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return EFI_SUCCESS;
}
