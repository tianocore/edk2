/*++

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  GraphicsOutput.c

Abstract:

  Graphics Output Protocol from the UEFI 2.0 specification.

  Abstraction of a very simple graphics device.

--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (GraphicsOutput)

EFI_GUID  gEfiGraphicsOutputProtocolGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiGraphicsOutputProtocolGuid, "EFI Graphics Output Protocol", "UEFI Graphics Output Protocol");
