/** @file // [CODE_FIRST] 13226
  This file defines the SPI I/O PPI. // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  This is the PEI-phase analogue of EFI_SPI_IO_PROTOCOL // [CODE_FIRST] 13226
  (MdePkg/Include/Protocol/SpiIo.h). The SPI bus PEIM installs one instance of // [CODE_FIRST] 13226
  this PPI per SPI peripheral, keyed on the peripheral's SpiPeripheralDriverGuid, // [CODE_FIRST] 13226
  so peripheral driver PEIMs (e.g. the SPI NOR flash SFDP PEIM) can perform // [CODE_FIRST] 13226
  managed SPI transactions. // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  The interface is identical to the DXE protocol, so EFI_PEI_SPI_IO_PPI is a // [CODE_FIRST] 13226
  structural alias of EFI_SPI_IO_PROTOCOL. This lets the phase-neutral SPI bus // [CODE_FIRST] 13226
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
#include <Protocol/SpiIo.h> // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
/// // [CODE_FIRST] 13226
/// The SPI I/O PPI has the same interface as EFI_SPI_IO_PROTOCOL. // [CODE_FIRST] 13226
/// // [CODE_FIRST] 13226
typedef EFI_SPI_IO_PROTOCOL EFI_PEI_SPI_IO_PPI; // [CODE_FIRST] 13226
