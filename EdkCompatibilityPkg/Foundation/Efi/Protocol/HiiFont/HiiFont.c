/*++

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    HiiFont.c
    
Abstract:

    EFI_HII_FONT_PROTOCOL

Revision History

--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (HiiFont)

EFI_GUID  gEfiHiiFontProtocolGuid = EFI_HII_FONT_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiHiiFontProtocolGuid, "EFI HII FONT Protocol", "UEFI 2.1 HII FONT Protocol");
