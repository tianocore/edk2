/** @file
  Null instance of Memory Test Library.
  
Copyright (c) 2009, Intel Corporation.<BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Base.h>
#include <Library/BaseMemoryTestLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/ReportStatusCodeLib.h>


UINT32 TestPattern[] = {
  0x5a5a5a5a,
  0xa5a5a5a5,
  0x5a5a5a5a,
  0xa5a5a5a5,
  0x5a5a5a5a,
  0xa5a5a5a5,
  0x5a5a5a5a,
  0xa5a5a5a5,
  0x5a5a5a5a,
  0xa5a5a5a5,
  0x5a5a5a5a,
  0xa5a5a5a5,
  0x5a5a5a5a,
  0xa5a5a5a5,
  0x5a5a5a5a,
  0xa5a5a5a5
};

/**
  Internal worker function for system memory range test.

  This function is the internal worker function for system memory range test.
  It writes test pattern to memory range and then reads back to check if memory
  works well.

  @param  StartAddress           Start address of the memory range to test.
  @param  Length                 Length of the memory range to test.
  @param  ErrorAddress           Address of the memory where error is encountered.

  @retval RETURN_SUCCESS         The memory range passes test.
  @retval RETURN_DEVICE_ERROR    The memory range does not pass test.

**/
RETURN_STATUS
MemoryTestWorker (
  IN  VOID              *StartAddress,
  IN  UINT64            Length,
  IN  UINTN             SpanSize,
  OUT VOID              **ErrorAddress
  )
{
  VOID   *TempAddress;

  //
  // Make sure we don't try and test anything above the max physical address range
  //
  ASSERT ((UINTN) StartAddress + Length < MAX_ADDRESS);

  REPORT_STATUS_CODE (EFI_PROGRESS_CODE,  PcdGet32 (PcdStatusCodeValueMemoryTestStarted));

  //
  // Write the test pattern into memory range
  //
  TempAddress = StartAddress;
  while ((UINTN) TempAddress < (UINTN) StartAddress + Length) {
    CopyMem (TempAddress, TestPattern, sizeof (TestPattern));
    TempAddress = (VOID *) ((UINTN) TempAddress + SpanSize);
  }

  //
  // Write back and invalidate cache to make sure data is in memory
  //
  WriteBackInvalidateDataCacheRange (StartAddress, (UINTN) Length);

  //
  // Read pattern from memory and compare it
  //
  TempAddress = StartAddress;
  while ((UINTN) TempAddress < (UINTN) StartAddress + Length) {
    if (CompareMem (TempAddress, TestPattern, sizeof (TestPattern)) != 0) {
      //
      // Value read back does not equal to the value written, so error is detected.
      //
      *ErrorAddress = TempAddress;
      REPORT_STATUS_CODE (EFI_ERROR_CODE | EFI_ERROR_UNRECOVERED, PcdGet32 (PcdStatusCodeValueUncorrectableMemoryError));

      return RETURN_DEVICE_ERROR;
    }

    TempAddress = (VOID *) ((UINTN) TempAddress + SpanSize);
  }

  return RETURN_SUCCESS;
}

/**
  Perform a quick system memory range test.

  This function performs a quick system memory range test. It leads to quick performance
  but least reliability.

  @param  StartAddress           Start address of the memory range to test.
  @param  Length                 Length of the memory range to test.
  @param  ErrorAddress           Address of the memory where error is encountered.

  @retval RETURN_SUCCESS         The memory range passes test.
  @retval RETURN_DEVICE_ERROR    The memory range does not pass test.

**/
RETURN_STATUS
EFIAPI
QuickMemoryTest (
  IN  VOID      *StartAddress,
  IN  UINT64    Length,
  OUT VOID      **ErrorAddress
  )
{
  RETURN_STATUS   Status;

  Status = MemoryTestWorker (
             StartAddress,
             Length,
             sizeof (TestPattern) * 0x20000,
             ErrorAddress
             );

  return Status;
}

/**
  Test a system memory range with sparsely sampled memory units.

  This function tests a system memory range, whose memory units
  are sampled sparsely. It leads to relatively good performance
  and partial reliability.

  @param  StartAddress           Start address of the memory range to test.
  @param  Length                 Length of the memory range to test.
  @param  ErrorAddress           Address of the memory where error is encountered.

  @retval RETURN_SUCCESS         The memory range passes test.
  @retval RETURN_DEVICE_ERROR    The memory range does not pass test.

**/
RETURN_STATUS
EFIAPI
SparseMemoryTest (
  IN  VOID      *StartAddress,
  IN  UINT64    Length,
  OUT VOID      **ErrorAddress
  )
{
  RETURN_STATUS   Status;

  Status = MemoryTestWorker (
             StartAddress,
             Length,
             sizeof (TestPattern) * 0x8000,
             ErrorAddress
             );

  return Status;
}

/**
  Test a system memory range with extensively sampled memory units.

  This function tests a system memory range, whose memory units
  are sampled extensively. Compared with SparseMemoryTest, it achieves
  more reliability and less performance.

  @param  StartAddress           Start address of the memory range to test.
  @param  Length                 Length of the memory range to test.
  @param  ErrorAddress           Address of the memory where error is encountered.

  @retval RETURN_SUCCESS         The memory range passes test.
  @retval RETURN_DEVICE_ERROR    The memory range does not pass test.

**/
RETURN_STATUS
EFIAPI
ExtensiveMemoryTest (
  IN  VOID      *StartAddress,
  IN  UINT64    Length,
  OUT VOID      **ErrorAddress
  )
{
  RETURN_STATUS   Status;

  Status = MemoryTestWorker (
             StartAddress,
             Length,
             sizeof (TestPattern),
             ErrorAddress
             );

  return Status;
}

/**
  Check if soft ECC initialzation is needed for system

  @retval TRUE         Soft ECC initialzation is needed.
  @retval FALSE        Soft ECC initialzation is not needed.

**/
BOOLEAN
EFIAPI
IsSoftEccInitRequired (
  VOID
  )
{
  return FALSE;
}
