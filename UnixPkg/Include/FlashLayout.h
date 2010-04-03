/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  FlashLayout.h
   
Abstract:

  Platform specific flash layout

  BugBug: We need a better way

--*/

#ifndef __EFI_FLASH_LAYOUT__
#define __EFI_FLASH_LAYOUT__

//
// Firmware Volume Information for Nt32
// adding one working block before FFS FV,
// and another one for spare block behind FFS FV
//
//
// Note: When block number is changed in .dsc file,
//       this value should be changed accordingly!!!
//
#define FIRMWARE_BLOCK_NUMBER                         0x28

#define EFI_WINNT_FIRMWARE_OFFSET                     0x0
#define EFI_WINNT_FIRMWARE_LENGTH                     (0x10000 * FIRMWARE_BLOCK_NUMBER)

#define EFI_WINNT_RUNTIME_UPDATABLE_OFFSET            (EFI_WINNT_FIRMWARE_OFFSET + EFI_WINNT_FIRMWARE_LENGTH)

#define EFI_WINNT_RUNTIME_UPDATABLE_LENGTH            0x10000

#define EFI_WINNT_FTW_SPARE_BLOCK_OFFSET              (EFI_WINNT_RUNTIME_UPDATABLE_OFFSET + EFI_WINNT_RUNTIME_UPDATABLE_LENGTH)

#define EFI_WINNT_FTW_SPARE_BLOCK_LENGTH              0x10000

#define EFI_WINNT_RUNTIME_UPDATABLE_FV_HEADER_LENGTH  0x48

#define EFI_VARIABLE_STORE_OFFSET                     EFI_WINNT_RUNTIME_UPDATABLE_OFFSET

#define EFI_VARIABLE_STORE_LENGTH                     0x00C000

#define EFI_EVENT_LOG_OFFSET                          (EFI_VARIABLE_STORE_OFFSET + EFI_VARIABLE_STORE_LENGTH)

#define EFI_EVENT_LOG_LENGTH                          0x002000

#define EFI_FTW_WORKING_OFFSET                        (EFI_EVENT_LOG_OFFSET + EFI_EVENT_LOG_LENGTH)

#define EFI_FTW_WORKING_LENGTH                        0x002000

#endif

