/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  PxeDhcp4.c

Abstract:
  PxeDhcp4 GUID declaration

--*/

#include "Tiano.h"

#include EFI_PROTOCOL_DEFINITION (PxeDhcp4)

EFI_GUID  gEfiPxeDhcp4ProtocolGuid = EFI_PXE_DHCP4_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiPxeDhcp4ProtocolGuid, "PXE DHCP4 Protocol", "PXE DHCPv4 Protocol");

/* EOF - PxeDhcp4.c */
