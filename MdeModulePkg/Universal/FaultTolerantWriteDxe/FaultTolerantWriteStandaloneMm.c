/** @file

  Parts of the SMM/MM implementation that are specific to standalone MM

Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2018, Linaro, Ltd. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/SmmMemLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include "FaultTolerantWrite.h"
#include "FaultTolerantWriteSmmCommon.h"

/**
  This function checks if the buffer is valid per processor architecture and
  does not overlap with SMRAM.

  @param Buffer The buffer start address to be checked.
  @param Length The buffer length to be checked.

  @retval TRUE  This buffer is valid per processor architecture and does not
                overlap with SMRAM.
  @retval FALSE This buffer is not valid per processor architecture or overlaps
                with SMRAM.
**/
BOOLEAN
FtwSmmIsBufferOutsideSmmValid (
  IN EFI_PHYSICAL_ADDRESS  Buffer,
  IN UINT64                Length
  )
{
  return TRUE;
}

/**
  Internal implementation of CRC32. Depending on the execution context
  (standalone SMM or DXE vs standalone MM), this function is implemented
  via a call to the CalculateCrc32 () boot service, or via a library
  call.

  If Buffer is NULL, then ASSERT().
  If Length is greater than (MAX_ADDRESS - Buffer + 1), then ASSERT().

  @param[in]  Buffer       A pointer to the buffer on which the 32-bit CRC is to be computed.
  @param[in]  Length       The number of bytes in the buffer Data.

  @retval Crc32            The 32-bit CRC was computed for the data buffer.

**/
UINT32
FtwCalculateCrc32 (
  IN  VOID   *Buffer,
  IN  UINTN  Length
  )
{
  return CalculateCrc32 (Buffer, Length);
}

/**
  Notify the system that the SMM FTW driver is ready.
**/
VOID
FtwNotifySmmReady (
  VOID
  )
{
}

/**
  This function is the entry point of the Fault Tolerant Write driver.

  @param[in] ImageHandle        A handle for the image that is initializing this driver
  @param[in] MmSystemTable      A pointer to the MM system table

  @retval EFI_SUCCESS           The initialization finished successfully.
  @retval EFI_OUT_OF_RESOURCES  Allocate memory error
  @retval EFI_INVALID_PARAMETER Workspace or Spare block does not exist

**/
EFI_STATUS
EFIAPI
StandaloneMmFaultTolerantWriteInitialize (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *MmSystemTable
  )
{
  return MmFaultTolerantWriteInitialize ();
}
