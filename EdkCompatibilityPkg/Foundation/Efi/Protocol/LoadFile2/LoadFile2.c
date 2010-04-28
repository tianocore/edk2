/*++

Copyright (c) 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  LoadFile2.c

Abstract:

  Load File2 protocol as defined in the UEFI specification.

  Load File2 protocol exists to support to obtain files from arbitrary devices
  but are not used as boot options.

--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (LoadFile2)

EFI_GUID  gEfiLoadFile2ProtocolGuid = EFI_LOAD_FILE2_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiLoad2FileProtocolGuid, "LoadFile2 Protocol", "UEFI 2.1 Load File2 Protocol");
