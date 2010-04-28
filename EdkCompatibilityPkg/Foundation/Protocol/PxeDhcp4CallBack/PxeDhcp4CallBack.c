/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  PxeDhcp4Callback.c

Abstract:
  PxeDhcp4Callback protocol GUID definition.

--*/

#include "Tiano.h"

#include EFI_PROTOCOL_DEFINITION (PxeDhcp4CallBack)

EFI_GUID  gEfiPxeDhcp4CallbackProtocolGuid = EFI_PXE_DHCP4_CALLBACK_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiPxeDhcp4CallbackProtocolGuid, "PXE DHCP4 Callback Protocol", "PXE DHCP IPv4 Callback Protocol");

/* EOF - PxeDhcp4Callback.c */
