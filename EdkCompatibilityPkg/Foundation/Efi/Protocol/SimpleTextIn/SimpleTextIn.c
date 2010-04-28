/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  SimpleTextIn.c

Abstract:

  Simple Text In protocol from the EFI 1.0 specification.

  Abstraction of a very simple input device like a keyboard or serial
  terminal.

--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (SimpleTextIn)

EFI_GUID  gEfiSimpleTextInProtocolGuid = EFI_SIMPLE_TEXT_IN_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiSimpleTextInProtocolGuid, "Simple Text In Protocol", "EFI 1.0 Simple Text In Protocol");
