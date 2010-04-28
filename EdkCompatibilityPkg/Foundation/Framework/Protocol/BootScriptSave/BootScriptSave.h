/*++

Copyright (c) 1999 - 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  BootScriptSave.h

Abstract:

  S3 Save Protocol
  
--*/

#ifndef _BOOT_SCRIPT_SAVE_PROTOCOL_H
#define _BOOT_SCRIPT_SAVE_PROTOCOL_H

//
// Includes
//
#include "Tiano.h"

//
// Forward reference for pure ANSI compatability
//
EFI_FORWARD_DECLARATION (EFI_BOOT_SCRIPT_SAVE_PROTOCOL);

//
// S3 Save Protocol GUID
//
#define EFI_BOOT_SCRIPT_SAVE_PROTOCOL_GUID \
  { \
    0x470e1529, 0xb79e, 0x4e32, {0xa0, 0xfe, 0x6a, 0x15, 0x6d, 0x29, 0xf9, 0xb2} \
  }

//
// Protocol Data Structures
//
typedef
EFI_STATUS
(EFIAPI *EFI_BOOT_SCRIPT_WRITE) (
  IN EFI_BOOT_SCRIPT_SAVE_PROTOCOL            * This,
  IN UINT16                                   TableName,
  IN UINT16                                   OpCode,
  ...
  );

typedef
EFI_STATUS
(EFIAPI *EFI_BOOT_SCRIPT_CLOSE_TABLE) (
  IN EFI_BOOT_SCRIPT_SAVE_PROTOCOL            * This,
  IN UINT16                                   TableName,
  OUT EFI_PHYSICAL_ADDRESS                    * Address
  );

//
// S3 Save Protocol data structure
//
struct _EFI_BOOT_SCRIPT_SAVE_PROTOCOL {
  EFI_BOOT_SCRIPT_WRITE       Write;
  EFI_BOOT_SCRIPT_CLOSE_TABLE CloseTable;
};

extern EFI_GUID gEfiBootScriptSaveGuid;

#endif
