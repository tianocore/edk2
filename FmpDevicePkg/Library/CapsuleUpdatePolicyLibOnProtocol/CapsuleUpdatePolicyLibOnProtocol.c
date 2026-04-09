/** @file
  Provides platform policy services used during a capsule update that uses the
  services of the EDKII_CAPSULE_UPDATE_POLICY_PROTOCOL. If the
  EDKII_CAPSULE_UPDATE_POLICY_PROTOCOL is not available, then assume the
  platform is in a state that supports a firmware update.

  Copyright (c) 2016, Microsoft Corporation. All rights reserved.<BR>
  Copyright (c) 2018-2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/CapsuleUpdatePolicyLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/CapsuleUpdatePolicy.h>

///
/// Pointer to the EDK II Capsule Update Policy Protocol instances that is
/// optionally installed by a platform.
///
EDKII_CAPSULE_UPDATE_POLICY_PROTOCOL  *mCapsuleUpdatePolicy = NULL;

/**
  Lookup the EDK II Capsule Update Policy Protocol.
**/
BOOLEAN
LookupCapsuleUpdatePolicyProtocol (
  VOID
  )
{
  EFI_STATUS  Status;

  if (mCapsuleUpdatePolicy != NULL) {
    return TRUE;
  }

  Status = gBS->LocateProtocol (
                  &gEdkiiCapsuleUpdatePolicyProtocolGuid,
                  NULL,
                  (VOID **)&mCapsuleUpdatePolicy
                  );
  if (EFI_ERROR (Status)) {
    mCapsuleUpdatePolicy = NULL;
    return FALSE;
  }

  return TRUE;
}

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
  )
{
  if (LookupCapsuleUpdatePolicyProtocol ()) {
    return mCapsuleUpdatePolicy->CheckSystemPower (mCapsuleUpdatePolicy, Good);
  }

  *Good = TRUE;
  return EFI_SUCCESS;
}

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
  )
{
  if (LookupCapsuleUpdatePolicyProtocol ()) {
    return mCapsuleUpdatePolicy->CheckSystemThermal (mCapsuleUpdatePolicy, Good);
  }

  *Good = TRUE;
  return EFI_SUCCESS;
}

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
  )
{
  if (LookupCapsuleUpdatePolicyProtocol ()) {
    return mCapsuleUpdatePolicy->CheckSystemEnvironment (mCapsuleUpdatePolicy, Good);
  }

  *Good = TRUE;
  return EFI_SUCCESS;
}

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
  )
{
  if (LookupCapsuleUpdatePolicyProtocol ()) {
    return mCapsuleUpdatePolicy->IsLowestSupportedVersionCheckRequired (mCapsuleUpdatePolicy);
  }

  return TRUE;
}

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
  )
{
  if (LookupCapsuleUpdatePolicyProtocol ()) {
    return mCapsuleUpdatePolicy->IsLockFmpDeviceAtLockEventGuidRequired (mCapsuleUpdatePolicy);
  }

  return TRUE;
}
