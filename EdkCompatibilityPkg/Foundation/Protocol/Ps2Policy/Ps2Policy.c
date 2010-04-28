/*++

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  Ps2Policy.c
    
Abstract:

  Protocol used for PS/2 Policy definition.

--*/

#include "Tiano.h"
#include EFI_PROTOCOL_DEFINITION (Ps2Policy)

EFI_GUID  gEfiPs2PolicyProtocolGuid = EFI_PS2_POLICY_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiPs2PolicyProtocolGuid, "PS2 Policy", "Policy for Configuring PS2");
