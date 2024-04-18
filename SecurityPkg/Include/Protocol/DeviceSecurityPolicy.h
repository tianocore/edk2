/** @file
  Platform Device Security Policy Protocol definition

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef EDKII_DEVICE_SECURITY_POLICY_PROTOCOL_H_
#define EDKII_DEVICE_SECURITY_POLICY_PROTOCOL_H_

#include <Uefi.h>
#include <Protocol/DeviceSecurity.h>

typedef struct _EDKII_DEVICE_SECURITY_POLICY_PROTOCOL EDKII_DEVICE_SECURITY_POLICY_PROTOCOL;

//
// Revision The revision to which the DEVICE_SECURITY_POLICY protocol interface adheres.
//          All future revisions must be backwards compatible.
//          If a future version is not back wards compatible it is not the same GUID.
//
#define EDKII_DEVICE_SECURITY_POLICY_PROTOCOL_REVISION  0x00010000

//
// Revision The revision to which the DEVICE_SECURITY_POLICY structure adheres.
//          All future revisions must be backwards compatible.
//
#define EDKII_DEVICE_SECURITY_POLICY_REVISION  0x00010000

///
/// The macro for the policy defined in EDKII_DEVICE_SECURITY_POLICY
///
#define EDKII_DEVICE_MEASUREMENT_REQUIRED     BIT0
#define EDKII_DEVICE_AUTHENTICATION_REQUIRED  BIT0

///
/// The device security policy data structure
///
typedef struct {
  UINT32    Revision;
  UINT32    MeasurementPolicy;
  UINT32    AuthenticationPolicy;
} EDKII_DEVICE_SECURITY_POLICY;

//
// Revision The revision to which the DEVICE_SECURITY_STATE structure adheres.
//          All future revisions must be backwards compatible.
//
#define EDKII_DEVICE_SECURITY_STATE_REVISION  0x00010000

///
/// The macro for the state defined in EDKII_DEVICE_SECURITY_STATE
///
#define EDKII_DEVICE_SECURITY_STATE_SUCCESS                         0
#define EDKII_DEVICE_SECURITY_STATE_ERROR                           BIT31
#define EDKII_DEVICE_SECURITY_STATE_ERROR_UEFI_UNSUPPORTED          (EDKII_DEVICE_SECURITY_STATE_ERROR + 0x0)
#define EDKII_DEVICE_SECURITY_STATE_ERROR_UEFI_GET_POLICY_PROTOCOL  (EDKII_DEVICE_SECURITY_STATE_ERROR + 0x1)
#define EDKII_DEVICE_SECURITY_STATE_ERROR_UEFI_OUT_OF_RESOURCE      (EDKII_DEVICE_SECURITY_STATE_ERROR + 0x2)
#define EDKII_DEVICE_SECURITY_STATE_ERROR_DEVICE_NO_CAPABILITIES    (EDKII_DEVICE_SECURITY_STATE_ERROR + 0x10)
#define EDKII_DEVICE_SECURITY_STATE_ERROR_DEVICE_ERROR              (EDKII_DEVICE_SECURITY_STATE_ERROR + 0x11)
#define EDKII_DEVICE_SECURITY_STATE_ERROR_TCG_EXTEND_TPM_PCR        (EDKII_DEVICE_SECURITY_STATE_ERROR + 0x20)
#define EDKII_DEVICE_SECURITY_STATE_ERROR_MEASUREMENT_AUTH_FAILURE  (EDKII_DEVICE_SECURITY_STATE_ERROR + 0x21)
#define EDKII_DEVICE_SECURITY_STATE_ERROR_CHALLENGE_FAILURE         (EDKII_DEVICE_SECURITY_STATE_ERROR + 0x30)
#define EDKII_DEVICE_SECURITY_STATE_ERROR_CERTIFIACTE_FAILURE       (EDKII_DEVICE_SECURITY_STATE_ERROR + 0x31)
#define EDKII_DEVICE_SECURITY_STATE_ERROR_NO_CERT_PROVISION         (EDKII_DEVICE_SECURITY_STATE_ERROR + 0x32)

///
/// The device security state data structure
///
typedef struct {
  UINT32    Revision;
  UINT32    MeasurementState;
  UINT32    AuthenticationState;
} EDKII_DEVICE_SECURITY_STATE;

/**
  This function returns the device security policy associated with the device.

  The device security driver may call this interface to get the platform policy
  for the specific device and determine if the measurement or authentication
  is required.

  @param[in]  This                   The protocol instance pointer.
  @param[in]  DeviceId               The Identifier for the device.
  @param[out] DeviceSecurityPolicy   The Device Security Policy associated with the device.

  @retval EFI_SUCCESS                The device security policy is returned
  @retval EFI_UNSUPPORTED            The function is unsupported for the specific Device.
**/
typedef
  EFI_STATUS
(EFIAPI *EDKII_DEVICE_SECURITY_GET_DEVICE_POLICY)(
  IN  EDKII_DEVICE_SECURITY_POLICY_PROTOCOL  *This,
  IN  EDKII_DEVICE_IDENTIFIER                *DeviceId,
  OUT EDKII_DEVICE_SECURITY_POLICY           *DeviceSecurityPolicy
  );

/**
  This function sets the device state based upon the authentication result.

  The device security driver may call this interface to give the platform
  a notify based upon the measurement or authentication result.
  If the authentication or measurement fails, the platform may choose:
  1) Do nothing.
  2) Disable this device or slot temporarily and continue boot.
  3) Reset the platform and retry again.
  4) Disable this device or slot permanently.
  5) Any other platform specific action.

  @param[in]  This                   The protocol instance pointer.
  @param[in]  DeviceId               The Identifier for the device.
  @param[in]  DeviceSecurityState    The Device Security state associated with the device.

  @retval EFI_SUCCESS                The device state is set.
  @retval EFI_UNSUPPORTED            The function is unsupported for the specific Device.
**/
typedef
  EFI_STATUS
(EFIAPI *EDKII_DEVICE_SECURITY_NOTIFY_DEVICE_STATE)(
  IN  EDKII_DEVICE_SECURITY_POLICY_PROTOCOL  *This,
  IN  EDKII_DEVICE_IDENTIFIER                *DeviceId,
  IN  EDKII_DEVICE_SECURITY_STATE            *DeviceSecurityState
  );

struct _EDKII_DEVICE_SECURITY_POLICY_PROTOCOL {
  UINT32                                       Revision;
  EDKII_DEVICE_SECURITY_GET_DEVICE_POLICY      GetDevicePolicy;
  EDKII_DEVICE_SECURITY_NOTIFY_DEVICE_STATE    NotifyDeviceState;
};

extern EFI_GUID  gEdkiiDeviceSecurityPolicyProtocolGuid;

#endif
