/** @file
  Simple PC Port 0x92 reset driver

  Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
  Portions copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


**/



VOID
LibResetInitializeReset (
  VOID
  )
{
}

VOID
LibResetVirtualAddressChangeEvent (
  VOID
  )
{
}


VOID
LibResetSystem (
  IN EFI_RESET_TYPE   ResetType,
  IN EFI_STATUS       ResetStatus,
  IN UINTN            DataSize,
  IN CHAR16           *ResetData OPTIONAL
  )
{
  UINT8   Data;

  switch (ResetType) {
  case EfiResetWarm:
  case EfiResetCold:
  case EfiResetShutdown:
    Data = IoRead8 (0x92);
    Data |= 1;
    IoWrite8 (0x92, Data);
    break;

  default:
    return ;
  }

  //
  // Given we should have reset getting here would be bad
  //
  ASSERT (FALSE);
}

