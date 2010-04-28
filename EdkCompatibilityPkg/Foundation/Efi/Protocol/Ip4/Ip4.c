/*++

Copyright (c) 2005 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED. 


Module Name:

  Ip4.c

Abstract:

  UEFI IPv4 protocol.

--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (Ip4)

EFI_GUID gEfiIp4ServiceBindingProtocolGuid 
           = EFI_IP4_SERVICE_BINDING_PROTOCOL_GUID;

EFI_GUID gEfiIp4ProtocolGuid = EFI_IP4_PROTOCOL_GUID;

EFI_GUID_STRING (
  &gEfiIp4ServiceBindingProtocolGuid, 
  "IP4 Service Binding Protocol", 
  "IP4 Service Binding Protocol"
  );

EFI_GUID_STRING (
  &gEfiIp4ProtocolGuid,               
  "IP4 Protocol",                 
  "IP4 Protocol"
  );
