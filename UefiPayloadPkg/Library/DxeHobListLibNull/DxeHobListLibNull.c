/** @file
  This library retrieve the EFI_BOOT_SERVICES pointer from EFI system table in
  library's constructor.

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include <Uefi.h>

/**
  The dummy constructor for DxeHobListLib.

  @retval  EFI_SUCCESS

**/
EFI_STATUS
EFIAPI
DxeHobListLibNullConstructor (
  VOID
  )
{
  return EFI_SUCCESS;
}
