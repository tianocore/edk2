/**@file

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

  NtAutoscan.h

Abstract:

Nt Autoscan PPI

**/

#ifndef __NT_PEI_AUTOSCAN_H__
#define __NT_PEI_AUTOSCAN_H__

#include <WinNtDxe.h>

#define PEI_NT_AUTOSCAN_PPI_GUID \
  { \
    0xdce384d, 0x7c, 0x4ba5, {0x94, 0xbd, 0xf, 0x6e, 0xb6, 0x4d, 0x2a, 0xa9 } \
  }

typedef
EFI_STATUS
(EFIAPI *PEI_NT_AUTOSCAN) (
  IN  UINTN                 Index,
  OUT EFI_PHYSICAL_ADDRESS  * MemoryBase,
  OUT UINT64                *MemorySize
  );

/*++

Routine Description:
  This service is called from Index == 0 until it returns EFI_UNSUPPORTED.
  It allows discontinuous memory regions to be supported by the emulator.
  It uses gSystemMemory[] and gSystemMemoryCount that were created by
  parsing the Windows environment variable EFI_MEMORY_SIZE.
  The size comes from the varaible and the address comes from the call to
  WinNtOpenFile. 

Arguments:
  Index      - Which memory region to use
  MemoryBase - Return Base address of memory region
  MemorySize - Return size in bytes of the memory region

Returns:
  EFI_SUCCESS - If memory region was mapped
  EFI_UNSUPPORTED - If Index is not supported

--*/
typedef struct {
  PEI_NT_AUTOSCAN NtAutoScan;
} PEI_NT_AUTOSCAN_PPI;

extern EFI_GUID gPeiNtAutoScanPpiGuid;

#endif
