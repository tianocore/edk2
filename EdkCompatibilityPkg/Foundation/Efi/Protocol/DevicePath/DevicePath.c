/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    DevicePath.c

Abstract:

  The device path protocol as defined in EFI 1.0.

  The device path represents a programatic path to a device. It's the view
  from a software point of view. It also must persist from boot to boot, so 
  it can not contain things like PCI bus numbers that change from boot to boot.
  

--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (DevicePath)

EFI_GUID  gEfiDevicePathProtocolGuid = EFI_DEVICE_PATH_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiDevicePathProtocolGuid, "Device Path Protocol", "EFI 1.0 Device Path protocol");
