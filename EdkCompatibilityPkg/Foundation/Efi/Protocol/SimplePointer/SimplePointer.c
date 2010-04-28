/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  SimplePointer.c

Abstract:

  Simple Pointer protocol from the EFI 1.1 specification.

  Abstraction of a very simple pointer device like a mice or trackballs.

--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (SimplePointer)

EFI_GUID  gEfiSimplePointerProtocolGuid = EFI_SIMPLE_POINTER_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiSimplePointerProtocolGuid, "Simple Pointer Protocol", "EFI 1.1 Simple Pointer Protocol");
