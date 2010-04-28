/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  StatusCodeMemory.c

Abstract:

  Status Code Memory descriptor

--*/

#include "Tiano.h"
#include "Pei.h"
#include EFI_PPI_DEFINITION (StatusCodeMemory)

EFI_GUID gPeiStatusCodeMemoryPpiGuid = PEI_STATUS_CODE_MEMORY_PPI_GUID;

EFI_GUID_STRING(&gPeiStatusCodeMemoryPpiGuid, "StatusCodeMemory", "Status Code memory descriptor.");
