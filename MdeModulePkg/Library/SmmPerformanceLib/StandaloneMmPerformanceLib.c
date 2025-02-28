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

#include <Guid/PerformanceMeasurement.h>

#include <Library/DebugLib.h>
#include <Library/MmServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/PerformanceLib.h>

#include "SmmPerformanceLibInternal.h"

extern BOOLEAN  mPerformanceMeasurementEnabled;
extern VOID     *mPerformanceLibExitBootServicesRegistration;

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
  EFI_STATUS  Status;

  mPerformanceMeasurementEnabled =  (BOOLEAN)((PcdGet8 (PcdPerformanceLibraryPropertyMask) & PERFORMANCE_LIBRARY_PROPERTY_MEASUREMENT_ENABLED) != 0);

  Status = gMmst->MmRegisterProtocolNotify (
                    &gEdkiiSmmExitBootServicesProtocolGuid,
                    SmmPerformanceLibExitBootServicesCallback,
                    &mPerformanceLibExitBootServicesRegistration
                    );
  ASSERT_EFI_ERROR (Status);

  return Status;
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
  EFI_STATUS  Status;

  //
  // Unregister SmmExitBootServices notification.
  //
  Status = gMmst->MmRegisterProtocolNotify (
                    &gEdkiiSmmExitBootServicesProtocolGuid,
                    NULL,
                    &mPerformanceLibExitBootServicesRegistration
                    );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
