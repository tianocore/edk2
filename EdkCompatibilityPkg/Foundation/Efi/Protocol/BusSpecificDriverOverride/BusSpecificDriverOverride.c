/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  BusSpecificDriverOverride.c

Abstract:

  Bus Specific Driver Override protocol as defined in the EFI 1.1 specification.

--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (BusSpecificDriverOverride)

EFI_GUID  gEfiBusSpecificDriverOverrideProtocolGuid = EFI_BUS_SPECIFIC_DRIVER_OVERRIDE_PROTOCOL_GUID;

EFI_GUID_STRING
  (
    &gEfiBusSpecificDriverOverrideProtocolGuid, "Bus Specific Driver Override Protocol",
      "EFI 1.1 Bus Specific Driver Override Protocol"
  );
