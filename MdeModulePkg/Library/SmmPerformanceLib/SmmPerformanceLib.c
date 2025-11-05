/** @file
  Performance Library used in SMM phase.

  This library instance provides infrastructure for SMM drivers to log performance
  data. It consumes SMM PerformanceEx or Performance Protocol published by SmmCorePerformanceLib
  to log performance data. If both SMM PerformanceEx and Performance Protocol are not available, it does not log any
  performance information.

  Copyright (c) 2011 - 2023, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiMm.h>
#include <Protocol/SmmExitBootServices.h>
#include "SmmPerformanceLibInternal.h"

/**
  The constructor function initializes the Performance Measurement Enable flag.

  @param[in]  ImageHandle   The firmware allocated handle for the EFI image.
  @param[in]  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
SmmPerformanceLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return RegisterExitBootServicesCallback (&gEdkiiSmmExitBootServicesProtocolGuid);
}

/**
  The destructor function frees resources allocated by constructor.

  @param[in]  ImageHandle   The firmware allocated handle for the EFI image.
  @param[in]  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The destructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
SmmPerformanceLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return UnregisterExitBootServicesCallback (&gEdkiiSmmExitBootServicesProtocolGuid);
}
