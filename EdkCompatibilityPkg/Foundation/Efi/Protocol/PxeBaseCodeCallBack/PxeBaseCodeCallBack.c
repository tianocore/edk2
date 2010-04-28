/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  PxeBaseCodeCallBack.c

Abstract:
  PXE BaseCode Callback GUID declaration.

--*/

#include "EfiSpec.h"

#include EFI_PROTOCOL_DEFINITION (PxeBaseCodeCallBack)

EFI_GUID  gEfiPxeBaseCodeCallbackProtocolGuid = EFI_PXE_BASE_CODE_CALLBACK_PROTOCOL_GUID;

EFI_GUID_STRING
  (&gEfiPxeBaseCodeCallbackProtocolGuid, "PXE Base Code Callback Protocol", "EFI PXE Base Code Callback Protocol");

/* EOF - PxeBaseCodeCallBack.c */
