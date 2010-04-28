/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    DebugAssert.c

Abstract:

  This protocol allows provides debug services to a driver. This is not 
  debugger support, but things like ASSERT() and DEBUG() macros

--*/

#include "Tiano.h"
#include EFI_PROTOCOL_DEFINITION(DebugAssert)
 

EFI_GUID gEfiDebugAssertProtocolGuid = EFI_DEBUG_ASSERT_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiDebugAssertProtocolGuid, "EFI Debug Assert", "Efi Debug Assert Protocol");

