/*++

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED. 

Module Name:

  Dhcp4.c

Abstract:

  UEFI Dynamic Host Configuration Protocol 4.

--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (Dhcp4)

EFI_GUID gEfiDhcp4ServiceBindingProtocolGuid 
           = EFI_DHCP4_SERVICE_BINDING_PROTOCOL_GUID;

EFI_GUID gEfiDhcp4ProtocolGuid             
           = EFI_DHCP4_PROTOCOL_GUID;

EFI_GUID_STRING (
  &gEfiDhcp4ServiceBindingProtocolGuid, 
  "DHCP4 Service Binding Protocol", 
  "DHCP4 Service Binding Protocol"
  );

EFI_GUID_STRING (
  &gEfiDhcp4ProtocolGuid,             
  "DHCP4 Protocol",             
  "DHCP4 Protocol"
  );
