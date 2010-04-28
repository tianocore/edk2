/*++

Copyright (c) 2005 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED. 

Module Name:

  Tcp4.c

Abstract:

  UEFI TCPv4 Protocol

--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (Tcp4)

EFI_GUID  gEfiTcp4ServiceBindingProtocolGuid = EFI_TCP4_SERVICE_BINDING_PROTOCOL_GUID;
EFI_GUID  gEfiTcp4ProtocolGuid               = EFI_TCP4_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiTcp4ServiceBindingProtocolGuid, "TCP4 Service Binding Protocol", "TCP4 Service Binding Protocol");
EFI_GUID_STRING(&gEfiTcp4ProtocolGuid, "TCP4 Protocol", "TCP4 Protocol");

