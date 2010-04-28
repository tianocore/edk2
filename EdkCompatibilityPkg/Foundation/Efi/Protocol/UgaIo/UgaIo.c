/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  UgaIo.c

Abstract:

  UGA IO protocol from the EFI 1.1 specification.

  Abstraction of a very simple graphics device.

--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (UgaIo)

EFI_GUID  gEfiUgaIoProtocolGuid = EFI_UGA_IO_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiUgaIoProtocolGuid, "UGA Protocol", "EFI 1.1 UGA Protocol");
