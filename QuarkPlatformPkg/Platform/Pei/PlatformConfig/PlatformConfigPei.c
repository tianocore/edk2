/** @file
Principle source module for Clanton Peak platform config PEIM driver.

Copyright (c) 2013 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>
#include <Library/IntelQNCLib.h>
#include <Library/PlatformHelperLib.h>
#include <Library/QNCAccessLib.h>

VOID
EFIAPI
LegacySpiProtect (
  VOID
  )
{
  UINT32  RegVal;

  RegVal = PcdGet32 (PcdLegacyProtectedBIOSRange0Pei);
  if (RegVal != 0) {
    PlatformWriteFirstFreeSpiProtect (
      RegVal,
      0,
      0
      );
  }
  RegVal = PcdGet32 (PcdLegacyProtectedBIOSRange1Pei);
  if (RegVal != 0) {
    PlatformWriteFirstFreeSpiProtect (
      RegVal,
      0,
      0
      );
  }
  RegVal = PcdGet32 (PcdLegacyProtectedBIOSRange2Pei);
  if (RegVal != 0) {
    PlatformWriteFirstFreeSpiProtect (
      RegVal,
      0,
      0
      );
  }

  //
  // Make legacy SPI READ/WRITE enabled if not a secure build
  //
  LpcPciCfg32And (R_QNC_LPC_BIOS_CNTL, ~B_QNC_LPC_BIOS_CNTL_BIOSWE);
}

/** PlatformConfigPei driver entry point.

  Platform config in PEI stage.

  @param[in]       FfsHeader    Pointer to Firmware File System file header.
  @param[in]       PeiServices  General purpose services available to every PEIM.

  @retval EFI_SUCCESS           Platform config success.
*/
EFI_STATUS
EFIAPI
PlatformConfigPeiInit (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  //
  // Do SOC Init Pre memory init.
  //
  PeiQNCPreMemInit ();

  //
  // Protect areas specified by PCDs.
  //
  LegacySpiProtect ();

  return EFI_SUCCESS;
}
