/** @file
  EDKII Device Security library for SPDM device.
  It follows the SPDM Specification.

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SpdmSecurityLibInternal.h"

/**
  The device driver uses this service to authenticate and measure an SPDM device.

  @param[in]  SpdmDeviceInfo            The SPDM context for the device.
  @param[in]  SecurityPolicy            The security policy of this device.
  @param[out] SecurityState             A pointer to security state if this device.

  @retval EFI_SUCCESS   The TCG SPDM device measurement context is returned.
  @retval EFI_UNSUPPORTED  The TCG SPDM device measurement context is unsupported.

**/
EFI_STATUS
EFIAPI
SpdmDeviceAuthenticationAndMeasurement (
  IN  EDKII_SPDM_DEVICE_INFO        *SpdmDeviceInfo,
  IN  EDKII_DEVICE_SECURITY_POLICY  *SecurityPolicy,
  OUT EDKII_DEVICE_SECURITY_STATE   *SecurityState
  )
{
  EFI_STATUS           Status;
  SPDM_DEVICE_CONTEXT  *SpdmDeviceContext;
  BOOLEAN              IsAuthenticated;
  UINT8                AuthState;
  UINT8                SlotId;

  SpdmDeviceContext = CreateSpdmDeviceContext (SpdmDeviceInfo);
  if (SpdmDeviceContext == NULL) {
    return EFI_UNSUPPORTED;
  }

  IsAuthenticated = FALSE;
  AuthState       = TCG_DEVICE_SECURITY_EVENT_DATA_DEVICE_AUTH_STATE_SUCCESS;
  SlotId          = 0;
  if ((SecurityPolicy->AuthenticationPolicy & EDKII_DEVICE_AUTHENTICATION_REQUIRED) != 0) {
    Status = DoDeviceAuthentication (SpdmDeviceContext, &AuthState, &SlotId, SecurityState);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "DoDeviceAuthentication failed - %r\n", Status));
      return Status;
    } else {
      if (AuthState == TCG_DEVICE_SECURITY_EVENT_DATA_DEVICE_AUTH_STATE_FAIL_NO_SIG) {
        return Status;
      } else {
        IsAuthenticated = TRUE;
      }
    }
  }

  if ((SecurityPolicy->MeasurementPolicy & EDKII_DEVICE_MEASUREMENT_REQUIRED) != 0) {
    Status = DoDeviceMeasurement (SpdmDeviceContext, IsAuthenticated, SlotId, SecurityState);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "DoDeviceMeasurement failed - %r\n", Status));
    }
  }

  DestroySpdmDeviceContext (SpdmDeviceContext);

  return EFI_SUCCESS;
}

/**
  This function will get SpdmIoProtocol via Context.

  @param[in]   SpdmContext   The SPDM context for the device.

  return the pointer of Spdm Io protocol

**/
VOID *
EFIAPI
SpdmGetIoProtocolViaSpdmContext (
  IN VOID  *SpdmContext
  )
{
  return GetSpdmIoProtocolViaSpdmContext (SpdmContext);
}
