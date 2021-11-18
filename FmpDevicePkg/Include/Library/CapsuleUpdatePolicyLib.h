/** @file
  Provides platform policy services used during a capsule update.

  Copyright (c) 2016, Microsoft Corporation. All rights reserved.<BR>
  Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __CAPSULE_UPDATE_POLICY_LIB__
#define __CAPSULE_UPDATE_POLICY_LIB__

/**
  Determine if the system power state supports a capsule update.

  @param[out] Good  Returns TRUE if system power state supports a capsule
                    update.  Returns FALSE if system power state does not
                    support a capsule update.  Return value is only valid if
                    return status is EFI_SUCCESS.

  @retval EFI_SUCCESS            Good parameter has been updated with result.
  @retval EFI_INVALID_PARAMETER  Good is NULL.
  @retval EFI_DEVICE_ERROR       System power state can not be determined.

**/
EFI_STATUS
EFIAPI
CheckSystemPower (
  OUT BOOLEAN  *Good
  );

/**
  Determines if the system thermal state supports a capsule update.

  @param[out] Good  Returns TRUE if system thermal state supports a capsule
                    update.  Returns FALSE if system thermal state does not
                    support a capsule update.  Return value is only valid if
                    return status is EFI_SUCCESS.

  @retval EFI_SUCCESS            Good parameter has been updated with result.
  @retval EFI_INVALID_PARAMETER  Good is NULL.
  @retval EFI_DEVICE_ERROR       System thermal state can not be determined.

**/
EFI_STATUS
EFIAPI
CheckSystemThermal (
  OUT BOOLEAN  *Good
  );

/**
  Determines if the system environment state supports a capsule update.

  @param[out] Good  Returns TRUE if system environment state supports a capsule
                    update.  Returns FALSE if system environment state does not
                    support a capsule update.  Return value is only valid if
                    return status is EFI_SUCCESS.

  @retval EFI_SUCCESS            Good parameter has been updated with result.
  @retval EFI_INVALID_PARAMETER  Good is NULL.
  @retval EFI_DEVICE_ERROR       System environment state can not be determined.

**/
EFI_STATUS
EFIAPI
CheckSystemEnvironment (
  OUT BOOLEAN  *Good
  );

/**
  Determines if the Lowest Supported Version checks should be performed.  The
  expected result from this function is TRUE.  A platform can choose to return
  FALSE (e.g. during manufacturing or servicing) to allow a capsule update to a
  version below the current Lowest Supported Version.

  @retval TRUE   The lowest supported version check is required.
  @retval FALSE  Do not perform lowest support version check.

**/
BOOLEAN
EFIAPI
IsLowestSupportedVersionCheckRequired (
  VOID
  );

/**
  Determines if the FMP device should be locked when the event specified by
  PcdFmpDeviceLockEventGuid is signaled. The expected result from this function
  is TRUE so the FMP device is always locked.  A platform can choose to return
  FALSE (e.g. during manufacturing) to allow FMP devices to remain unlocked.

  @retval TRUE   The FMP device lock action is required at lock event guid.
  @retval FALSE  Do not perform FMP device lock at lock event guid.

**/
BOOLEAN
EFIAPI
IsLockFmpDeviceAtLockEventGuidRequired (
  VOID
  );

#endif
