/** @file
  Support functions for managing debug image info table when loading and unloading
  images.

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __DEBUG_IMAGE_INFO_H__
#define __DEBUG_IMAGE_INFO_H__

#define FOUR_MEG_ALIGNMENT   0x400000

#define EFI_DEBUG_TABLE_ENTRY_SIZE       (sizeof (VOID *))


/**
  Creates and initializes the DebugImageInfo Table.  Also creates the configuration
  table and registers it into the system table.

  Note:
    This function allocates memory, frees it, and then allocates memory at an
    address within the initial allocation. Since this function is called early
    in DXE core initialization (before drivers are dispatched), this should not
    be a problem.

**/
VOID
CoreInitializeDebugImageInfoTable (
  VOID
  );


/**
  Update the CRC32 in the Debug Table.
  Since the CRC32 service is made available by the Runtime driver, we have to
  wait for the Runtime Driver to be installed before the CRC32 can be computed.
  This function is called elsewhere by the core when the runtime architectural
  protocol is produced.

**/
VOID
CoreUpdateDebugTableCrc32 (
  VOID
  );


/**
  Adds a new DebugImageInfo structure to the DebugImageInfo Table.  Re-Allocates
  the table if it's not large enough to accomidate another entry.

  @param  ImageInfoType  type of debug image information
  @param  LoadedImage    pointer to the loaded image protocol for the image being
                         loaded
  @param  ImageHandle    image handle for the image being loaded

**/
VOID
CoreNewDebugImageInfoEntry (
  IN  UINT32                      ImageInfoType,
  IN  EFI_LOADED_IMAGE_PROTOCOL   *LoadedImage,
  IN  EFI_HANDLE                  ImageHandle
  );


/**
  Removes and frees an entry from the DebugImageInfo Table.

  @param  ImageHandle    image handle for the image being unloaded

**/
VOID
CoreRemoveDebugImageInfoEntry (
  EFI_HANDLE ImageHandle
  );

#endif
