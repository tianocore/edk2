/** @file
  The PEI memory test support

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <BaseMemoryTest.h>
#include <Library/PeiServicesLib.h>

PEI_BASE_MEMORY_TEST_PPI mPeiBaseMemoryTestPpi = { BaseMemoryTest };

EFI_PEI_PPI_DESCRIPTOR   PpiListPeiBaseMemoryTest = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gPeiBaseMemoryTestPpiGuid,
  &mPeiBaseMemoryTestPpi
};

EFI_STATUS
EFIAPI
PeiBaseMemoryTestInit (
  IN EFI_FFS_FILE_HEADER       *FfsHeader,
  IN EFI_PEI_SERVICES          **PeiServices
  )
/*++
Description:

  Entry point function of BaseMemoryTestInit Peim.

Arguments:

  PeiServices           - General purpose services available to every PEIM.
  FfsHeader             - Ffs header pointer

Returns:

  Status                - Result of InstallPpi

--*/  
{

  return PeiServicesInstallPpi (&PpiListPeiBaseMemoryTest);
  
}

EFI_STATUS
EFIAPI
BaseMemoryTest (
  IN  EFI_PEI_SERVICES                   **PeiServices,
  IN PEI_BASE_MEMORY_TEST_PPI            *This,
  IN  EFI_PHYSICAL_ADDRESS               BeginAddress,
  IN  UINT64                             MemoryLength,
  IN  PEI_MEMORY_TEST_OP                 Operation,
  OUT EFI_PHYSICAL_ADDRESS               *ErrorAddress
  )
/*++
Description:

  Test base memory.

Arguments:

  PeiServices   - General purpose services available to every PEIM.
  This          - Pei memory test PPI pointer.
  BeginAddress  - Beginning of the memory address to be checked.
  MemoryLength  - Bytes of memory range to be checked.
  Operation     - Type of memory check operation to be performed.
  ErrorAddress  - Return the address of the error memory address.
  ErrorAddress  - Address which has error when checked.

Returns:

  Status        - Result of InstallPpi

--*/    
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
    SpanSize = 0x4;
    break;

  case Sparse:
  case Quick:
    SpanSize = COVER_SPAN;
    break;

  case Ignore:
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
      *ErrorAddress = TempAddress;
      REPORT_STATUS_CODE (EFI_ERROR_CODE | EFI_ERROR_UNRECOVERED, PcdGet32 (PcdStatusCodeValueUncorrectableMemoryError));

      return EFI_DEVICE_ERROR;
    }

    TempAddress += SpanSize;
  }

Done:
  return EFI_SUCCESS;
}
