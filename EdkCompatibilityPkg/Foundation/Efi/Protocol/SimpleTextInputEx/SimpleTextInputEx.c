/*++

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  SimpleTextInputEx.c

Abstract:

  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL from the UEFI 2.1 specification.

  This protocol defines an extension to the EFI_SIMPLE_TEXT_INPUT_PROTOCOL
  which exposes much more state and modifier information from the input device,
  also allows one to register a notification for a particular keystroke.

--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (SimpleTextInputEx)

EFI_GUID  gEfiSimpleTextInputExProtocolGuid = EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiSimpleTextInputExProtocolGuid, "Simple Text Input Ex Protocol", "UEFI 2.1 Simple Text Input Ex Protocol");
