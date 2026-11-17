/** @file // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  Internal types for the SPI bus PEIM. // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
  Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.<BR> // [CODE_FIRST] 13226
  SPDX-License-Identifier: BSD-2-Clause-Patent // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
**/ // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
#pragma once // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
#include "SpiBus.h" // [CODE_FIRST] 13226
 // [CODE_FIRST] 13226
// // [CODE_FIRST] 13226
// PEI-only wrapper: bundles the shared chip instance with the PPI descriptor // [CODE_FIRST] 13226
// in a single allocation so the descriptor lifetime matches the chip object. // [CODE_FIRST] 13226
// EFI_PEI_PPI_DESCRIPTOR is not placed in SPI_IO_CHIP because DXE/SMM drivers // [CODE_FIRST] 13226
// publish via InstallProtocolInterface and have no use for it. // [CODE_FIRST] 13226
// // [CODE_FIRST] 13226
typedef struct { // [CODE_FIRST] 13226
  SPI_IO_CHIP               Chip; // [CODE_FIRST] 13226
  EFI_PEI_PPI_DESCRIPTOR    PpiDescriptor; // [CODE_FIRST] 13226
} SPI_IO_CHIP_PEI; // [CODE_FIRST] 13226
