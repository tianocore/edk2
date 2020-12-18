/** @file
  Produces the SMM CPU I/O Protocol.

Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiMm.h>

#include "CpuIo2Common.h"

/**
  The module Entry Point SmmCpuIoProtocol driver

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS  The entry point is executed successfully.
  @retval Other        Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
StandaloneMmCpuIo2Initialize (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *SystemTable
  )
{
  return CommonCpuIo2Initialize ();
}
