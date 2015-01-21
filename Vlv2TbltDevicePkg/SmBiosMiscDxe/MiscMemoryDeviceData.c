/*++

Copyright (c) 2006  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


Module Name:

  MiscMemoryDeviceData.c

Abstract:

   Memory Device
   Misc. subclass type 17.
   SMBIOS type 17.

--*/


#include "CommonHeader.h"
#include "MiscSubclassDriver.h"

MISC_SMBIOS_TABLE_DATA(EFI_MEMORY_ARRAY_LINK_DATA, MiscMemoryDevice) = {
	STRING_TOKEN (STR_MISC_MEM_DEV_LOCATOR0),        // Memory Device locator
	STRING_TOKEN (STR_MISC_MEM_BANK_LOCATOR0),       // Memory Bank Locator
	STRING_TOKEN (STR_MISC_MEM_MANUFACTURER),        // Memory manufacturer
	STRING_TOKEN (STR_MISC_MEM_SERIAL_NO),           // Memory serial Number
	STRING_TOKEN (STR_MISC_MEM_ASSET_TAG),           // Memory Asset Tag
	STRING_TOKEN (STR_MISC_MEM_PART_NUMBER),         // Memory Part Number
	0,											    // Memory Array Link
	0, 					                // Memory SubArray link
	0, 					                // UINT16 MemoryTotalWidth
	0, 					                // UINT16 MemoryDatawidth
	0,										      // Memory Device Size
	EfiMemoryFormFactorDip, 	  // Memory Form Factor
    2,                        // UINT8 Memory Device type
	EfiMemoryTypeDram, 			    // Memory Type
    0,                        // Memory Type Detail
    0,										    // Memory Speed
	0 					                // Memory State

};
