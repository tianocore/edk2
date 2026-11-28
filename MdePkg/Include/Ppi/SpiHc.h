/** @file // [CODE_FIRST] 13226
  This file defines the SPI Host Controller PPI. // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  This is the PEI-phase analogue of EFI_SPI_HC_PROTOCOL // [CODE_FIRST] 13226
  (MdePkg/Include/Protocol/SpiHc.h). The interface (Attributes, // [CODE_FIRST] 13226
  FrameSizeSupportMask, MaximumTransferBytes, ChipSelect, Clock, Transaction) is // [CODE_FIRST] 13226
  identical to the DXE protocol, so EFI_PEI_SPI_HC_PPI is a structural alias of // [CODE_FIRST] 13226
  EFI_SPI_HC_PROTOCOL. This lets the phase-neutral SPI host controller code be // [CODE_FIRST] 13226
  shared between the DXE/SMM drivers and the PEIM without duplication; only the // [CODE_FIRST] 13226
  installed GUID differs (gEfiPeiSpiHcPpiGuid vs gEfiSpiHcProtocolGuid). // [CODE_FIRST] 13226
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
#include <Protocol/SpiIo.h> // [CODE_FIRST] 13226
#include <Protocol/SpiHc.h> // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
/// // [CODE_FIRST] 13226
/// Global ID for the SPI Host Controller PPI // [CODE_FIRST] 13226
/// // [CODE_FIRST] 13226
#define EFI_PEI_SPI_HC_PPI_GUID  \ // [CODE_FIRST] 13226
  { 0x8f8f8b8e, 0x6c2a, 0x4d3e, \ // [CODE_FIRST] 13226
    { 0x9a, 0x11, 0x2b, 0x7c, 0x5d, 0x0e, 0x84, 0x21 }} // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
/// // [CODE_FIRST] 13226
/// The SPI Host Controller PPI has the same interface as // [CODE_FIRST] 13226
/// EFI_SPI_HC_PROTOCOL. // [CODE_FIRST] 13226
/// // [CODE_FIRST] 13226
typedef EFI_SPI_HC_PROTOCOL EFI_PEI_SPI_HC_PPI; // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
extern EFI_GUID  gEfiPeiSpiHcPpiGuid; // [CODE_FIRST] 13226
