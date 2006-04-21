/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  SecurityStub.h

Abstract:

  Some definitions for Security Architectural Protocol stub driver

--*/

#ifndef _SECURITY_STUB_ARCH_PROTOCOL_H
#define _SECURITY_STUB_ARCH_PROTOCOL_H



//
// Function prototypes
//
EFI_STATUS
EFIAPI
SecurityStubAuthenticateState (
  IN EFI_SECURITY_ARCH_PROTOCOL          *This,
  IN UINT32                              AuthenticationStatus,
  IN  EFI_DEVICE_PATH_PROTOCOL           *File
  )
;

EFI_STATUS
EFIAPI
SecurityStubInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
;

#endif
