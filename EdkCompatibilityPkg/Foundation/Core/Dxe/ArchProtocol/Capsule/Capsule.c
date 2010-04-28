/*++

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Capsule.c

Abstract:

  Capsule Architectural Protocol is used to produce the UEFI 2.0 capsule runtime services

--*/

#include "Tiano.h"
#include EFI_ARCH_PROTOCOL_DEFINITION (Capsule)

EFI_GUID  gEfiCapsuleArchProtocolGuid = EFI_CAPSULE_ARCH_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiCapsuleArchProtocolGuid, "Capsule", "Capsule Arch Protocol");
