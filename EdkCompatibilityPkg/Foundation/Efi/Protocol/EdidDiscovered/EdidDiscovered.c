/*++

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EdidDiscovered.c

Abstract:

  EDID Discovered Protocol from the UEFI 2.0 specification.

  This protocol is placed on the video output device child handle and it represents
  the EDID information being used for output device represented by the child handle.

--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (EdidDiscovered)

EFI_GUID  gEfiEdidDiscoveredProtocolGuid = EFI_EDID_DISCOVERED_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiEdidDiscoveredProtocolGuid, "EFI EDID Discovered Protocol", "UEFI EDID Discovered Protocol");
