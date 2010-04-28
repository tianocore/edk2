/*++

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EdidOverride.c

Abstract:

  EDID Override Protocol from the UEFI 2.0 specification.

  Allow platform to provide EDID information to producer of the Graphics Output
  protocol.

--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (EdidOverride)

EFI_GUID  gEfiEdidOverrideProtocolGuid = EFI_EDID_OVERRIDE_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiEdidOverrideProtocolGuid, "EFI EDID Override Protocol", "UEFI EDID Override Protocol");
