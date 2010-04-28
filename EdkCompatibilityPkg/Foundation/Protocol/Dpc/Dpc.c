/*++

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED. 

Module Name:

  Dpc.c

Abstract:

  EFI Deferred Procedure Call Protocol

--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (Dpc)

EFI_GUID gEfiDpcProtocolGuid = EFI_DPC_PROTOCOL_GUID;

EFI_GUID_STRING (&gEfiDpcProtocolGuid, "EFI Deferred Procedure Call Protocol", "EFI Deferred Procedure Call Protocol");
