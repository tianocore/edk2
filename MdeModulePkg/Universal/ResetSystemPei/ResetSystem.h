/** @file

  Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _RESET_SYSTEM2_H_
#define _RESET_SYSTEM2_H_

#include <Uefi.h>
#include <PiPei.h>

#include <Ppi/Reset2.h>
#include <Ppi/PlatformSpecificResetFilter.h>
#include <Ppi/PlatformSpecificResetNotification.h>
#include <Ppi/PlatformSpecificResetHandler.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/HobLib.h>
#include <Library/ResetSystemLib.h>
#include <Library/ReportStatusCodeLib.h>

//
// The maximum recursion depth to ResetSystem() by reset notification handlers
//
#define MAX_RESET_NOTIFY_DEPTH  10

//
// Data to put in GUIDed HOB
//
typedef struct {
  UINT32              Signature;
  UINT32              Count;
  EFI_RESET_SYSTEM    ResetFilters[0];             // ResetFilters[PcdGet32 (PcdMaximumResetNotifies)]
} RESET_FILTER_LIST;
#define RESET_FILTER_LIST_SIGNATURE  SIGNATURE_32('r', 's', 't', 'l')

typedef struct {
  EDKII_PLATFORM_SPECIFIC_RESET_FILTER_PPI    ResetFilter;
  EFI_GUID                                    *Guid;
} RESET_FILTER_INSTANCE;

/**
  Resets the entire platform.

  @param[in] ResetType          The type of reset to perform.
  @param[in] ResetStatus        The status code for the reset.
  @param[in] DataSize           The size, in bytes, of ResetData.
  @param[in] ResetData          For a ResetType of EfiResetCold, EfiResetWarm, or
                                EfiResetShutdown the data buffer starts with a Null-terminated
                                string, optionally followed by additional binary data.
                                The string is a description that the caller may use to further
                                indicate the reason for the system reset.
                                For a ResetType of EfiResetPlatformSpecific the data buffer
                                also starts with a Null-terminated string that is followed
                                by an EFI_GUID that describes the specific type of reset to perform.

**/
VOID
EFIAPI
ResetSystem2 (
  IN EFI_RESET_TYPE  ResetType,
  IN EFI_STATUS      ResetStatus,
  IN UINTN           DataSize,
  IN VOID            *ResetData OPTIONAL
  );

/**
  Register a notification function to be called when ResetSystem() is called.

  The RegisterResetNotify() function registers a notification function that is called when
  ResetSystem()is called and prior to completing the reset of the platform.
  The registered functions must not perform a platform reset themselves. These
  notifications are intended only for the notification of components which may need some
  special-purpose maintenance prior to the platform resetting.

  @param[in]  This              A pointer to the EDKII_PLATFORM_SPECIFIC_RESET_FILTER_PPI instance.
  @param[in]  ResetFunction     Points to the function to be called when a ResetSystem() is executed.

  @retval EFI_SUCCESS           The reset notification function was successfully registered.
  @retval EFI_INVALID_PARAMETER ResetFunction is NULL.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resources available to register the reset notification function.
  @retval EFI_ALREADY_STARTED   The reset notification function specified by ResetFunction has already been registered.

**/
EFI_STATUS
EFIAPI
RegisterResetNotify (
  IN EDKII_PLATFORM_SPECIFIC_RESET_FILTER_PPI  *This,
  IN EFI_RESET_SYSTEM                          ResetFunction
  );

/**
  Unregister a notification function.

  The UnregisterResetNotify() function removes the previously registered
  notification using RegisterResetNotify().

  @param[in]  This              A pointer to the EFI_RESET_NOTIFICATION_PROTOCOL instance.
  @param[in]  ResetFunction     The pointer to the ResetFunction being unregistered.

  @retval EFI_SUCCESS           The reset notification function was unregistered.
  @retval EFI_INVALID_PARAMETER ResetFunction is NULL.
  @retval EFI_INVALID_PARAMETER The reset notification function specified by ResetFunction was not previously
                                registered using RegisterResetNotify().

**/
EFI_STATUS
EFIAPI
UnregisterResetNotify (
  IN EFI_RESET_NOTIFICATION_PROTOCOL  *This,
  IN EFI_RESET_SYSTEM                 ResetFunction
  );

#endif
