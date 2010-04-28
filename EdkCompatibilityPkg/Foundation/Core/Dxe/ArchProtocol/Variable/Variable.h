/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Variable.h

Abstract:

  Variable Architectural Protocol as defined in the DXE CIS

  This code is used to produce the EFI 1.0 runtime variable services

  The GetVariable (), GetNextVariableName (), and SetVariable () EFI 1.0 
  services are added to the EFI system table and the 
  EFI_VARIABLE_ARCH_PROTOCOL_GUID protocol is registered with a NULL pointer.

  No CRC of the EFI system table is required, as it is done in the DXE core.

--*/

#ifndef _ARCH_PROTOCOL_VARIABLE_ARCH_H_
#define _ARCH_PROTOCOL_VARIABLE_ARCH_H_

//
// Global ID for the Variable Architectural Protocol
//
#define EFI_VARIABLE_ARCH_PROTOCOL_GUID \
  { 0x1e5668e2, 0x8481, 0x11d4, {0xbc, 0xf1, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81} }

extern EFI_GUID gEfiVariableArchProtocolGuid;

#endif 
