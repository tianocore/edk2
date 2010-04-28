/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PlatformMemTest.c

Abstract:

  // TBD defined in the Tiano specification??

  The Platform memory test protocol is used to provide platform specific
  information and functionality for memory test
 
--*/

#include "Tiano.h"
#include EFI_PROTOCOL_DEFINITION (PlatformMemTest)

EFI_GUID  gEfiPlatformMemTestGuid = EFI_PLATFORM_MEMTEST_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiPlatformMemTestGuid, "Platform Memory Test Protocol", "Platform MemTest protocol");
