/*++

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED. 

Module Name:

  Udp4.c

Abstract:

  UEFI UDPv4 protocol.

--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (Udp4)

EFI_GUID gEfiUdp4ServiceBindingProtocolGuid = EFI_UDP4_SERVICE_BINDING_PROTOCOL_GUID;
EFI_GUID gEfiUdp4ProtocolGuid               = EFI_UDP4_PROTOCOL_GUID;

EFI_GUID_STRING (&gEfiUdp4ServiceBindingProtocolGuid, "UDP4 Service Binding Protocol", "UDP4 Service Binding Protocol");
EFI_GUID_STRING (&gEfiUdp4ProtocolGuid, "UDP4 Protocol", "UDP4 Protocol");
