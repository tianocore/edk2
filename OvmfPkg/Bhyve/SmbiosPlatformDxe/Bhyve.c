/*
 * Copyright (c) 2020, Rebecca Cran <rebecca@bsdio.com>
 * Copyright (c) 2014, Pluribus Networks, Inc.
 *
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */

#include "SmbiosPlatformDxe.h"

#define BHYVE_SMBIOS_PHYSICAL_ADDRESS       0x000F0000
#define BHYVE_SMBIOS_PHYSICAL_END           0x000FFFFF

/**
  Locates the bhyve SMBIOS data if it exists

  @return SMBIOS_TABLE_ENTRY_POINT   Address of bhyve SMBIOS data

**/
SMBIOS_TABLE_ENTRY_POINT *
GetBhyveSmbiosTables (
  VOID
  )
{
  UINT8                     *BhyveSmbiosPtr;
  SMBIOS_TABLE_ENTRY_POINT  *BhyveSmbiosEntryPointStructure;

  for (BhyveSmbiosPtr = (UINT8*)(UINTN) BHYVE_SMBIOS_PHYSICAL_ADDRESS;
       BhyveSmbiosPtr < (UINT8*)(UINTN) BHYVE_SMBIOS_PHYSICAL_END;
       BhyveSmbiosPtr += 0x10) {

    BhyveSmbiosEntryPointStructure = (SMBIOS_TABLE_ENTRY_POINT *) BhyveSmbiosPtr;

    if (!AsciiStrnCmp ((CHAR8 *) BhyveSmbiosEntryPointStructure->AnchorString, "_SM_", 4) &&
        !AsciiStrnCmp ((CHAR8 *) BhyveSmbiosEntryPointStructure->IntermediateAnchorString, "_DMI_", 5) &&
        IsEntryPointStructureValid (BhyveSmbiosEntryPointStructure)) {

      return BhyveSmbiosEntryPointStructure;

    }
  }

  return NULL;
}
