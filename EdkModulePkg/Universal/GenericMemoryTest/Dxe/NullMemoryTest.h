/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    NullMemoryTest.h
  
Abstract:
  The generic memory test driver definition

--*/

#ifndef _NULL_MEMORY_TEST_H
#define _NULL_MEMORY_TEST_H

#include "Common.h"

//
// Function Prototypes
//
EFI_STATUS
EFIAPI
InitializeMemoryTest (
  IN EFI_GENERIC_MEMORY_TEST_PROTOCOL          *This,
  IN  EXTENDMEM_COVERAGE_LEVEL                 Level,
  OUT BOOLEAN                                  *RequireSoftECCInit
  )
;

EFI_STATUS
EFIAPI
GenPerformMemoryTest (
  IN EFI_GENERIC_MEMORY_TEST_PROTOCOL          *This,
  IN OUT UINT64                                *TestedMemorySize,
  OUT UINT64                                   *TotalMemorySize,
  OUT BOOLEAN                                  *ErrorOut,
  IN BOOLEAN                                   TestAbort
  )
;

EFI_STATUS
EFIAPI
GenMemoryTestFinished (
  IN EFI_GENERIC_MEMORY_TEST_PROTOCOL *This
  )
;

EFI_STATUS
EFIAPI
GenCompatibleRangeTest (
  IN EFI_GENERIC_MEMORY_TEST_PROTOCOL          *This,
  IN  EFI_PHYSICAL_ADDRESS                     StartAddress,
  IN  UINT64                                   Length
  )
;

#endif
