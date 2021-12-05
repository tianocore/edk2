/** @file
  Detect Xen hvmloader SMBIOS data for usage by OVMF.

  Copyright (c) 2011, Bei Guan <gbtju85@gmail.com>
  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h> // AsciiStrnCmp()
#include <Library/HobLib.h>  // GetFirstGuidHob()
#include <Pi/PiHob.h>        // EFI_HOB_GUID_TYPE

#include "XenSmbiosPlatformDxe.h"

#define XEN_SMBIOS_PHYSICAL_ADDRESS  0x000EB000
#define XEN_SMBIOS_PHYSICAL_END      0x000F0000

/**
  Validates the SMBIOS entry point structure

  @param  EntryPointStructure  SMBIOS entry point structure

  @retval TRUE   The entry point structure is valid
  @retval FALSE  The entry point structure is not valid

**/
STATIC
BOOLEAN
IsEntryPointStructureValid (
  IN SMBIOS_TABLE_ENTRY_POINT  *EntryPointStructure
  )
{
  UINTN  Index;
  UINT8  Length;
  UINT8  Checksum;
  UINT8  *BytePtr;

  BytePtr  = (UINT8 *)EntryPointStructure;
  Length   = EntryPointStructure->EntryPointLength;
  Checksum = 0;

  for (Index = 0; Index < Length; Index++) {
    Checksum = Checksum + (UINT8)BytePtr[Index];
  }

  if (Checksum != 0) {
    return FALSE;
  } else {
    return TRUE;
  }
}

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

  for (XenSmbiosPtr = (UINT8 *)(UINTN)XEN_SMBIOS_PHYSICAL_ADDRESS;
       XenSmbiosPtr < (UINT8 *)(UINTN)XEN_SMBIOS_PHYSICAL_END;
       XenSmbiosPtr += 0x10)
  {
    XenSmbiosEntryPointStructure = (SMBIOS_TABLE_ENTRY_POINT *)XenSmbiosPtr;

    if (!AsciiStrnCmp ((CHAR8 *)XenSmbiosEntryPointStructure->AnchorString, "_SM_", 4) &&
        !AsciiStrnCmp ((CHAR8 *)XenSmbiosEntryPointStructure->IntermediateAnchorString, "_DMI_", 5) &&
        IsEntryPointStructureValid (XenSmbiosEntryPointStructure))
    {
      return XenSmbiosEntryPointStructure;
    }
  }

  return NULL;
}
