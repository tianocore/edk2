/** @file
  Agent Module to load other modules to deploy SMM Entry Vector for X86 CPU.

  @copyright
  INTEL CONFIDENTIAL
  Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PiSmmCpuCommon.h"
#include <Library/StandaloneMmMemLib.h>

/**
  This function is an abstraction layer for implementation specific Mm buffer validation routine.

  @param Buffer  The buffer start address to be checked.
  @param Length  The buffer length to be checked.

  @retval TRUE  This buffer is valid per processor architecture and not overlap with SMRAM.
  @retval FALSE This buffer is not valid per processor architecture or overlap with SMRAM.
**/
BOOLEAN
IsBufferOutsideMmValid (
  IN EFI_PHYSICAL_ADDRESS  Buffer,
  IN UINT64                Length
  )
{
  return MmIsBufferOutsideMmValid (Buffer, Length);
}

/**
  The module Entry Point of the CPU StandaloneMm driver.

  @param  ImageHandle    The firmware allocated handle for the EFI image.
  @param  SystemTable    A pointer to the MM System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
PiCpuStandaloneMmEntry (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status =  PiSmmCpuEntryCommon ();

  ASSERT_EFI_ERROR (Status);

  //
  // Install the SMM Configuration Protocol onto a new handle on the handle database.
  // The entire SMM Configuration Protocol is allocated from SMRAM, so only a pointer
  // to an SMRAM address will be present in the handle database
  //
  Status = gMmst->MmInstallProtocolInterface (
                    &gSmmCpuPrivate->SmmCpuHandle,
                    &gEfiSmmConfigurationProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &gSmmCpuPrivate->SmmConfiguration
                    );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
