/** @file
  This file defines the hob structure for the SPI flash variable info.

  Copyright (c) 2017 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SPI_FLASH_INFO_GUID_H_
#define SPI_FLASH_INFO_GUID_H_

#include <IndustryStandard/Acpi.h>
//
// SPI Flash infor hob GUID
//
extern EFI_GUID gSpiFlashInfoGuid;

//
// Set this bit if platform need disable SMM write protection when writing flash
// in SMM mode using this method:  -- AsmWriteMsr32 (0x1FE, MmioRead32 (0xFED30880) | BIT0);
//
#define FLAGS_SPI_DISABLE_SMM_WRITE_PROTECT     BIT0

//
// Reuse ACPI definition
//
typedef EFI_ACPI_3_0_GENERIC_ADDRESS_STRUCTURE PLD_GENERIC_ADDRESS;
#define SPACE_ID_PCI_CONFIGURATION              EFI_ACPI_3_0_PCI_CONFIGURATION_SPACE
#define REGISTER_BIT_WIDTH_DWORD                EFI_ACPI_3_0_DWORD

typedef struct {
  UINT8                        Revision;
  UINT8                        Reserved;
  UINT16                       Flags;
  PLD_GENERIC_ADDRESS          SpiAddress;
} SPI_FLASH_INFO;

#endif
