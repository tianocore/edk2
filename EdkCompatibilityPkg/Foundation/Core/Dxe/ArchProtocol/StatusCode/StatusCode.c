/*++

Copyright (c) 2004 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  StatusCode.c

Abstract:

  Status code Architectural Protocol as defined in Tiano

  This code abstracts Status Code reporting.

--*/

#include "Tiano.h"
#include EFI_ARCH_PROTOCOL_DEFINITION (StatusCode)

EFI_GUID  gEfiStatusCodeRuntimeProtocolGuid = EFI_STATUS_CODE_RUNTIME_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiStatusCodeRuntimeProtocolGuid, "Status Code", "Status Code Arch Protocol");
