/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  VariableWrite.h

Abstract:

  Variable Write Architectural Protocol as defined in the DXE CIS

  This code is used to produce the EFI 1.0 runtime variable services

  The GetVariable (), GetNextVariableName (), and SetVariable () EFI 1.0 
  services are added to the EFI system table and the 
  EFI_VARIABLE_WRITE_ARCH_PROTOCOL_GUID protocol is registered with a NULL pointer.

  No CRC of the EFI system table is required, as it is done in the DXE core.

--*/

#ifndef _ARCH_PROTOCOL_VARIABLE_WRITE_ARCH_H_
#define _ARCH_PROTOCOL_VARIABLE_WRITE_ARCH_H_

//
// Global ID for the Variable Write Architectural Protocol
//
#define EFI_VARIABLE_WRITE_ARCH_PROTOCOL_GUID \
  { 0x6441f818, 0x6362, 0x4e44, {0xb5, 0x70, 0x7d, 0xba, 0x31, 0xdd, 0x24, 0x53} }

extern EFI_GUID gEfiVariableWriteArchProtocolGuid;

#endif 
