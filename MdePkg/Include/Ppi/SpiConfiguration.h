/** @file // [CODE_FIRST] 13226
  This file defines the SPI Configuration PPI. // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  This is the PEI-phase analogue of EFI_SPI_CONFIGURATION_PROTOCOL // [CODE_FIRST] 13226
  (MdePkg/Include/Protocol/SpiConfiguration.h). The board layer installs this // [CODE_FIRST] 13226
  PPI to describe the SPI buses and peripherals present on the platform; the SPI // [CODE_FIRST] 13226
  bus PEIM consumes it to enumerate peripherals. // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  The interface is identical to the DXE protocol, so EFI_PEI_SPI_CONFIGURATION_PPI is // [CODE_FIRST] 13226
  a structural alias of EFI_SPI_CONFIGURATION_PROTOCOL, keeping the board // [CODE_FIRST] 13226
  configuration table format identical between PEI and DXE. // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.<BR> // [CODE_FIRST] 13226
  SPDX-License-Identifier: BSD-2-Clause-Patent // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
**/ // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
#pragma once // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
#include <Protocol/DevicePath.h> // [CODE_FIRST] 13226
#include <Protocol/SpiConfiguration.h> // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
/// // [CODE_FIRST] 13226
/// Global ID for the SPI Configuration PPI // [CODE_FIRST] 13226
/// // [CODE_FIRST] 13226
#define EFI_PEI_SPI_CONFIGURATION_PPI_GUID  \ // [CODE_FIRST] 13226
  { 0x2d6a1c74, 0x9b3f, 0x4e58, \ // [CODE_FIRST] 13226
    { 0xa2, 0x0d, 0x71, 0x36, 0x5f, 0x8c, 0x19, 0xb4 }} // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
/// // [CODE_FIRST] 13226
/// The SPI Configuration PPI has the same interface as // [CODE_FIRST] 13226
/// EFI_SPI_CONFIGURATION_PROTOCOL. // [CODE_FIRST] 13226
/// // [CODE_FIRST] 13226
typedef EFI_SPI_CONFIGURATION_PROTOCOL EFI_PEI_SPI_CONFIGURATION_PPI; // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
extern EFI_GUID  gEfiPeiSpiConfigurationPpiGuid; // [CODE_FIRST] 13226
