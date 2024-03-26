/** @file
  Implement EFI RealTimeClock PEI phase RTC Lib.

  Copyright (c) 2024 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/HobLib.h>

#include "LsRealTimeClock.h"

VOID
SaveRtcRegisterAddressHob (
  UINT64  RtcRegisterBase
  )
{
  UINT64  Data64;

  //
  // Build location of RTC register base address buffer in HOB
  //
  Data64 = (UINT64)(UINTN)RtcRegisterBase;

  BuildGuidDataHob (
    &mRtcRegisterBaseAddressGuid,
    (VOID *)&Data64,
    sizeof (UINT64)
    );
}
