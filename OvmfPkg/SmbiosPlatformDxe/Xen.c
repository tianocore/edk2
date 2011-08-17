/** @file
  Detect Xen hvmloader SMBIOS data for usage by OVMF.

  Copyright (c) 2011, Bei Guan <gbtju85@gmail.com>
  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "SmbiosPlatformDxe.h"
#include <Library/HobLib.h>
#include <Guid/XenInfo.h>

#define XEN_SMBIOS_PHYSICAL_ADDRESS       0x000EB000
#define XEN_SMBIOS_PHYSICAL_END           0x000F0000

/**
  Locates the Xen SMBIOS data if it exists

  @return SMBIOS_TABLE_ENTRY_POINT   Address of Xen SMBIOS data

**/
SMBIOS_TABLE_ENTRY_POINT *
GetXenSmbiosTables (
  VOID
  )
{
  UINT8                     *XenSmbiosPtr;
  SMBIOS_TABLE_ENTRY_POINT  *XenSmbiosEntryPointStructure;
  EFI_HOB_GUID_TYPE         *GuidHob;

  //
  // See if a XenInfo HOB is available
  //
  GuidHob = GetFirstGuidHob (&gEfiXenInfoGuid);
  if (GuidHob == NULL) {
    return NULL;
  }

  for (XenSmbiosPtr = (UINT8*)(UINTN) XEN_SMBIOS_PHYSICAL_ADDRESS;
       XenSmbiosPtr < (UINT8*)(UINTN) XEN_SMBIOS_PHYSICAL_END;
       XenSmbiosPtr += 0x10) {

    XenSmbiosEntryPointStructure = (SMBIOS_TABLE_ENTRY_POINT *) XenSmbiosPtr;

    if (!AsciiStrnCmp ((CHAR8 *) XenSmbiosEntryPointStructure->AnchorString, "_SM_", 4) &&
        !AsciiStrnCmp ((CHAR8 *) XenSmbiosEntryPointStructure->IntermediateAnchorString, "_DMI_", 5) &&
        IsEntryPointStructureValid (XenSmbiosEntryPointStructure)) {

      return XenSmbiosEntryPointStructure;

    }
  }

  return NULL;
}
