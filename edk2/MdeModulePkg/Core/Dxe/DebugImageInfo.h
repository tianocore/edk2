/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  DebugImageInfo.h
    
Abstract:

  Support functions for managing debug image info table when loading and unloading
  images.

--*/

#ifndef __DEBUG_IMAGE_INFO_H__
#define __DEBUG_IMAGE_INFO_H__

#define FOUR_MEG_PAGES  0x400  
#define FOUR_MEG_MASK   ((FOUR_MEG_PAGES * EFI_PAGE_SIZE) - 1)

#define EFI_DEBUG_TABLE_ENTRY_SIZE       (sizeof (VOID *))

VOID
CoreInitializeDebugImageInfoTable (
  VOID
  )
/*++

Routine Description:

  Creates and initializes the DebugImageInfo Table.  Also creates the configuration
  table and registers it into the system table.

Arguments:
  None

Returns:
  NA

Notes:
  This function allocates memory, frees it, and then allocates memory at an
  address within the initial allocation. Since this function is called early
  in DXE core initialization (before drivers are dispatched), this should not
  be a problem.

--*/
;

VOID
CoreUpdateDebugTableCrc32 (
  VOID
  )
/*++

Routine Description:

  Update the CRC32 in the Debug Table.
  Since the CRC32 service is made available by the Runtime driver, we have to
  wait for the Runtime Driver to be installed before the CRC32 can be computed.
  This function is called elsewhere by the core when the runtime architectural
  protocol is produced.

Arguments:
  None

Returns:
  NA

--*/
;

VOID
CoreNewDebugImageInfoEntry (
  UINT32                    ImageInfoType,
  EFI_LOADED_IMAGE_PROTOCOL *LoadedImage,
  EFI_HANDLE                ImageHandle
  )
/*++

Routine Description:

  Adds a new DebugImageInfo structure to the DebugImageInfo Table.  Re-Allocates
  the table if it's not large enough to accomidate another entry.

Arguments:

  ImageInfoType     - type of debug image information
  LoadedImage       - pointer to the loaded image protocol for the image being loaded
  ImageHandle       - image handle for the image being loaded

Returns:
  NA

--*/
;

VOID
CoreRemoveDebugImageInfoEntry (
  EFI_HANDLE ImageHandle
  )
/*++

Routine Description:

  Removes and frees an entry from the DebugImageInfo Table.

Arguments:

  ImageHandle       - image handle for the image being unloaded

Returns:

  NA

--*/
;

#endif
