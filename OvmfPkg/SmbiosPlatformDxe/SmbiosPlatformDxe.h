/** @file
  This driver installs SMBIOS information for OVMF

  Copyright (c) 2011, Bei Guan <gbtju85@gmail.com>
  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _SMBIOS_PLATFORM_DXE_H_
#define _SMBIOS_PLATFORM_DXE_H_

#include <PiDxe.h>

#include <Protocol/Smbios.h>
#include <IndustryStandard/SmBios.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>


/**
  Locates the Xen SMBIOS data if it exists

  @return SMBIOS_TABLE_ENTRY_POINT   Address of Xen SMBIOS data

**/
SMBIOS_TABLE_ENTRY_POINT *
GetXenSmbiosTables (
  VOID
  );


/**
  Locates and extracts the QEMU SMBIOS table data if present in fw_cfg

  @return             Address of extracted QEMU SMBIOS data

**/
UINT8 *
GetQemuSmbiosTables (
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

#endif
