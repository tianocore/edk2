/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    EfiOEMBadging.c 
    
Abstract:

    EFI OEM Badging Protocol definition source file

Revision History

--*/

#include "Tiano.h"

#include EFI_PROTOCOL_DEFINITION (EfiOemBadging)

EFI_GUID  gEfiOEMBadgingProtocolGuid = EFI_OEM_BADGING_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiOEMBadgingProtocolGuid, "OEM Badging Protocol", "OEM Badging Protocol");
