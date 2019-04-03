/*++

Copyright (c) 2006  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   


Module Name:

  MiscPhysicalArrayData.c

Abstract:

  BIOS Physical Array static data.
  SMBIOS type 16.

--*/


#include "CommonHeader.h"

#include "MiscSubclassDriver.h"



//
// Static (possibly build generated) Physical Memory Array Dat.
//
MISC_SMBIOS_TABLE_DATA(EFI_MEMORY_ARRAY_LOCATION_DATA, MiscPhysicalMemoryArray) =
{
	EfiMemoryArrayLocationSystemBoard,						// Memory location
	EfiMemoryArrayUseSystemMemory, 						    // Memory array use
	EfiMemoryErrorCorrectionNone, 						    // Memory error correction
	0,                                                      // Maximum Memory Capacity
	0x01						                            // Number of Devices
};
