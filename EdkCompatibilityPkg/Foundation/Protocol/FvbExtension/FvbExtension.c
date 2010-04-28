/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    FvbExtension.c

Abstract:

  IPMI Extension Protocol to extend the FVB functionality to multiple erase blocks
  at different block offsets.


--*/

#include "Tiano.h"
#include EFI_PROTOCOL_DEFINITION(FvbExtension)

EFI_GUID gEfiFvbExtensionProtocolGuid = EFI_FVB_EXTENSION_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiFvbExtensionProtocolGuid, "FVB EXTENSION", "FVB EXTENSION");
