/*++

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED. 

Module Name:

  NicIp4Config.c

Abstract:

--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (NicIp4Config)

EFI_GUID gEfiNicIp4ConfigProtocolGuid = EFI_NIC_IP4_CONFIG_PROTOCOL_GUID;

EFI_GUID_STRING (
  &gEfiNicIp4ConfigProtocolGuid, 
  "NicIP4Config Protocol",  
  "NicIP4Config Protocol"
  );

EFI_GUID gEfiNicIp4ConfigVariableGuid  = EFI_NIC_IP4_CONFIG_VARIABLE_GUID;

EFI_GUID_STRING(
  &gEfiNicIp4ConfigVariableGuid, 
  "Ip4 Static Config", 
  "Ip4 Configuration Data"
  );
