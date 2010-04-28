/*++

Copyright (c) 2004 - 2005, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  MemoryStatusCodeLib.h

Abstract:

  Lib to provide memory status code reporting.

--*/

#ifndef _PEI_MEMORY_STATUS_CODE_LIB_H_
#define _PEI_MEMORY_STATUS_CODE_LIB_H_

//
// Statements that include other files
//
#include "Tiano.h"
#include "Pei.h"

//
// Publicly exported data
//
extern BOOLEAN  mRunningFromMemory;

//
// Initialization function
//
VOID
MemoryInitializeStatusCode (
  IN EFI_FFS_FILE_HEADER       *FfsHeader,
  IN EFI_PEI_SERVICES          **PeiServices
  );

//
// Status code reporting function
//
EFI_STATUS
MemoryReportStatusCode (
  IN EFI_PEI_SERVICES         **PeiServices,
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 * CallerId,
  IN EFI_STATUS_CODE_DATA     * Data OPTIONAL
  );

#endif
