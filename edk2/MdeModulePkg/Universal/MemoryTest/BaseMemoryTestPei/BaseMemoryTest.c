/** @file
  Support of memory test in PEI Phase.

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "BaseMemoryTest.h"

PEI_BASE_MEMORY_TEST_PPI mPeiBaseMemoryTestPpi = {
  BaseMemoryTest
};

EFI_PEI_PPI_DESCRIPTOR   PpiListPeiBaseMemoryTest = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gPeiBaseMemoryTestPpiGuid,
  &mPeiBaseMemoryTestPpi
};

/**
  Entry point of BaseMemoryTestPei PEIM.

  This function is the entry point of BaseMemoryTestPei PEIM.
  It installs the PEI_BASE_MEMORY_TEST_PPI.

  @param  FfsHeader      Pointer to FFS File Header.
  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.

  @retval EFI_SUCCESS    PEI_BASE_MEMORY_TEST_PPI is successfully installed.
  @retval Others         PEI_BASE_MEMORY_TEST_PPI is not successfully installed.

**/  
EFI_STATUS
EFIAPI
PeiBaseMemoryTestInit (
  IN       EFI_FFS_FILE_HEADER       *FfsHeader,
  IN CONST EFI_PEI_SERVICES          **PeiServices
  )
{
  return PeiServicesInstallPpi (&PpiListPeiBaseMemoryTest);
 
}

/**
  Test base memory.

  @param  PeiServices      An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This             Pointer to this PPI instance.
  @param  BeginAddress     Beginning of the memory address to be checked.
  @param  MemoryLength     Bytes of memory range to be checked.
  @param  Operation        Type of memory check operation to be performed.
  @param  ErrorAddress     Pointer to address of the error memory returned.

  @retval EFI_SUCCESS      Memory test passed.
  @retval EFI_DEVICE_ERROR Memory test failed.

**/    
EFI_STATUS
EFIAPI
BaseMemoryTest (
  IN  EFI_PEI_SERVICES                   **PeiServices,
  IN  PEI_BASE_MEMORY_TEST_PPI           *This,
  IN  EFI_PHYSICAL_ADDRESS               BeginAddress,
  IN  UINT64                             MemoryLength,
  IN  PEI_MEMORY_TEST_OP                 Operation,
  OUT EFI_PHYSICAL_ADDRESS               *ErrorAddress
  )
{
  UINT32                TestPattern;
  EFI_PHYSICAL_ADDRESS  TempAddress;
  UINT32                SpanSize;

  REPORT_STATUS_CODE (EFI_PROGRESS_CODE,  PcdGet32 (PcdStatusCodeValueMemoryTestStarted));

  TestPattern = TEST_PATTERN;
  SpanSize    = 0;

  //
  // Make sure we don't try and test anything above the max physical address range
  //
  ASSERT (BeginAddress + MemoryLength < EFI_MAX_ADDRESS);

  switch (Operation) {
  case Extensive:
    //
    // Extensive means full and detailed check,
    // so use small span size to cover the entire test range.
    //
    SpanSize = 0x4;
    break;

  case Sparse:
  case Quick:
    //
    // Sparse and Quick indicates quick test,
    // so use large span size for sample test.
    //
    SpanSize = COVER_SPAN;
    break;

  case Ignore:
    //
    // Ignore means no test.
    //
    goto Done;
    break;
  }
  //
  // Write the test pattern into memory range
  //
  TempAddress = BeginAddress;
  while (TempAddress < BeginAddress + MemoryLength) {
    (*(UINT32 *) (UINTN) TempAddress) = TestPattern;
    TempAddress += SpanSize;
  }
  //
  // Read pattern from memory and compare it
  //
  TempAddress = BeginAddress;
  while (TempAddress < BeginAddress + MemoryLength) {
    if ((*(UINT32 *) (UINTN) TempAddress) != TestPattern) {
      //
      // Value read back does not equal to the value written, so error is detected.
      //
      *ErrorAddress = TempAddress;
      REPORT_STATUS_CODE (EFI_ERROR_CODE | EFI_ERROR_UNRECOVERED, PcdGet32 (PcdStatusCodeValueUncorrectableMemoryError));

      return EFI_DEVICE_ERROR;
    }

    TempAddress += SpanSize;
  }

Done:
  return EFI_SUCCESS;
}
