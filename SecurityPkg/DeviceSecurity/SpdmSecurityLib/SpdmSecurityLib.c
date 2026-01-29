/** @file
  EDKII Device Security library for SPDM device.
  It follows the SPDM Specification.

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SpdmSecurityLibInternal.h"

/**
  Helper function to quickly determine whether device authentication boot is enabled.

  @retval     TRUE    device authentication boot is verifiably enabled.
  @retval     FALSE   device authentication boot is either disabled or an error prevented checking.

**/
BOOLEAN
EFIAPI
IsDeviceAuthBootEnabled (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT8       *DeviceAuthBootMode;

  DeviceAuthBootMode = NULL;

  Status = GetEfiGlobalVariable2 (EFI_DEVICE_AUTH_BOOT_MODE_NAME, (VOID **)&DeviceAuthBootMode, NULL);
  //
  // Skip verification if DeviceAuthBootMode variable doesn't exist.
  //
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Cannot check DeviceAuthBootMode variable %r \n ", Status));
    return FALSE;
  }

  //
  // Skip verification if DeviceAuthBootMode is disabled but not AuditMode
  //
  if (*DeviceAuthBootMode == DEVICE_AUTH_BOOT_MODE_DISABLE) {
    FreePool (DeviceAuthBootMode);
    return FALSE;
  } else {
    FreePool (DeviceAuthBootMode);
    return TRUE;
  }
}

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
  UINT8                AuthState;
  UINT8                SlotId;
  BOOLEAN              IsValidCertChain;
  BOOLEAN              RootCertMatch;

  if ((PcdGet32 (PcdTcgPfpMeasurementRevision) < TCG_EfiSpecIDEventStruct_SPEC_ERRATA_TPM2_REV_106) ||
      (PcdGet8 (PcdEnableSpdmDeviceAuthentication) == 0))
  {
    return EFI_UNSUPPORTED;
  }

  SpdmDeviceContext = CreateSpdmDeviceContext (SpdmDeviceInfo, SecurityState);
  if (SpdmDeviceContext == NULL) {
    return EFI_UNSUPPORTED;
  }

  Status           = EFI_SUCCESS;
  AuthState        = TCG_DEVICE_SECURITY_EVENT_DATA_DEVICE_AUTH_STATE_SUCCESS;
  SlotId           = 0;
  IsValidCertChain = FALSE;
  RootCertMatch    = FALSE;

  if (((SecurityPolicy->AuthenticationPolicy & EDKII_DEVICE_AUTHENTICATION_REQUIRED) != 0) ||
      ((SecurityPolicy->MeasurementPolicy & EDKII_DEVICE_MEASUREMENT_REQUIRED) != 0))
  {
    Status = DoDeviceCertificate (SpdmDeviceContext, &AuthState, &SlotId, SecurityState, &IsValidCertChain, &RootCertMatch);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "DoDeviceCertificate failed - %r\n", Status));
      goto Ret;
    } else if ((AuthState == TCG_DEVICE_SECURITY_EVENT_DATA_DEVICE_AUTH_STATE_FAIL_NO_SIG) ||
               (AuthState == TCG_DEVICE_SECURITY_EVENT_DATA_DEVICE_AUTH_STATE_FAIL_INVALID))
    {
      goto Ret;
    }
  }

  if (((SecurityPolicy->AuthenticationPolicy & EDKII_DEVICE_AUTHENTICATION_REQUIRED) != 0) && (IsDeviceAuthBootEnabled ())) {
    Status = DoDeviceAuthentication (SpdmDeviceContext, &AuthState, SlotId, IsValidCertChain, RootCertMatch, SecurityState);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "DoDeviceAuthentication failed - %r\n", Status));
      goto Ret;
    } else if ((AuthState == TCG_DEVICE_SECURITY_EVENT_DATA_DEVICE_AUTH_STATE_FAIL_NO_SIG) ||
               (AuthState == TCG_DEVICE_SECURITY_EVENT_DATA_DEVICE_AUTH_STATE_FAIL_INVALID))
    {
      goto Ret;
    }
  }

  if ((SecurityPolicy->MeasurementPolicy & EDKII_DEVICE_MEASUREMENT_REQUIRED) != 0) {
    Status = DoDeviceMeasurement (SpdmDeviceContext, SlotId, SecurityState);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "DoDeviceMeasurement failed - %r\n", Status));
    }
  }

Ret:
  DestroySpdmDeviceContext (SpdmDeviceContext);

  return Status;
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
