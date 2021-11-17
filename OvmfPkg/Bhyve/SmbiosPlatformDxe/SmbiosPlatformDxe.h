/** @file
  This driver installs SMBIOS information for OVMF

  Copyright (c) 2020, Rebecca Cran <rebecca@bsdio.com>
  Copyright (c) 2011, Bei Guan <gbtju85@gmail.com>
  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SMBIOS_PLATFORM_DXE_H_
#define _SMBIOS_PLATFORM_DXE_H_

#include <PiDxe.h>
#include <Protocol/Smbios.h>
#include <IndustryStandard/SmBios.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

/**
  Locates the bhyve SMBIOS data if it exists

  @return SMBIOS_TABLE_ENTRY_POINT   Address of bhyve SMBIOS data

**/
SMBIOS_TABLE_ENTRY_POINT *
GetBhyveSmbiosTables (
  VOID
  );

/**
  Validates the SMBIOS entry point structure

  @param  EntryPointStructure  SMBIOS entry point structure

  @retval TRUE   The entry point structure is valid
  @retval FALSE  The entry point structure is not valid

**/
BOOLEAN
IsEntryPointStructureValid (
  IN SMBIOS_TABLE_ENTRY_POINT  *EntryPointStructure
  );

#endif /* _SMBIOS_PLATFORM_DXE_H_ */
