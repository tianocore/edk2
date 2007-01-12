/*++

Copyright (c) 2006, Intel Corporation. All rights reserved. 
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.


Module Name:

  MemoryMappedConfigurationSpaceAccessTable.h

Abstract:

  ACPI memory mapped configuration space access table definition, defined at 
  in the PCI Firmware Specification, version 3.0 draft version 0.5.
  Specification is available at http://www.pcisig.com.

--*/

#ifndef _MEMORY_MAPPED_CONFIGURATION_SPACE_ACCESS_TABLE_H_
#define _MEMORY_MAPPED_CONFIGURATION_SPACE_ACCESS_TABLE_H_

//
// Ensure proper structure formats
//
#pragma pack(1)
//
// Memory Mapped Configuration Space Access Table (MCFG)
// This table is a basic description table header followed by
// a number of base address allocation structures.
//
typedef struct {
  UINT64  BaseAddress;
  UINT16  PciSegmentGroupNumber;
  UINT8   StartBusNumber;
  UINT8   EndBusNumber;
  UINT32  Reserved;
} EFI_ACPI_MEMORY_MAPPED_ENHANCED_CONFIGURATION_SPACE_BASE_ADDRESS_ALLOCATION_STRUCTURE;

//
// MCFG Revision (defined in spec)
//
#define EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_SPACE_ACCESS_TABLE_REVISION  0x01

#pragma pack()

#endif
