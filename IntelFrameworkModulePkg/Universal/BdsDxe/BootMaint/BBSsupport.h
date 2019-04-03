/** @file
  declares interface functions

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EFI_BDS_BBS_SUPPORT_H_
#define _EFI_BDS_BBS_SUPPORT_H_

#include "BootMaint.h"

/**
  Build Legacy Device Name String according.

  @param CurBBSEntry     BBS Table.
  @param Index           Index.
  @param BufSize         The buffer size.
  @param BootString      The output string.

  @return VOID           No output.

**/
VOID
BdsBuildLegacyDevNameString (
  IN BBS_TABLE                     *CurBBSEntry,
  IN UINTN                         Index,
  IN UINTN                         BufSize,
  OUT CHAR16                       *BootString
  );

/**
  Group the legacy boot options in the BootOption.

  The routine assumes the boot options in the beginning that covers all the device
  types are ordered properly and re-position the following boot options just after
  the corresponding boot options with the same device type.
  For example:
  1. Input  = [Harddisk1 CdRom2 Efi1 Harddisk0 CdRom0 CdRom1 Harddisk2 Efi0]
     Assuming [Harddisk1 CdRom2 Efi1] is ordered properly
     Output = [Harddisk1 Harddisk0 Harddisk2 CdRom2 CdRom0 CdRom1 Efi1 Efi0]

  2. Input  = [Efi1 Efi0 CdRom1 Harddisk0 Harddisk1 Harddisk2 CdRom0 CdRom2]
     Assuming [Efi1 Efi0 CdRom1 Harddisk0] is ordered properly
     Output = [Efi1 Efi0 CdRom1 CdRom0 CdRom2 Harddisk0 Harddisk1 Harddisk2]
**/
VOID
GroupMultipleLegacyBootOption4SameType (
  VOID
  );

/**
  Re-order the Boot Option according to the DevOrder.

  The routine re-orders the Boot Option in BootOption array according to
  the order specified by DevOrder.

  @param DevOrder           Pointer to buffer containing the BBS Index,
                            high 8-bit value 0xFF indicating a disabled boot option
  @param DevOrderCount      Count of the BBS Index
  @param EnBootOption       Callee allocated buffer containing the enabled Boot Option Numbers
  @param EnBootOptionCount  Count of the enabled Boot Option Numbers
  @param DisBootOption      Callee allocated buffer containing the disabled Boot Option Numbers
  @param DisBootOptionCount Count of the disabled Boot Option Numbers
**/
VOID
OrderLegacyBootOption4SameType (
  UINT16                   *DevOrder,
  UINTN                    DevOrderCount,
  UINT16                   **EnBootOption,
  UINTN                    *EnBootOptionCount,
  UINT16                   **DisBootOption,
  UINTN                    *DisBootOptionCount
  );
#endif
