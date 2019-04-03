/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   


Module Name:

  Mcfg.h

Abstract:

  ACPI Memory mapped configuration space base address Description Table
  implementation, based on PCI Firmware Specification Revision 3.0 final draft,
  downloadable at http://www.pcisig.com/home

**/

#ifndef _MCFG_H_
#define _MCFG_H_

//
// Statements that include other files
//
#include <IndustryStandard/Acpi20.h>
#include "McfgTable.h"
#include "Platform.h"

//
// "MCFG" Static Resource Affinity Table
//
#define EFI_ACPI_3_0_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE_SIGNATURE 0x4746434D

//
// MCFG Definitions, see specification for details.
//
#define EFI_ACPI_OEM_MCFG_REVISION  0x00000001

//
// Define the number of each table type.
// This is where the table layout is modified.
//
#define EFI_ACPI_MEMORY_MAPPED_ENHANCED_CONFIGURATION_BASE_ADDRESS_STRUCTURE_COUNT  1

//
// MCFG Table definition.  The table must be defined in a platform
// specific manner.
//
//
// Ensure proper structure formats
//
#pragma pack(1)

typedef struct {
  EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE_HEADER        Header;

#if EFI_ACPI_MEMORY_MAPPED_ENHANCED_CONFIGURATION_BASE_ADDRESS_STRUCTURE_COUNT > 0
  EFI_ACPI_MEMORY_MAPPED_ENHANCED_CONFIGURATION_BASE_ADDRESS_STRUCTURE  Segment[
    EFI_ACPI_MEMORY_MAPPED_ENHANCED_CONFIGURATION_BASE_ADDRESS_STRUCTURE_COUNT];
#endif

} EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE;

#pragma pack()

#endif // _MCFG_H_
