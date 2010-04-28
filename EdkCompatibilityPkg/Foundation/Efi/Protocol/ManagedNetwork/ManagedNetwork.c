/*++

Copyright (c) 2005 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  ManagedNetwork.c

Abstract:

  UEFI Managed Network protocol.

--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (ManagedNetwork)

EFI_GUID gEfiManagedNetworkServiceBindingProtocolGuid = EFI_MANAGED_NETWORK_SERVICE_BINDING_PROTOCOL_GUID;
EFI_GUID gEfiManagedNetworkProtocolGuid               = EFI_MANAGED_NETWORK_PROTOCOL_GUID;

EFI_GUID_STRING (&gEfiManagedNetworkServiceBindingProtocolGuid, "Managed Network Service Binding Protocol", "Managed Network Service Binding Protocol");
EFI_GUID_STRING (&gEfiManagedNetworkProtocolGuid, "Managed Network Protocol", "Managed Network Protocol");
