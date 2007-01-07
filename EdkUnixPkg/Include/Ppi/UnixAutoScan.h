/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  UnixAutoscan.h

Abstract:

Unix Autoscan PPI

--*/

#ifndef __UNIX_PEI_AUTOSCAN_H__
#define __UNIX_PEI_AUTOSCAN_H__

#include <UnixDxe.h>

#define PEI_UNIX_AUTOSCAN_PPI_GUID \
  { \
    0xf2ed3d14, 0x8985, 0x11db, {0xb0, 0x57, 0x00, 0x40, 0xd0, 0x2b, 0x18, 0x35 } \
  }

typedef
EFI_STATUS
(EFIAPI *PEI_UNIX_AUTOSCAN) (
  IN  UINTN                 Index,
  OUT EFI_PHYSICAL_ADDRESS  *MemoryBase,
  OUT UINT64                *MemorySize
  );

/*++

Routine Description:
  This service is called from Index == 0 until it returns EFI_UNSUPPORTED.
  It allows discontiguous memory regions to be supported by the emulator.
  It uses gSystemMemory[] and gSystemMemoryCount that were created by
  parsing the host environment variable EFI_MEMORY_SIZE.
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
  PEI_UNIX_AUTOSCAN UnixAutoScan;
} PEI_UNIX_AUTOSCAN_PPI;

extern EFI_GUID gPeiUnixAutoScanPpiGuid;

#endif
