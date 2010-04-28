/*++

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED. 

Module Name:

  Ip4Config.c

Abstract:

--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (Ip4Config)

EFI_GUID gEfiIp4ConfigProtocolGuid  = EFI_IP4_CONFIG_PROTOCOL_GUID;

EFI_GUID_STRING (
  &gEfiIp4ConfigProtocolGuid, 
  "Ip4Config Protocol",                 
  "Ip4Config Protocol"
  );
