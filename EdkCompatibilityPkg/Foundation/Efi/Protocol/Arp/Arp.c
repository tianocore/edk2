/*++

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED. 

Module Name:

  Arp.c

Abstract:

  UEFI Arp protocol.

--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (Arp)

EFI_GUID gEfiArpServiceBindingProtocolGuid  = EFI_ARP_SERVICE_BINDING_PROTOCOL_GUID;
EFI_GUID gEfiArpProtocolGuid                = EFI_ARP_PROTOCOL_GUID;

EFI_GUID_STRING (&gEfiArpServiceBindingProtocolGuid, "ARP Service Binding Protocol", "ARP Service Binding Protocol");
EFI_GUID_STRING (&gEfiArpProtocolGuid,               "ARP Protocol",                 "ARP Protocol");
