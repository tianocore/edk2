/** @file
This file describes the contents of the ACPI Memory Mapped Configuration
Space Access Table (MCFG).  Some additional ACPI values are defined in Acpi10.h,
Acpi20.h, and Acpi30.h.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _MCFG_H_
#define _MCFG_H_

//
// Statements that include other files
//

#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/MemoryMappedConfigurationSpaceAccessTable.h>

//
// MCFG Definitions
//

#define EFI_ACPI_OEM_MCFG_REVISION 0x00000001

//
// Define the number of allocation structures so that we can build the table structure.
//

#define EFI_ACPI_ALLOCATION_STRUCTURE_COUNT           1

//
// MCFG structure
//

//
// Ensure proper structure formats
//
#pragma pack (1)

//
// MCFG Table structure
//
typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER                                                            Header;
  UINT64                                                                                 Reserved;
#if EFI_ACPI_ALLOCATION_STRUCTURE_COUNT > 0
  EFI_ACPI_MEMORY_MAPPED_ENHANCED_CONFIGURATION_SPACE_BASE_ADDRESS_ALLOCATION_STRUCTURE  AllocationStructure[EFI_ACPI_ALLOCATION_STRUCTURE_COUNT];
#endif
} EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_SPACE_ACCESS_DESCRIPTION_TABLE;

#pragma pack ()

#endif
