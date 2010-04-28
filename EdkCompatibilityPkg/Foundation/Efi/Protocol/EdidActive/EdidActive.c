/*++

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EdidActive.c

Abstract:

  EDID Active Protocol from the UEFI 2.0 specification.

  Placed on the video output device child handle that are actively displaying output.

--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (EdidActive)

EFI_GUID  gEfiEdidActiveProtocolGuid = EFI_EDID_ACTIVE_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiEdidActiveProtocolGuid, "EFI EDID Active Protocol", "UEFI EDID Active Protocol");
