/*++

Copyright (c) 2006  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


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
