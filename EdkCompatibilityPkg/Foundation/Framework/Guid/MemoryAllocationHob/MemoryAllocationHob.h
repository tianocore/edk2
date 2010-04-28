/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    MemoryAllocationHob.h
    
Abstract:

  GUIDs for HOBs used in memory allcation

--*/

#ifndef _HOB__MEMORY_ALLOCATION_GUID_H_
#define _HOB__MEMORY_ALLOCATION_GUID_H_

#define EFI_HOB_MEMORY_ALLOC_BSP_STORE_GUID  \
  {0x564b33cd, 0xc92a, 0x4593, {0x90, 0xbf, 0x24, 0x73, 0xe4, 0x3c, 0x63, 0x22}};

#define EFI_HOB_MEMORY_ALLOC_STACK_GUID  \
  {0x4ed4bf27, 0x4092, 0x42e9, {0x80, 0x7d, 0x52, 0x7b, 0x1d, 0x0, 0xc9, 0xbd}}

#define EFI_HOB_MEMORY_ALLOC_MODULE_GUID  \
  {0xf8e21975, 0x899, 0x4f58, {0xa4, 0xbe, 0x55, 0x25, 0xa9, 0xc6, 0xd7, 0x7a}}

extern EFI_GUID gEfiHobMemeryAllocBspStoreGuid;
extern EFI_GUID gEfiHobMemeryAllocStackGuid;
extern EFI_GUID gEfiHobMemeryAllocModuleGuid;

#endif
