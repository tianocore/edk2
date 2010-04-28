/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  DeviceIo.c

Abstract:

  Device IO protocol as defined in the EFI 1.0 specification.

  Device IO is used to abstract hardware access to devices. It includes
  memory mapped IO, IO, PCI Config space, and DMA.

 
--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (DeviceIo)

EFI_GUID  gEfiDeviceIoProtocolGuid = EFI_DEVICE_IO_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiDeviceIoProtocolGuid, "DeviceIo Protocol", "EFI 1.0 Device IO Protocol");
