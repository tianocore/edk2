/** @file // [CODE_FIRST] 13226
  This file defines the SPI NOR Flash PPI. // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  This is the PEI-phase analogue of EFI_SPI_NOR_FLASH_PROTOCOL // [CODE_FIRST] 13226
  (MdePkg/Include/Protocol/SpiNorFlash.h). The SPI NOR flash SFDP PEIM installs // [CODE_FIRST] 13226
  this PPI to expose a generic NOR flash interface (read/write/erase/status) to // [CODE_FIRST] 13226
  other PEIMs that need SPI flash access before the DXE phase. // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  The interface is identical to the DXE protocol, so EFI_PEI_SPI_NOR_FLASH_PPI is a // [CODE_FIRST] 13226
  structural alias of EFI_SPI_NOR_FLASH_PROTOCOL, letting the phase-neutral SFDP // [CODE_FIRST] 13226
  code be shared with the DXE/SMM drivers without duplication. // [CODE_FIRST] 13226
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
#include <Protocol/SpiNorFlash.h> // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
/// // [CODE_FIRST] 13226
/// Global ID for the SPI NOR Flash PPI // [CODE_FIRST] 13226
/// // [CODE_FIRST] 13226
#define EFI_PEI_SPI_NOR_FLASH_PPI_GUID  \ // [CODE_FIRST] 13226
  { 0x0b9e6f3a, 0x4c72, 0x4f9d, \ // [CODE_FIRST] 13226
    { 0x8e, 0x53, 0x1a, 0x6b, 0x27, 0xd4, 0x90, 0x3c }} // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
/// // [CODE_FIRST] 13226
/// The SPI NOR Flash PPI has the same interface as // [CODE_FIRST] 13226
/// EFI_SPI_NOR_FLASH_PROTOCOL. // [CODE_FIRST] 13226
/// // [CODE_FIRST] 13226
typedef EFI_SPI_NOR_FLASH_PROTOCOL EFI_PEI_SPI_NOR_FLASH_PPI; // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
extern EFI_GUID  gEfiPeiSpiNorFlashPpiGuid; // [CODE_FIRST] 13226
