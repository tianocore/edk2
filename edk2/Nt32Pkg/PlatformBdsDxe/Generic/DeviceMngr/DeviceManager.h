/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name: 

  DeviceManager.c

Abstract:

  The platform device manager reference implement

Revision History

--*/

#ifndef _DEVICE_MANAGER_H
#define _DEVICE_MANAGER_H

#include "Generic/FrontPage.h"

#define EFI_NON_DEVICE_CLASS              0x00  // Useful when you do not want something in the Device Manager
#define EFI_DISK_DEVICE_CLASS             0x01
#define EFI_VIDEO_DEVICE_CLASS            0x02
#define EFI_NETWORK_DEVICE_CLASS          0x04
#define EFI_INPUT_DEVICE_CLASS            0x08
#define EFI_ON_BOARD_DEVICE_CLASS         0x10
#define EFI_OTHER_DEVICE_CLASS            0x20

EFI_STATUS
EFIAPI
DeviceManagerCallbackRoutine (
  IN EFI_FORM_CALLBACK_PROTOCOL       *This,
  IN UINT16                           KeyValue,
  IN FRAMEWORK_EFI_IFR_DATA_ARRAY               *DataArray,
  OUT EFI_HII_CALLBACK_PACKET         **Packet
  )
;

EFI_STATUS
InitializeDeviceManager (
  VOID
  )
;

EFI_STATUS
CallDeviceManager (
  VOID
  )
;

#endif
