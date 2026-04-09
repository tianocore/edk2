/** @file
  Provides platform policy services used during a capsule update that uses the
  services of the EDKII_CAPSULE_UPDATE_POLICY_PROTOCOL.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CapsuleUpdatePolicyDxe.h"

///
/// Handle for the Capsule Update Policy Protocol
///
EFI_HANDLE  mHandle = NULL;

///
/// Capsule Update Policy Protocol instance
///
EDKII_CAPSULE_UPDATE_POLICY_PROTOCOL  mCapsuleUpdatePolicy = {
  CapsuleUpdatePolicyCheckSystemPower,
  CapsuleUpdatePolicyCheckSystemThermal,
  CapsuleUpdatePolicyCheckSystemEnvironment,
  CapsuleUpdatePolicyIsLowestSupportedVersionCheckRequired,
  CapsuleUpdatePolicyIsLockFmpDeviceAtLockEventGuidRequired
};

/**
  Determine if the system power state supports a capsule update.

  @param[in]  This  A pointer to the EDKII_CAPSULE_UPDATE_POLICY_PROTOCOL instance.
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
CapsuleUpdatePolicyCheckSystemPower (
  IN  EDKII_CAPSULE_UPDATE_POLICY_PROTOCOL  *This,
  OUT BOOLEAN                               *Good
  )
{
  return CheckSystemPower (Good);
}

/**
  Determines if the system thermal state supports a capsule update.

  @param[in]  This  A pointer to the EDKII_CAPSULE_UPDATE_POLICY_PROTOCOL instance.
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
CapsuleUpdatePolicyCheckSystemThermal (
  IN  EDKII_CAPSULE_UPDATE_POLICY_PROTOCOL  *This,
  OUT BOOLEAN                               *Good
  )
{
  return CheckSystemThermal (Good);
}

/**
  Determines if the system environment state supports a capsule update.

  @param[in]  This  A pointer to the EDKII_CAPSULE_UPDATE_POLICY_PROTOCOL instance.
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
CapsuleUpdatePolicyCheckSystemEnvironment (
  IN  EDKII_CAPSULE_UPDATE_POLICY_PROTOCOL  *This,
  OUT BOOLEAN                               *Good
  )
{
  return CheckSystemEnvironment (Good);
}

/**
  Determines if the Lowest Supported Version checks should be performed.  The
  expected result from this function is TRUE.  A platform can choose to return
  FALSE (e.g. during manufacturing or servicing) to allow a capsule update to a
  version below the current Lowest Supported Version.

  @param[in]  This  A pointer to the EDKII_CAPSULE_UPDATE_POLICY_PROTOCOL instance.

  @retval TRUE   The lowest supported version check is required.
  @retval FALSE  Do not perform lowest support version check.

**/
BOOLEAN
EFIAPI
CapsuleUpdatePolicyIsLowestSupportedVersionCheckRequired (
  IN  EDKII_CAPSULE_UPDATE_POLICY_PROTOCOL  *This
  )
{
  return IsLowestSupportedVersionCheckRequired ();
}

/**
  Determines if the FMP device should be locked when the event specified by
  PcdFmpDeviceLockEventGuid is signaled. The expected result from this function
  is TRUE so the FMP device is always locked.  A platform can choose to return
  FALSE (e.g. during manufacturing) to allow FMP devices to remain unlocked.

  @param[in]  This  A pointer to the EDKII_CAPSULE_UPDATE_POLICY_PROTOCOL instance.

  @retval TRUE   The FMP device lock action is required at lock event guid.
  @retval FALSE  Do not perform FMP device lock at lock event guid.

**/
BOOLEAN
EFIAPI
CapsuleUpdatePolicyIsLockFmpDeviceAtLockEventGuidRequired (
  IN  EDKII_CAPSULE_UPDATE_POLICY_PROTOCOL  *This
  )
{
  return IsLockFmpDeviceAtLockEventGuidRequired ();
}

/**
  The user Entry Point for module CapsuleUpdatePolicyDxe. The user code starts
  with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
CapsuleUpdatePolicyInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gEdkiiCapsuleUpdatePolicyProtocolGuid);
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mHandle,
                  &gEdkiiCapsuleUpdatePolicyProtocolGuid,
                  &mCapsuleUpdatePolicy,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
