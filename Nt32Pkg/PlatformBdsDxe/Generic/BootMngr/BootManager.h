/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name: 

  BootManager.h

Abstract:

  The platform boot manager reference implement

Revision History

--*/

#ifndef _EFI_BOOT_MANAGER_H
#define _EFI_BOOT_MANAGER_H

//
// Include common header file for this module.
//
#include "CommonHeader.h"

#include "Generic/Bds.h"
#include "BdsPlatform.h"
#include "Generic/String.h"

EFI_STATUS
EFIAPI
BootManagerCallbackRoutine (
  IN EFI_FORM_CALLBACK_PROTOCOL       *This,
  IN UINT16                           KeyValue,
  IN EFI_IFR_DATA_ARRAY               *DataArray,
  OUT EFI_HII_CALLBACK_PACKET         **Packet
  );

VOID
CallBootManager (
  VOID
);

#define BOOT_MANAGER_GUID \
  { \
    0x847bc3fe, 0xb974, 0x446d, {0x94, 0x49, 0x5a, 0xd5, 0x41, 0x2e, 0x99, 0x3b } \
  }

#endif
