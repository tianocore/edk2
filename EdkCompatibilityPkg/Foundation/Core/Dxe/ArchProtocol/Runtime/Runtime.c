/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Runtime.c

Abstract:

  Runtime Architectural Protocol as defined in Tiano

  This code is used to produce the EFI 1.0 runtime virtual switch over

--*/

#include "Tiano.h"
#include EFI_ARCH_PROTOCOL_DEFINITION (Runtime)

EFI_GUID  gEfiRuntimeArchProtocolGuid = EFI_RUNTIME_ARCH_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiRuntimeArchProtocolGuid, "Runtime", "Runtime Arch Protocol");
