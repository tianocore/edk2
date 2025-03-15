/** @file
  Performance Library used in Standalone MM phase.

  This library instance provides infrastructure for Standalone MM drivers to log performance
  data. It consumes the MM PerformanceEx or Performance Protocol to log performance data. If
  both MM PerformanceEx and Performance Protocol are not available, it does not log any
  performance information.

  Copyright (c) 2011 - 2023, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiMm.h>
#include <Guid/EventGroup.h>
#include "SmmPerformanceLibInternal.h"

/**
  The constructor function initializes the Performance Measurement Enable flag.

  @param[in]  ImageHandle   The firmware allocated handle for the EFI image.
  @param[in]  MmSystemTable A pointer to the MM System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
StandaloneMmPerformanceLibConstructor (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *MmSystemTable
  )
{
  return RegisterExitBootServicesCallback (&gEfiEventExitBootServicesGuid);
}

/**
  The destructor function frees resources allocated by constructor.

  @param[in]  ImageHandle   The firmware allocated handle for the EFI image.
  @param[in]  MmSystemTable A pointer to the MM System Table.

  @retval EFI_SUCCESS   The destructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
StandaloneMmPerformanceLibDestructor (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *MmSystemTable
  )
{
  return UnregisterExitBootServicesCallback (&gEfiEventExitBootServicesGuid);
}
