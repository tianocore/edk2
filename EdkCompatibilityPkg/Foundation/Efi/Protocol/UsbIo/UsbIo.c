/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    UsbIo.c
    
Abstract:

    EFI Usb I/O Protocol

Revision History

--*/

#include "EfiSpec.h"

#include EFI_PROTOCOL_DEFINITION (UsbIo)

EFI_GUID  gEfiUsbIoProtocolGuid = EFI_USB_IO_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiUsbIoProtocolGuid, "Usb IO Protocol", "EFI 1.1 Usb IO Protocol");
