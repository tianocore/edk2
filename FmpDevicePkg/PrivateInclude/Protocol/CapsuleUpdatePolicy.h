/** @file
  Provides platform policy services used during a capsule update.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __CAPSULE_UPDATE_POLICY_H__
#define __CAPSULE_UPDATE_POLICY_H__

#define EDKII_CAPSULE_UPDATE_POLICY_PROTOCOL_GUID \
  { \
    0x487784c5, 0x6299, 0x4ba6, { 0xb0, 0x96, 0x5c, 0xc5, 0x27, 0x7c, 0xf7, 0x57 } \
  }

typedef struct _EDKII_CAPSULE_UPDATE_POLICY_PROTOCOL  EDKII_CAPSULE_UPDATE_POLICY_PROTOCOL;

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
typedef
EFI_STATUS
(EFIAPI * EDKII_CAPSULE_UPDATE_POLICY_CHECK_SYSTEM_POWER) (
  IN  EDKII_CAPSULE_UPDATE_POLICY_PROTOCOL  *This,
  OUT BOOLEAN                               *Good
  );

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
typedef
EFI_STATUS
(EFIAPI * EDKII_CAPSULE_UPDATE_POLICY_CHECK_SYSTEM_THERMAL) (
  IN  EDKII_CAPSULE_UPDATE_POLICY_PROTOCOL  *This,
  OUT BOOLEAN                               *Good
  );

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
typedef
EFI_STATUS
(EFIAPI * EDKII_CAPSULE_UPDATE_POLICY_CHECK_SYSTEM_ENVIRONMENT) (
  IN  EDKII_CAPSULE_UPDATE_POLICY_PROTOCOL  *This,
  OUT BOOLEAN                               *Good
  );

/**
  Determines if the Lowest Supported Version checks should be performed.  The
  expected result from this function is TRUE.  A platform can choose to return
  FALSE (e.g. during manufacturing or servicing) to allow a capsule update to a
  version below the current Lowest Supported Version.

  @param[in]  This  A pointer to the EDKII_CAPSULE_UPDATE_POLICY_PROTOCOL instance.

  @retval TRUE   The lowest supported version check is required.
  @retval FALSE  Do not perform lowest support version check.

**/
typedef
BOOLEAN
(EFIAPI * EDKII_CAPSULE_UPDATE_POLICY_IS_LOWEST_SUPPORTED_VERSION_CHECK_REQUIRED) (
  IN  EDKII_CAPSULE_UPDATE_POLICY_PROTOCOL  *This
  );

/**
  Determines if the FMP device should be locked when the event specified by
  PcdFmpDeviceLockEventGuid is signaled. The expected result from this function
  is TRUE so the FMP device is always locked.  A platform can choose to return
  FALSE (e.g. during manufacturing) to allow FMP devices to remain unlocked.

  @param[in]  This  A pointer to the EDKII_CAPSULE_UPDATE_POLICY_PROTOCOL instance.

  @retval TRUE   The FMP device lock action is required at lock event guid.
  @retval FALSE  Do not perform FMP device lock at lock event guid.

**/
typedef
BOOLEAN
(EFIAPI * EDKII_CAPSULE_UPDATE_POLICY_IS_FMP_DEVICE_AT_LOCK_EVENT_REQUIRED) (
  IN  EDKII_CAPSULE_UPDATE_POLICY_PROTOCOL  *This
  );

///
/// This protocol provides platform policy services used during a capsule update.
///
struct _EDKII_CAPSULE_UPDATE_POLICY_PROTOCOL {
  EDKII_CAPSULE_UPDATE_POLICY_CHECK_SYSTEM_POWER                          CheckSystemPower;
  EDKII_CAPSULE_UPDATE_POLICY_CHECK_SYSTEM_THERMAL                        CheckSystemThermal;
  EDKII_CAPSULE_UPDATE_POLICY_CHECK_SYSTEM_ENVIRONMENT                    CheckSystemEnvironment;
  EDKII_CAPSULE_UPDATE_POLICY_IS_LOWEST_SUPPORTED_VERSION_CHECK_REQUIRED  IsLowestSupportedVersionCheckRequired;
  EDKII_CAPSULE_UPDATE_POLICY_IS_FMP_DEVICE_AT_LOCK_EVENT_REQUIRED        IsLockFmpDeviceAtLockEventGuidRequired;
};

extern EFI_GUID gEdkiiCapsuleUpdatePolicyProtocolGuid;

#endif
