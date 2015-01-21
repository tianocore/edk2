/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   



Module Name:

  MemoryConfigData.h

Abstract:

  GUID used for Memory Configuration Data entries in the HOB list.

--*/

#ifndef _MEMORY_CONFIG_DATA_GUID_H_
#define _MEMORY_CONFIG_DATA_GUID_H_

#define EFI_MEMORY_CONFIG_DATA_GUID \
  { \
    0x80dbd530, 0xb74c, 0x4f11, 0x8c, 0x03, 0x41, 0x86, 0x65, 0x53, 0x28, 0x31 \
  }

extern EFI_GUID gEfiMemoryConfigDataGuid;
extern CHAR16   EfiMemoryConfigVariable[];

#endif
