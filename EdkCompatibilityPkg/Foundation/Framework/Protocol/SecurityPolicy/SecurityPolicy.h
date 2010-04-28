/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  SecurityPolicy.h

Abstract:

  Security Policy protocol as defined in the DXE CIS

--*/

#ifndef _SECURITY_POLICY_H_
#define _SECURITY_POLICY_H_

//
// Security policy protocol GUID definition
//
#define EFI_SECURITY_POLICY_PROTOCOL_GUID  \
  {0x78E4D245, 0xCD4D, 0x4a05, {0xA2, 0xBA, 0x47, 0x43, 0xE8, 0x6C, 0xFC, 0xAB}}

extern EFI_GUID gEfiSecurityPolicyProtocolGuid;

#endif
