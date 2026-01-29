/** @file
  This file declares the Install PEI Memory Bins PPI. This PPI must be produced in pre-mem PEI by a platform
  in order to opt in to the PEI memory bins feature. On permanent memory installation, the PEI core will
  look for this PPI, and if found, will search for the Memory Type Information HOB and set up the memory bins
  accordingly. If the PPI is not found, or if the HOB is not found, no memory bins will be set up and memory bins
  will be deferred until DXE.

  This has no associated structure and just is intended as a notification mechanism.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef INSTALL_PEI_MEMORY_BINS_PPI_H_
#define INSTALL_PEI_MEMORY_BINS_PPI_H_

#define INSTALL_PEI_MEMORY_BINS_PPI_GUID \
  { \
    0x24fd4e27, 0x6f4b, 0x4460, {0xb7, 0xa7, 0x21, 0x8b, 0xe8, 0xc3, 0x50, 0xc7} \
  }

extern EFI_GUID  gInstallPeiMemoryBinsPpiGuid;

#endif // INSTALL_PEI_MEMORY_BINS_PPI_H_
