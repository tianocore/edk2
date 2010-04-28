/*++

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  AbsolutePointer.c

Abstract:

  EFI_ABSOLUTE_POINTER_PROTOCOL from the UEFI 2.1 specification.

  This protocol specifies a simple method for accessing absolute pointer devices.  
  
--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (AbsolutePointer)

EFI_GUID  gEfiAbsolutePointerProtocolGuid = EFI_ABSOLUTE_POINTER_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiAbsolutePointerProtocolGuid, "Absolute Pointer Protocol", "UEFI 2.1 Absolute Pointer Protocol");
