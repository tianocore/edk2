/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  GenericMemoryTest.c

Abstract:

  The generic memory test protocol is used to test EFI memory.

  For more information please look at EfiMemoryTest.doc

--*/

#include "Tiano.h"
#include EFI_PROTOCOL_DEFINITION (GenericMemoryTest)

EFI_GUID  gEfiGenericMemTestProtocolGuid = EFI_GENERIC_MEMORY_TEST_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiGenericMemTestProtocolGuid, "GenericMemoryTest Protocol", "Tiano Generic Memory Test Protocol");
