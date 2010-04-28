/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  SecurityPolicy.c

Abstract:

  Security Policy protocol as defined in the DXE CIS

--*/

#include "Tiano.h"                  
#include EFI_PROTOCOL_DEFINITION (SecurityPolicy)

EFI_GUID gEfiSecurityPolicyProtocolGuid = EFI_SECURITY_POLICY_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiSecurityPolicyProtocolGuid, "Security Policy protocol", "Security Policy protocol");

