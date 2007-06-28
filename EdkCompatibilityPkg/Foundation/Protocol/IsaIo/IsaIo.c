/*++

Copyright (c) 2004, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    IsaIo.c
    
Abstract:

    EFI_ISA_IO_PROTOCOL or EFI_LIGHT_ISA_IO_PROTOCOL 
    based on macro SIZE_REDUCTION_ISA_COMBINED.

Revision History

--*/

#include "Tiano.h"

#include EFI_PROTOCOL_DEFINITION (IsaIo)

#ifndef SIZE_REDUCTION_ISA_COMBINED
//
// EFI_ISA_IO_PROTOCOL
//
EFI_GUID  gEfiIsaIoProtocolGuid = EFI_ISA_IO_PROTOCOL_GUID;
EFI_GUID_STRING(&gEfiIsaIoProtocolGuid, "ISA IO Protocol", "ISA IO Protocol");

#else
//
// EFI_LIGHT_ISA_IO_PROTOCOL
//
EFI_GUID  gEfiLightIsaIoProtocolGuid = EFI_LIGHT_ISA_IO_PROTOCOL_GUID;
EFI_GUID_STRING(&gEfiLightIsaIoProtocolGuid, "Light ISA IO Protocol", "Light ISA IO Protocol");
#endif
