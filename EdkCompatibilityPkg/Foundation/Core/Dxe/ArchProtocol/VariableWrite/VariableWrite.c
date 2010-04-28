/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  VariableWrite.c

Abstract:

  Variable Write Architectural Protocol as defined in DXE CIS

  This code is used to indicate the EFI 1.0 runtime variable services
  support writting to variables.

--*/

#include "Tiano.h"
#include EFI_ARCH_PROTOCOL_DEFINITION (VariableWrite)

EFI_GUID  gEfiVariableWriteArchProtocolGuid = EFI_VARIABLE_WRITE_ARCH_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiVariableWriteArchProtocolGuid, "VariableWrite", "Variable Write Arch Protocol");
