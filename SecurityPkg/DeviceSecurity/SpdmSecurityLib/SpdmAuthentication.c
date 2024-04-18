/** @file
  EDKII Device Security library for SPDM device.
  It follows the SPDM Specification.

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SpdmSecurityLibInternal.h"

/**
  Measure and log an EFI variable, and extend the measurement result into a specific PCR.

  @param[in]  PcrIndex          PCR Index.
  @param[in]  EventType         Event type.
  @param[in]  VarName           A Null-terminated string that is the name of the vendor's variable.
  @param[in]  VendorGuid        A unique identifier for the vendor.
  @param[in]  VarData           The content of the variable data.
  @param[in]  VarSize           The size of the variable data.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_DEVICE_ERROR      The operation was unsuccessful.

**/
EFI_STATUS
EFIAPI
MeasureVariable (
  IN      UINT32    PcrIndex,
  IN      UINT32    EventType,
  IN      CHAR16    *VarName,
  IN      EFI_GUID  *VendorGuid,
  IN      VOID      *VarData,
  IN      UINTN     VarSize
  )
{
  EFI_STATUS          Status;
  UINTN               VarNameLength;
  UEFI_VARIABLE_DATA  *VarLog;
  UINT32              VarLogSize;

  if (!(((VarSize == 0) && (VarData == NULL)) || ((VarSize != 0) && (VarData != NULL)))) {
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
    return EFI_INVALID_PARAMETER;
  }

  VarNameLength = StrLen (VarName);
  VarLogSize    = (UINT32)(sizeof (*VarLog) + VarNameLength * sizeof (*VarName) + VarSize
                           - sizeof (VarLog->UnicodeName) - sizeof (VarLog->VariableData));

  VarLog = (UEFI_VARIABLE_DATA *)AllocateZeroPool (VarLogSize);
  if (VarLog == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (&VarLog->VariableName, VendorGuid, sizeof (VarLog->VariableName));
  VarLog->UnicodeNameLength  = VarNameLength;
  VarLog->VariableDataLength = VarSize;
  CopyMem (
    VarLog->UnicodeName,
    VarName,
    VarNameLength * sizeof (*VarName)
    );
  if (VarSize != 0) {
    CopyMem (
      (CHAR16 *)VarLog->UnicodeName + VarNameLength,
      VarData,
      VarSize
      );
  }

  DEBUG ((DEBUG_INFO, "VariableDxe: MeasureVariable (Pcr - %x, EventType - %x, ", (UINTN)PcrIndex, (UINTN)EventType));
  DEBUG ((DEBUG_INFO, "VariableName - %s, VendorGuid - %g)\n", VarName, VendorGuid));

  Status = TpmMeasureAndLogData (
             PcrIndex,
             EventType,
             VarLog,
             VarLogSize,
             VarLog,
             VarLogSize
             );
  FreePool (VarLog);
  return Status;
}

/**
  Extend Certicate and auth state to NV Index and measure trust anchor to PCR.

  @param[in]  SpdmDeviceContext          The SPDM context for the device.
  @param[in]  AuthState                  The auth state of this deice.
  @param[in]  CertChainSize              The size of cert chain.
  @param[in]  CertChain                  A pointer to a destination buffer to store the certificate chain.
  @param[in]  TrustAnchor                A buffer to hold the trust_anchor which is used to validate the peer
                                         certificate, if not NULL.
  @param[in]  TrustAnchorSize            A buffer to hold the trust_anchor_size, if not NULL..
  @param[in]  SlotId                     The number of slot for the certificate chain.
  @param[out]  SecurityState             A pointer to the security state of the requester.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_DEVICE_ERROR      The operation was unsuccessful.

**/
EFI_STATUS
ExtendCertificate (
  IN  SPDM_DEVICE_CONTEXT          *SpdmDeviceContext,
  IN UINT8                         AuthState,
  IN UINTN                         CertChainSize,
  IN UINT8                         *CertChain,
  IN VOID                          *TrustAnchor,
  IN UINTN                         TrustAnchorSize,
  IN UINT8                         SlotId,
  OUT EDKII_DEVICE_SECURITY_STATE  *SecurityState
  )
{
  VOID                                                       *EventLog;
  UINT32                                                     EventLogSize;
  UINT8                                                      *EventLogPtr;
  TCG_NV_INDEX_INSTANCE_EVENT_LOG_STRUCT                     *NvIndexInstance;
  TCG_DEVICE_SECURITY_EVENT_DATA_HEADER2                     *EventData2;
  TCG_DEVICE_SECURITY_EVENT_DATA_SUB_HEADER_SPDM_CERT_CHAIN  *TcgSpdmCertChain;
  VOID                                                       *DeviceContext;
  UINTN                                                      DeviceContextSize;
  EFI_STATUS                                                 Status;
  UINTN                                                      DevicePathSize;
  UINT32                                                     BaseHashAlgo;
  UINTN                                                      DataSize;
  VOID                                                       *SpdmContext;
  SPDM_DATA_PARAMETER                                        Parameter;
  EFI_SIGNATURE_DATA                                         *SignatureData;
  UINTN                                                      SignatureDataSize;

  SpdmContext = SpdmDeviceContext->SpdmContext;

  EventLog = NULL;
  ZeroMem (&Parameter, sizeof (Parameter));
  Parameter.location = SpdmDataLocationConnection;
  DataSize           = sizeof (BaseHashAlgo);
  Status             = SpdmGetData (SpdmContext, SpdmDataBaseHashAlgo, &Parameter, &BaseHashAlgo, &DataSize);
  ASSERT_EFI_ERROR (Status);

  DeviceContextSize = GetDeviceMeasurementContextSize (SpdmDeviceContext);
  DevicePathSize    = GetDevicePathSize (SpdmDeviceContext->DevicePath);

  switch (AuthState) {
    case TCG_DEVICE_SECURITY_EVENT_DATA_DEVICE_AUTH_STATE_SUCCESS:
    case TCG_DEVICE_SECURITY_EVENT_DATA_DEVICE_AUTH_STATE_NO_AUTH:
    case TCG_DEVICE_SECURITY_EVENT_DATA_DEVICE_AUTH_STATE_NO_BINDING:
      EventLogSize = (UINT32)(sizeof (TCG_NV_INDEX_INSTANCE_EVENT_LOG_STRUCT) +
                              sizeof (TCG_DEVICE_SECURITY_EVENT_DATA_HEADER2) +
                              sizeof (UINT64) + DevicePathSize +
                              sizeof (TCG_DEVICE_SECURITY_EVENT_DATA_SUB_HEADER_SPDM_CERT_CHAIN) +
                              CertChainSize +
                              DeviceContextSize);
      EventLog = AllocatePool (EventLogSize);
      if (EventLog == NULL) {
        SecurityState->AuthenticationState = EDKII_DEVICE_SECURITY_STATE_ERROR_UEFI_OUT_OF_RESOURCE;
        return EFI_OUT_OF_RESOURCES;
      }

      EventLogPtr = EventLog;

      NvIndexInstance = (VOID *)EventLogPtr;
      CopyMem (NvIndexInstance->Signature, TCG_NV_EXTEND_INDEX_FOR_INSTANCE_SIGNATURE, sizeof (TCG_NV_EXTEND_INDEX_FOR_INSTANCE_SIGNATURE));
      NvIndexInstance->Version = TCG_NV_INDEX_INSTANCE_EVENT_LOG_STRUCT_VERSION;
      ZeroMem (NvIndexInstance->Reserved, sizeof (NvIndexInstance->Reserved));
      EventLogPtr += sizeof (TCG_NV_INDEX_INSTANCE_EVENT_LOG_STRUCT);

      EventData2 = (VOID *)EventLogPtr;
      CopyMem (EventData2->Signature, TCG_DEVICE_SECURITY_EVENT_DATA_SIGNATURE_2, sizeof (EventData2->Signature));
      EventData2->Version    = TCG_DEVICE_SECURITY_EVENT_DATA_VERSION_2;
      EventData2->AuthState  = AuthState;
      EventData2->Reserved   = 0;
      EventData2->Length     = (UINT32)EventLogSize;
      EventData2->DeviceType = GetSpdmDeviceType (SpdmDeviceContext);

      EventData2->SubHeaderType   = TCG_DEVICE_SECURITY_EVENT_DATA_DEVICE_SUB_HEADER_TYPE_SPDM_CERT_CHAIN;
      EventData2->SubHeaderLength = (UINT32)(sizeof (TCG_DEVICE_SECURITY_EVENT_DATA_SUB_HEADER_SPDM_CERT_CHAIN) + CertChainSize);
      EventData2->SubHeaderUID    = SpdmDeviceContext->DeviceUID;

      EventLogPtr = (VOID *)(EventData2 + 1);

      *(UINT64 *)EventLogPtr = (UINT64)DevicePathSize;
      EventLogPtr           += sizeof (UINT64);
      CopyMem (EventLogPtr, SpdmDeviceContext->DevicePath, DevicePathSize);
      EventLogPtr += DevicePathSize;

      TcgSpdmCertChain               = (VOID *)EventLogPtr;
      TcgSpdmCertChain->SpdmVersion  = SpdmDeviceContext->SpdmVersion;
      TcgSpdmCertChain->SpdmSlotId   = SlotId;
      TcgSpdmCertChain->Reserved     = 0;
      TcgSpdmCertChain->SpdmHashAlgo = BaseHashAlgo;
      EventLogPtr                   += sizeof (TCG_DEVICE_SECURITY_EVENT_DATA_SUB_HEADER_SPDM_CERT_CHAIN);

      CopyMem (EventLogPtr, CertChain, CertChainSize);
      EventLogPtr += CertChainSize;

      if (DeviceContextSize != 0) {
        DeviceContext = (VOID *)EventLogPtr;
        Status        = CreateDeviceMeasurementContext (SpdmDeviceContext, DeviceContext, DeviceContextSize);
        if (Status != EFI_SUCCESS) {
          SecurityState->AuthenticationState = EDKII_DEVICE_SECURITY_STATE_ERROR_DEVICE_ERROR;
          Status                             = EFI_DEVICE_ERROR;
          goto Exit;
        }
      }

      Status = TpmMeasureAndLogData (
                 TCG_NV_EXTEND_INDEX_FOR_INSTANCE,
                 EV_NO_ACTION,
                 EventLog,
                 EventLogSize,
                 EventLog,
                 EventLogSize
                 );
      if (EFI_ERROR (Status)) {
        SecurityState->AuthenticationState = EDKII_DEVICE_SECURITY_STATE_ERROR_TCG_EXTEND_TPM_PCR;
      }

      DEBUG ((DEBUG_INFO, "TpmMeasureAndLogData (Instance) - %r\n", Status));

      break;
    case TCG_DEVICE_SECURITY_EVENT_DATA_DEVICE_AUTH_STATE_FAIL_INVALID:
      EventLogSize = (UINT32)(sizeof (TCG_NV_INDEX_INSTANCE_EVENT_LOG_STRUCT) +
                              sizeof (TCG_DEVICE_SECURITY_EVENT_DATA_HEADER2) +
                              sizeof (UINT64) + DevicePathSize +
                              sizeof (TCG_DEVICE_SECURITY_EVENT_DATA_SUB_HEADER_SPDM_CERT_CHAIN) +
                              DeviceContextSize);
      EventLog = AllocatePool (EventLogSize);
      if (EventLog == NULL) {
        SecurityState->AuthenticationState = EDKII_DEVICE_SECURITY_STATE_ERROR_UEFI_OUT_OF_RESOURCE;
        return EFI_OUT_OF_RESOURCES;
      }

      EventLogPtr = EventLog;

      NvIndexInstance = (VOID *)EventLogPtr;
      CopyMem (NvIndexInstance->Signature, TCG_NV_EXTEND_INDEX_FOR_INSTANCE_SIGNATURE, sizeof (TCG_NV_EXTEND_INDEX_FOR_INSTANCE_SIGNATURE));
      NvIndexInstance->Version = TCG_NV_INDEX_INSTANCE_EVENT_LOG_STRUCT_VERSION;
      ZeroMem (NvIndexInstance->Reserved, sizeof (NvIndexInstance->Reserved));
      EventLogPtr += sizeof (TCG_NV_INDEX_INSTANCE_EVENT_LOG_STRUCT);

      EventData2 = (VOID *)EventLogPtr;
      CopyMem (EventData2->Signature, TCG_DEVICE_SECURITY_EVENT_DATA_SIGNATURE_2, sizeof (EventData2->Signature));
      EventData2->Version    = TCG_DEVICE_SECURITY_EVENT_DATA_VERSION_2;
      EventData2->AuthState  = AuthState;
      EventData2->Reserved   = 0;
      EventData2->Length     = (UINT32)EventLogSize;
      EventData2->DeviceType = GetSpdmDeviceType (SpdmDeviceContext);

      EventData2->SubHeaderType   = TCG_DEVICE_SECURITY_EVENT_DATA_DEVICE_SUB_HEADER_TYPE_SPDM_CERT_CHAIN;
      EventData2->SubHeaderLength = sizeof (TCG_DEVICE_SECURITY_EVENT_DATA_SUB_HEADER_SPDM_CERT_CHAIN);
      EventData2->SubHeaderUID    = SpdmDeviceContext->DeviceUID;

      EventLogPtr = (VOID *)(EventData2 + 1);

      *(UINT64 *)EventLogPtr = (UINT64)DevicePathSize;
      EventLogPtr           += sizeof (UINT64);
      CopyMem (EventLogPtr, SpdmDeviceContext->DevicePath, DevicePathSize);
      EventLogPtr += DevicePathSize;

      TcgSpdmCertChain               = (VOID *)EventLogPtr;
      TcgSpdmCertChain->SpdmVersion  = SpdmDeviceContext->SpdmVersion;
      TcgSpdmCertChain->SpdmSlotId   = SlotId;
      TcgSpdmCertChain->Reserved     = 0;
      TcgSpdmCertChain->SpdmHashAlgo = BaseHashAlgo;
      EventLogPtr                   += sizeof (TCG_DEVICE_SECURITY_EVENT_DATA_SUB_HEADER_SPDM_CERT_CHAIN);

      if (DeviceContextSize != 0) {
        DeviceContext = (VOID *)EventLogPtr;
        Status        = CreateDeviceMeasurementContext (SpdmDeviceContext, DeviceContext, DeviceContextSize);
        if (Status != EFI_SUCCESS) {
          SecurityState->AuthenticationState = EDKII_DEVICE_SECURITY_STATE_ERROR_DEVICE_ERROR;
          Status                             = EFI_DEVICE_ERROR;
          goto Exit;
        }
      }

      Status = TpmMeasureAndLogData (
                 TCG_NV_EXTEND_INDEX_FOR_INSTANCE,
                 EV_NO_ACTION,
                 EventLog,
                 EventLogSize,
                 EventLog,
                 EventLogSize
                 );
      if (EFI_ERROR (Status)) {
        SecurityState->AuthenticationState = EDKII_DEVICE_SECURITY_STATE_ERROR_TCG_EXTEND_TPM_PCR;
      }

      DEBUG ((DEBUG_INFO, "TpmMeasureAndLogData (Instance) - %r\n", Status));

      goto Exit;
    case TCG_DEVICE_SECURITY_EVENT_DATA_DEVICE_AUTH_STATE_FAIL_NO_SIG:
    case TCG_DEVICE_SECURITY_EVENT_DATA_DEVICE_AUTH_STATE_NO_SPDM:
      EventLogSize = (UINT32)(sizeof (TCG_NV_INDEX_INSTANCE_EVENT_LOG_STRUCT) +
                              sizeof (TCG_DEVICE_SECURITY_EVENT_DATA_HEADER2) +
                              sizeof (UINT64) + DevicePathSize +
                              DeviceContextSize);
      EventLog = AllocatePool (EventLogSize);
      if (EventLog == NULL) {
        SecurityState->AuthenticationState = EDKII_DEVICE_SECURITY_STATE_ERROR_UEFI_OUT_OF_RESOURCE;
        return EFI_OUT_OF_RESOURCES;
      }

      EventLogPtr = EventLog;

      NvIndexInstance = (VOID *)EventLogPtr;
      CopyMem (NvIndexInstance->Signature, TCG_NV_EXTEND_INDEX_FOR_INSTANCE_SIGNATURE, sizeof (TCG_NV_EXTEND_INDEX_FOR_INSTANCE_SIGNATURE));
      NvIndexInstance->Version = TCG_NV_INDEX_INSTANCE_EVENT_LOG_STRUCT_VERSION;
      ZeroMem (NvIndexInstance->Reserved, sizeof (NvIndexInstance->Reserved));
      EventLogPtr += sizeof (TCG_NV_INDEX_INSTANCE_EVENT_LOG_STRUCT);

      EventData2 = (VOID *)EventLogPtr;
      CopyMem (EventData2->Signature, TCG_DEVICE_SECURITY_EVENT_DATA_SIGNATURE_2, sizeof (EventData2->Signature));
      EventData2->Version    = TCG_DEVICE_SECURITY_EVENT_DATA_VERSION_2;
      EventData2->AuthState  = AuthState;
      EventData2->Reserved   = 0;
      EventData2->Length     = (UINT32)EventLogSize;
      EventData2->DeviceType = GetSpdmDeviceType (SpdmDeviceContext);

      EventData2->SubHeaderType   = TCG_DEVICE_SECURITY_EVENT_DATA_DEVICE_SUB_HEADER_TYPE_SPDM_CERT_CHAIN;
      EventData2->SubHeaderLength = 0;
      EventData2->SubHeaderUID    = SpdmDeviceContext->DeviceUID;

      EventLogPtr = (VOID *)(EventData2 + 1);

      *(UINT64 *)EventLogPtr = (UINT64)DevicePathSize;
      EventLogPtr           += sizeof (UINT64);
      CopyMem (EventLogPtr, SpdmDeviceContext->DevicePath, DevicePathSize);
      EventLogPtr += DevicePathSize;

      if (DeviceContextSize != 0) {
        DeviceContext = (VOID *)EventLogPtr;
        Status        = CreateDeviceMeasurementContext (SpdmDeviceContext, DeviceContext, DeviceContextSize);
        if (Status != EFI_SUCCESS) {
          SecurityState->AuthenticationState = EDKII_DEVICE_SECURITY_STATE_ERROR_DEVICE_ERROR;
          Status                             = EFI_DEVICE_ERROR;
          goto Exit;
        }
      }

      Status = TpmMeasureAndLogData (
                 TCG_NV_EXTEND_INDEX_FOR_INSTANCE,
                 EV_NO_ACTION,
                 EventLog,
                 EventLogSize,
                 EventLog,
                 EventLogSize
                 );
      if (EFI_ERROR (Status)) {
        SecurityState->AuthenticationState = EDKII_DEVICE_SECURITY_STATE_ERROR_TCG_EXTEND_TPM_PCR;
      }

      DEBUG ((DEBUG_INFO, "TpmMeasureAndLogData (Instance) - %r\n", Status));

      goto Exit;
    default:
      SecurityState->AuthenticationState = EDKII_DEVICE_SECURITY_STATE_ERROR_UEFI_UNSUPPORTED;
      return EFI_UNSUPPORTED;
  }

  if ((TrustAnchor != NULL) && (TrustAnchorSize != 0)) {
    SignatureDataSize = sizeof (EFI_GUID) + TrustAnchorSize;
    SignatureData     = AllocateZeroPool (SignatureDataSize);
    if (SignatureData == NULL) {
      ASSERT (SignatureData != NULL);
      SecurityState->AuthenticationState = EDKII_DEVICE_SECURITY_STATE_ERROR_UEFI_OUT_OF_RESOURCE;
      Status                             = EFI_OUT_OF_RESOURCES;
      goto Exit;
    }

    CopyGuid (&SignatureData->SignatureOwner, &gEfiCallerIdGuid);
    CopyMem (
      (UINT8 *)SignatureData + sizeof (EFI_GUID),
      TrustAnchor,
      TrustAnchorSize
      );

    MeasureVariable (
      PCR_INDEX_FOR_SIGNATURE_DB,
      EV_EFI_SPDM_DEVICE_AUTHORITY,
      EFI_DEVICE_SECURITY_DATABASE,
      &gEfiDeviceSignatureDatabaseGuid,
      SignatureData,
      SignatureDataSize
      );
    FreePool (SignatureData);
  }

Exit:
  if (EventLog != NULL) {
    FreePool (EventLog);
  }

  return Status;
}

/**
  Measure and log Auth state and Requester and responder Nonce into NV Index.

  @param[in]  SpdmDeviceContext        The SPDM context for the device.
  @param[in]  AuthState                The auth state of this deice.
  @param[in]  RequesterNonce           A buffer to hold the requester nonce (32 bytes), if not NULL.
  @param[in]  ResponderNonce           A buffer to hold the responder nonce (32 bytes), if not NULL.
  @param[out]  SecurityState           A pointer to the security state of the requester.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_DEVICE_ERROR      The operation was unsuccessful.

**/
EFI_STATUS
ExtendAuthentication (
  IN  SPDM_DEVICE_CONTEXT          *SpdmDeviceContext,
  IN UINT8                         AuthState,
  IN UINT8                         *RequesterNonce,
  IN UINT8                         *ResponderNonce,
  OUT EDKII_DEVICE_SECURITY_STATE  *SecurityState
  )
{
  EFI_STATUS  Status;

  {
    TCG_NV_INDEX_DYNAMIC_EVENT_LOG_STRUCT_SPDM_CHALLENGE       DynamicEventLogSpdmChallengeEvent;
    TCG_NV_INDEX_DYNAMIC_EVENT_LOG_STRUCT_SPDM_CHALLENGE_AUTH  DynamicEventLogSpdmChallengeAuthEvent;

    CopyMem (DynamicEventLogSpdmChallengeEvent.Header.Signature, TCG_NV_EXTEND_INDEX_FOR_DYNAMIC_SIGNATURE, sizeof (TCG_NV_EXTEND_INDEX_FOR_DYNAMIC_SIGNATURE));
    DynamicEventLogSpdmChallengeEvent.Header.Version = TCG_NV_INDEX_DYNAMIC_EVENT_LOG_STRUCT_VERSION;
    ZeroMem (DynamicEventLogSpdmChallengeEvent.Header.Reserved, sizeof (DynamicEventLogSpdmChallengeEvent.Header.Reserved));
    DynamicEventLogSpdmChallengeEvent.Header.Uid      = SpdmDeviceContext->DeviceUID;
    DynamicEventLogSpdmChallengeEvent.DescriptionSize = sizeof (TCG_SPDM_CHALLENGE_DESCRIPTION);
    CopyMem (DynamicEventLogSpdmChallengeEvent.Description, TCG_SPDM_CHALLENGE_DESCRIPTION, sizeof (TCG_SPDM_CHALLENGE_DESCRIPTION));
    DynamicEventLogSpdmChallengeEvent.DataSize = SPDM_NONCE_SIZE;
    CopyMem (DynamicEventLogSpdmChallengeEvent.Data, RequesterNonce, SPDM_NONCE_SIZE);

    Status = TpmMeasureAndLogData (
               TCG_NV_EXTEND_INDEX_FOR_DYNAMIC,
               EV_NO_ACTION,
               &DynamicEventLogSpdmChallengeEvent,
               sizeof (DynamicEventLogSpdmChallengeEvent),
               &DynamicEventLogSpdmChallengeEvent,
               sizeof (DynamicEventLogSpdmChallengeEvent)
               );
    if (EFI_ERROR (Status)) {
      SecurityState->AuthenticationState = EDKII_DEVICE_SECURITY_STATE_ERROR_TCG_EXTEND_TPM_PCR;
    }

    DEBUG ((DEBUG_INFO, "TpmMeasureAndLogData (Dynamic) - %r\n", Status));

    CopyMem (DynamicEventLogSpdmChallengeAuthEvent.Header.Signature, TCG_NV_EXTEND_INDEX_FOR_DYNAMIC_SIGNATURE, sizeof (TCG_NV_EXTEND_INDEX_FOR_DYNAMIC_SIGNATURE));
    DynamicEventLogSpdmChallengeAuthEvent.Header.Version = TCG_NV_INDEX_DYNAMIC_EVENT_LOG_STRUCT_VERSION;
    ZeroMem (DynamicEventLogSpdmChallengeAuthEvent.Header.Reserved, sizeof (DynamicEventLogSpdmChallengeAuthEvent.Header.Reserved));
    DynamicEventLogSpdmChallengeAuthEvent.Header.Uid      = SpdmDeviceContext->DeviceUID;
    DynamicEventLogSpdmChallengeAuthEvent.DescriptionSize = sizeof (TCG_SPDM_CHALLENGE_AUTH_DESCRIPTION);
    CopyMem (DynamicEventLogSpdmChallengeAuthEvent.Description, TCG_SPDM_CHALLENGE_AUTH_DESCRIPTION, sizeof (TCG_SPDM_CHALLENGE_AUTH_DESCRIPTION));
    DynamicEventLogSpdmChallengeAuthEvent.DataSize = SPDM_NONCE_SIZE;
    CopyMem (DynamicEventLogSpdmChallengeAuthEvent.Data, ResponderNonce, SPDM_NONCE_SIZE);

    Status = TpmMeasureAndLogData (
               TCG_NV_EXTEND_INDEX_FOR_DYNAMIC,
               EV_NO_ACTION,
               &DynamicEventLogSpdmChallengeAuthEvent,
               sizeof (DynamicEventLogSpdmChallengeAuthEvent),
               &DynamicEventLogSpdmChallengeAuthEvent,
               sizeof (DynamicEventLogSpdmChallengeAuthEvent)
               );
    if (EFI_ERROR (Status)) {
      SecurityState->AuthenticationState = EDKII_DEVICE_SECURITY_STATE_ERROR_TCG_EXTEND_TPM_PCR;
    }

    DEBUG ((DEBUG_INFO, "TpmMeasureAndLogData (Dynamic) - %r\n", Status));
  }

  return Status;
}

/**
  This function gets SPDM digest and certificates.

  @param[in]  SpdmDeviceContext           The SPDM context for the device.
  @param[out]  AuthState                  The auth state of the devices.
  @param[out]  ValidSlotId                The number of slot for the certificate chain.
  @param[out]  SecurityState              The security state of the requester.
  @param[out]  IsValidCertChain           The validity of the certificate chain.
  @param[out]  RootCertMatch              The authority of the certificate chain.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_DEVICE_ERROR      The operation was unsuccessful.

**/
EFI_STATUS
EFIAPI
DoDeviceCertificate (
  IN  SPDM_DEVICE_CONTEXT          *SpdmDeviceContext,
  OUT UINT8                        *AuthState,
  OUT UINT8                        *ValidSlotId,
  OUT EDKII_DEVICE_SECURITY_STATE  *SecurityState,
  OUT BOOLEAN                      *IsValidCertChain,
  OUT BOOLEAN                      *RootCertMatch
  )
{
  EFI_STATUS           Status;
  SPDM_RETURN          SpdmReturn;
  VOID                 *SpdmContext;
  UINT32               CapabilityFlags;
  UINTN                DataSize;
  SPDM_DATA_PARAMETER  Parameter;
  UINT8                SlotMask;
  UINT8                TotalDigestBuffer[LIBSPDM_MAX_HASH_SIZE * SPDM_MAX_SLOT_COUNT];
  UINTN                CertChainSize;
  UINT8                CertChain[LIBSPDM_MAX_CERT_CHAIN_SIZE];
  VOID                 *TrustAnchor;
  UINTN                TrustAnchorSize;
  UINT8                SlotId;

  SpdmContext = SpdmDeviceContext->SpdmContext;

  ZeroMem (&Parameter, sizeof (Parameter));
  Parameter.location = SpdmDataLocationConnection;
  DataSize           = sizeof (CapabilityFlags);
  SpdmReturn         = SpdmGetData (SpdmContext, SpdmDataCapabilityFlags, &Parameter, &CapabilityFlags, &DataSize);
  if (LIBSPDM_STATUS_IS_ERROR (SpdmReturn)) {
    SecurityState->AuthenticationState = EDKII_DEVICE_SECURITY_STATE_ERROR_DEVICE_ERROR;
    return EFI_DEVICE_ERROR;
  }

  *IsValidCertChain = FALSE;
  *RootCertMatch    = FALSE;
  CertChainSize     = sizeof (CertChain);
  ZeroMem (CertChain, sizeof (CertChain));
  TrustAnchor     = NULL;
  TrustAnchorSize = 0;

  //
  // Init *ValidSlotId to invalid slot_id
  //
  *ValidSlotId = SPDM_MAX_SLOT_COUNT;

  if ((CapabilityFlags & SPDM_GET_CAPABILITIES_RESPONSE_FLAGS_CERT_CAP) == 0) {
    *AuthState                         = TCG_DEVICE_SECURITY_EVENT_DATA_DEVICE_AUTH_STATE_FAIL_NO_SIG;
    SecurityState->AuthenticationState = EDKII_DEVICE_SECURITY_STATE_ERROR_DEVICE_NO_CAPABILITIES;
    Status                             = ExtendCertificate (SpdmDeviceContext, *AuthState, 0, NULL, NULL, 0, 0, SecurityState);
    return Status;
  } else {
    ZeroMem (TotalDigestBuffer, sizeof (TotalDigestBuffer));
    SpdmReturn = SpdmGetDigest (SpdmContext, NULL, &SlotMask, TotalDigestBuffer);
    if ((LIBSPDM_STATUS_IS_ERROR (SpdmReturn)) || ((SlotMask & 0x01) == 0)) {
      *AuthState                         = TCG_DEVICE_SECURITY_EVENT_DATA_DEVICE_AUTH_STATE_FAIL_INVALID;
      SecurityState->AuthenticationState = EDKII_DEVICE_SECURITY_STATE_ERROR_CERTIFIACTE_FAILURE;
      SlotId                             = 0;
      Status                             = ExtendCertificate (SpdmDeviceContext, *AuthState, 0, NULL, NULL, 0, SlotId, SecurityState);
      return Status;
    }

    for (SlotId = 0; SlotId < SPDM_MAX_SLOT_COUNT; SlotId++) {
      if (((SlotMask >> SlotId) & 0x01) == 0) {
        continue;
      }

      CertChainSize = sizeof (CertChain);
      ZeroMem (CertChain, sizeof (CertChain));
      SpdmReturn = SpdmGetCertificateEx (SpdmContext, NULL, SlotId, &CertChainSize, CertChain, (CONST VOID **)&TrustAnchor, &TrustAnchorSize);
      if (LIBSPDM_STATUS_IS_SUCCESS (SpdmReturn)) {
        *IsValidCertChain = TRUE;
        break;
      } else if (SpdmReturn == LIBSPDM_STATUS_VERIF_FAIL) {
        *IsValidCertChain                  = FALSE;
        *AuthState                         = TCG_DEVICE_SECURITY_EVENT_DATA_DEVICE_AUTH_STATE_FAIL_INVALID;
        SecurityState->AuthenticationState = EDKII_DEVICE_SECURITY_STATE_ERROR_CERTIFIACTE_FAILURE;
        Status                             = ExtendCertificate (SpdmDeviceContext, *AuthState, 0, NULL, NULL, 0, SlotId, SecurityState);
      } else if (SpdmReturn == LIBSPDM_STATUS_VERIF_NO_AUTHORITY) {
        *IsValidCertChain                  = TRUE;
        *AuthState                         = TCG_DEVICE_SECURITY_EVENT_DATA_DEVICE_AUTH_STATE_NO_AUTH;
        SecurityState->AuthenticationState = EDKII_DEVICE_SECURITY_STATE_ERROR_CERTIFIACTE_FAILURE;
        *ValidSlotId                       = SlotId;
      }
    }

    if ((SlotId >= SPDM_MAX_SLOT_COUNT) && (*ValidSlotId == SPDM_MAX_SLOT_COUNT)) {
      SecurityState->AuthenticationState = EDKII_DEVICE_SECURITY_STATE_ERROR_DEVICE_ERROR;
      return EFI_DEVICE_ERROR;
    }

    if (TrustAnchor != NULL) {
      *RootCertMatch = TRUE;
      *ValidSlotId   = SlotId;
    } else {
      *ValidSlotId = 0;
    }

    DEBUG ((DEBUG_INFO, "SpdmGetCertificateEx - SpdmReturn %p, TrustAnchorSize 0x%x, RootCertMatch %d\n", SpdmReturn, TrustAnchorSize, *RootCertMatch));

    return EFI_SUCCESS;
  }
}

/**
  This function does authentication.

  @param[in]  SpdmDeviceContext           The SPDM context for the device.
  @param[out]  AuthState                  The auth state of the devices.
  @param[in]  ValidSlotId                 The number of slot for the certificate chain.
  @param[in]  IsValidCertChain            Indicate the validity of CertChain
  @param[in]  RootCertMatch               Indicate the match or mismatch for Rootcert
  @param[out]  SecurityState              The security state of the requester.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_DEVICE_ERROR      The operation was unsuccessful.

**/
EFI_STATUS
EFIAPI
DoDeviceAuthentication (
  IN  SPDM_DEVICE_CONTEXT          *SpdmDeviceContext,
  OUT UINT8                        *AuthState,
  IN  UINT8                        ValidSlotId,
  IN  BOOLEAN                      IsValidCertChain,
  IN  BOOLEAN                      RootCertMatch,
  OUT EDKII_DEVICE_SECURITY_STATE  *SecurityState
  )
{
  EFI_STATUS           Status;
  SPDM_RETURN          SpdmReturn;
  VOID                 *SpdmContext;
  UINT32               CapabilityFlags;
  UINTN                DataSize;
  SPDM_DATA_PARAMETER  Parameter;
  UINTN                CertChainSize;
  UINT8                CertChain[LIBSPDM_MAX_CERT_CHAIN_SIZE];
  UINT8                RequesterNonce[SPDM_NONCE_SIZE];
  UINT8                ResponderNonce[SPDM_NONCE_SIZE];
  VOID                 *TrustAnchor;
  UINTN                TrustAnchorSize;
  BOOLEAN              IsValidChallengeAuthSig;

  SpdmContext = SpdmDeviceContext->SpdmContext;

  ZeroMem (&Parameter, sizeof (Parameter));
  Parameter.location = SpdmDataLocationConnection;
  DataSize           = sizeof (CapabilityFlags);
  SpdmReturn         = SpdmGetData (SpdmContext, SpdmDataCapabilityFlags, &Parameter, &CapabilityFlags, &DataSize);
  if (LIBSPDM_STATUS_IS_ERROR (SpdmReturn)) {
    SecurityState->AuthenticationState = EDKII_DEVICE_SECURITY_STATE_ERROR_DEVICE_ERROR;
    return EFI_DEVICE_ERROR;
  }

  IsValidChallengeAuthSig = FALSE;

  // get the valid CertChain
  CertChainSize = sizeof (CertChain);
  ZeroMem (CertChain, sizeof (CertChain));
  SpdmReturn = SpdmGetCertificateEx (SpdmContext, NULL, ValidSlotId, &CertChainSize, CertChain, (CONST VOID **)&TrustAnchor, &TrustAnchorSize);
  if ((!LIBSPDM_STATUS_IS_SUCCESS (SpdmReturn)) && (!(SpdmReturn == LIBSPDM_STATUS_VERIF_NO_AUTHORITY))) {
    return EFI_DEVICE_ERROR;
  }

  if ((CapabilityFlags & SPDM_GET_CAPABILITIES_RESPONSE_FLAGS_CHAL_CAP) == 0) {
    *AuthState                         = TCG_DEVICE_SECURITY_EVENT_DATA_DEVICE_AUTH_STATE_NO_BINDING;
    SecurityState->AuthenticationState = EDKII_DEVICE_SECURITY_STATE_ERROR_DEVICE_NO_CAPABILITIES;
    Status                             = ExtendCertificate (SpdmDeviceContext, *AuthState, CertChainSize, CertChain, NULL, 0, ValidSlotId, SecurityState);
    return Status;
  } else {
    ZeroMem (RequesterNonce, sizeof (RequesterNonce));
    ZeroMem (ResponderNonce, sizeof (ResponderNonce));
    SpdmReturn = SpdmChallengeEx (SpdmContext, NULL, ValidSlotId, SPDM_CHALLENGE_REQUEST_NO_MEASUREMENT_SUMMARY_HASH, NULL, NULL, NULL, RequesterNonce, ResponderNonce, NULL, 0);
    if (SpdmReturn == LIBSPDM_STATUS_SUCCESS) {
      IsValidChallengeAuthSig = TRUE;
    } else if ((LIBSPDM_STATUS_IS_ERROR (SpdmReturn))) {
      IsValidChallengeAuthSig            = FALSE;
      *AuthState                         = TCG_DEVICE_SECURITY_EVENT_DATA_DEVICE_AUTH_STATE_FAIL_INVALID;
      SecurityState->AuthenticationState = EDKII_DEVICE_SECURITY_STATE_ERROR_CHALLENGE_FAILURE;
      Status                             = ExtendCertificate (SpdmDeviceContext, *AuthState, 0, NULL, NULL, 0, ValidSlotId, SecurityState);
      return Status;
    } else {
      return EFI_DEVICE_ERROR;
    }

    if (IsValidCertChain && IsValidChallengeAuthSig && !RootCertMatch) {
      *AuthState                         = TCG_DEVICE_SECURITY_EVENT_DATA_DEVICE_AUTH_STATE_NO_AUTH;
      SecurityState->AuthenticationState = EDKII_DEVICE_SECURITY_STATE_ERROR_NO_CERT_PROVISION;
      Status                             = ExtendCertificate (SpdmDeviceContext, *AuthState, CertChainSize, CertChain, NULL, 0, ValidSlotId, SecurityState);
    } else if (IsValidCertChain && IsValidChallengeAuthSig && RootCertMatch) {
      *AuthState                         = TCG_DEVICE_SECURITY_EVENT_DATA_DEVICE_AUTH_STATE_SUCCESS;
      SecurityState->AuthenticationState = EDKII_DEVICE_SECURITY_STATE_SUCCESS;
      Status                             = ExtendCertificate (SpdmDeviceContext, *AuthState, CertChainSize, CertChain, TrustAnchor, TrustAnchorSize, ValidSlotId, SecurityState);
    }

    Status = ExtendAuthentication (SpdmDeviceContext, *AuthState, RequesterNonce, ResponderNonce, SecurityState);
  }

  return Status;
}
