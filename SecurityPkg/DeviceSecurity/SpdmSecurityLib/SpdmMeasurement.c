/** @file
  EDKII Device Security library for SPDM device.
  It follows the SPDM Specification.

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SpdmSecurityLibInternal.h"

/**
  This function returns the SPDM device type for TCG SPDM event.

  @param[in]  SpdmDeviceContext           The SPDM context for the device.

  @return TCG SPDM device type
**/
UINT32
EFIAPI
GetSpdmDeviceType (
  IN  SPDM_DEVICE_CONTEXT  *SpdmDeviceContext
  )
{
  if (CompareGuid (&SpdmDeviceContext->DeviceId.DeviceType, &gEdkiiDeviceIdentifierTypePciGuid)) {
    return TCG_DEVICE_SECURITY_EVENT_DATA_DEVICE_TYPE_PCI;
  }

  if (CompareGuid (&SpdmDeviceContext->DeviceId.DeviceType, &gEdkiiDeviceIdentifierTypeUsbGuid)) {
    return TCG_DEVICE_SECURITY_EVENT_DATA_DEVICE_TYPE_USB;
  }

  return TCG_DEVICE_SECURITY_EVENT_DATA_DEVICE_TYPE_NULL;
}

/**
  This function returns the SPDM device measurement context size for TCG SPDM event.

  @param[in]  SpdmDeviceContext          The SPDM context for the device.

  @return TCG SPDM device measurement context size
**/
UINTN
EFIAPI
GetDeviceMeasurementContextSize (
  IN  SPDM_DEVICE_CONTEXT  *SpdmDeviceContext
  )
{
  if (CompareGuid (&SpdmDeviceContext->DeviceId.DeviceType, &gEdkiiDeviceIdentifierTypePciGuid)) {
    return sizeof (TCG_DEVICE_SECURITY_EVENT_DATA_PCI_CONTEXT);
  }

  if (CompareGuid (&SpdmDeviceContext->DeviceId.DeviceType, &gEdkiiDeviceIdentifierTypeUsbGuid)) {
    // TBD - usb context
    return 0;
  }

  return 0;
}

/**
  This function creates the SPDM PCI device measurement context for TCG SPDM event.

  @param[in]       SpdmDeviceContext       The SPDM context for the device.
  @param[in, out]  DeviceContext           The TCG SPDM PCI device measurement context.
  @param[in]       DeviceContextSize       The size of TCG SPDM PCI device measurement context.

  @retval EFI_SUCCESS      The TCG SPDM PCI device measurement context is returned.
**/
EFI_STATUS
CreatePciDeviceMeasurementContext (
  IN  SPDM_DEVICE_CONTEXT  *SpdmDeviceContext,
  IN OUT VOID              *DeviceContext,
  IN UINTN                 DeviceContextSize
  )
{
  TCG_DEVICE_SECURITY_EVENT_DATA_PCI_CONTEXT  *PciContext;
  PCI_TYPE00                                  PciData;
  EFI_PCI_IO_PROTOCOL                         *PciIo;
  EFI_STATUS                                  Status;

  if (DeviceContextSize != sizeof (*PciContext)) {
    return EFI_BUFFER_TOO_SMALL;
  }

  PciIo  = SpdmDeviceContext->DeviceIo;
  Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint8, 0, sizeof (PciData), &PciData);
  ASSERT_EFI_ERROR (Status);

  PciContext               = DeviceContext;
  PciContext->Version      = TCG_DEVICE_SECURITY_EVENT_DATA_PCI_CONTEXT_VERSION;
  PciContext->Length       = sizeof (*PciContext);
  PciContext->VendorId     = PciData.Hdr.VendorId;
  PciContext->DeviceId     = PciData.Hdr.DeviceId;
  PciContext->RevisionID   = PciData.Hdr.RevisionID;
  PciContext->ClassCode[0] = PciData.Hdr.ClassCode[0];
  PciContext->ClassCode[1] = PciData.Hdr.ClassCode[1];
  PciContext->ClassCode[2] = PciData.Hdr.ClassCode[2];
  if ((PciData.Hdr.HeaderType & HEADER_LAYOUT_CODE) == HEADER_TYPE_DEVICE) {
    PciContext->SubsystemVendorID = PciData.Device.SubsystemVendorID;
    PciContext->SubsystemID       = PciData.Device.SubsystemID;
  } else {
    PciContext->SubsystemVendorID = 0;
    PciContext->SubsystemID       = 0;
  }

  return EFI_SUCCESS;
}

/**
  This function creates the SPDM device measurement context for TCG SPDM event.

  @param[in]       SpdmDeviceContext       The SPDM context for the device.
  @param[in, out]  DeviceContext           The TCG SPDM device measurement context.
  @param[in]       DeviceContextSize       The size of TCG SPDM device measurement context.

  @retval EFI_SUCCESS      The TCG SPDM device measurement context is returned.
  @retval EFI_UNSUPPORTED  The TCG SPDM device measurement context is unsupported.
**/
EFI_STATUS
EFIAPI
CreateDeviceMeasurementContext (
  IN  SPDM_DEVICE_CONTEXT  *SpdmDeviceContext,
  IN OUT VOID              *DeviceContext,
  IN UINTN                 DeviceContextSize
  )
{
  if (CompareGuid (&SpdmDeviceContext->DeviceId.DeviceType, &gEdkiiDeviceIdentifierTypePciGuid)) {
    return CreatePciDeviceMeasurementContext (SpdmDeviceContext, DeviceContext, DeviceContextSize);
  }

  if (CompareGuid (&SpdmDeviceContext->DeviceId.DeviceType, &gEdkiiDeviceIdentifierTypeUsbGuid)) {
    return EFI_UNSUPPORTED;
  }

  return EFI_UNSUPPORTED;
}

/**
  This function dumps data.

  @param[in]  Data             A pointer to Data.
  @param[in]  Size             The size of Data.

**/
VOID
EFIAPI
InternalDumpData (
  CONST UINT8  *Data,
  UINTN        Size
  )
{
  UINTN  Index;

  for (Index = 0; Index < Size; Index++) {
    DEBUG ((DEBUG_INFO, "%02x ", (UINTN)Data[Index]));
  }
}

/**
  This function extend the PCI digest from the DvSec register.

  @param[in]  SpdmDeviceContext       The SPDM context for the device.
  @param[in]  AuthState               The auth state of the device.
  @param[in]  MeasurementRecordLength The length of the SPDM measurement record
  @param[in]  MeasurementRecord       The SPDM measurement record
  @param[in]  RequesterNonce           A buffer to hold the requester nonce (32 bytes), if not NULL.
  @param[in]  ResponderNonce           A buffer to hold the responder nonce (32 bytes), if not NULL.
  @param[out] SecurityState            The Device Security state associated with the device.

  @retval EFI_SUCCESS                 Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES          Out of memory.
  @retval EFI_DEVICE_ERROR              The operation was unsuccessful.

**/
EFI_STATUS
ExtendMeasurement (
  IN  SPDM_DEVICE_CONTEXT          *SpdmDeviceContext,
  IN UINT8                         AuthState,
  IN UINT32                        MeasurementRecordLength,
  IN UINT8                         *MeasurementRecord,
  IN UINT8                         *RequesterNonce,
  IN UINT8                         *ResponderNonce,
  OUT EDKII_DEVICE_SECURITY_STATE  *SecurityState
  )
{
  UINT32  PcrIndex;
  UINT32  EventType;
  VOID    *EventLog;
  UINT32  EventLogSize;
  UINT8   *EventLogPtr;

  TCG_DEVICE_SECURITY_EVENT_DATA_HEADER2                            *EventData2;
  TCG_DEVICE_SECURITY_EVENT_DATA_SUB_HEADER_SPDM_MEASUREMENT_BLOCK  *TcgSpdmMeasurementBlock;
  VOID                                                              *DeviceContext;
  UINTN                                                             DeviceContextSize;
  EFI_STATUS                                                        Status;
  SPDM_MEASUREMENT_BLOCK_COMMON_HEADER                              *SpdmMeasurementBlockCommonHeader;
  SPDM_MEASUREMENT_BLOCK_DMTF_HEADER                                *SpdmMeasurementBlockDmtfHeader;
  VOID                                                              *Digest;
  UINTN                                                             DigestSize;
  UINTN                                                             DevicePathSize;
  UINT32                                                            MeasurementHashAlgo;
  UINTN                                                             DataSize;
  VOID                                                              *SpdmContext;
  SPDM_DATA_PARAMETER                                               Parameter;

  SpdmContext = SpdmDeviceContext->SpdmContext;

  EventLog = NULL;
  ZeroMem (&Parameter, sizeof (Parameter));
  Parameter.location = SpdmDataLocationConnection;
  DataSize           = sizeof (MeasurementHashAlgo);
  Status             = SpdmGetData (SpdmContext, SpdmDataMeasurementHashAlgo, &Parameter, &MeasurementHashAlgo, &DataSize);
  ASSERT_EFI_ERROR (Status);

  if (MeasurementRecord != NULL) {
    SpdmMeasurementBlockCommonHeader = (VOID *)MeasurementRecord;
    SpdmMeasurementBlockDmtfHeader   = (VOID *)(SpdmMeasurementBlockCommonHeader + 1);
    Digest                           = (SpdmMeasurementBlockDmtfHeader + 1);
    DigestSize                       = MeasurementRecordLength - sizeof (SPDM_MEASUREMENT_BLOCK_DMTF);

    DEBUG ((DEBUG_INFO, "SpdmMeasurementBlockCommonHeader\n"));
    DEBUG ((DEBUG_INFO, "  Index                        - 0x%02x\n", SpdmMeasurementBlockCommonHeader->Index));
    DEBUG ((DEBUG_INFO, "  MeasurementSpecification     - 0x%02x\n", SpdmMeasurementBlockCommonHeader->MeasurementSpecification));
    DEBUG ((DEBUG_INFO, "  MeasurementSize              - 0x%04x\n", SpdmMeasurementBlockCommonHeader->MeasurementSize));
    DEBUG ((DEBUG_INFO, "SpdmMeasurementBlockDmtfHeader\n"));
    DEBUG ((DEBUG_INFO, "  DMTFSpecMeasurementValueType - 0x%02x\n", SpdmMeasurementBlockDmtfHeader->DMTFSpecMeasurementValueType));
    DEBUG ((DEBUG_INFO, "  DMTFSpecMeasurementValueSize - 0x%04x\n", SpdmMeasurementBlockDmtfHeader->DMTFSpecMeasurementValueSize));
    DEBUG ((DEBUG_INFO, "Measurement - "));
    InternalDumpData (Digest, DigestSize);
    DEBUG ((DEBUG_INFO, "\n"));
    if (MeasurementRecordLength <= sizeof (SPDM_MEASUREMENT_BLOCK_COMMON_HEADER) + sizeof (SPDM_MEASUREMENT_BLOCK_DMTF_HEADER)) {
      SecurityState->MeasurementState = EDKII_DEVICE_SECURITY_STATE_ERROR_MEASUREMENT_AUTH_FAILURE;
      return EFI_SECURITY_VIOLATION;
    }

    if ((SpdmMeasurementBlockCommonHeader->MeasurementSpecification & SPDM_MEASUREMENT_SPECIFICATION_DMTF) == 0) {
      SecurityState->MeasurementState = EDKII_DEVICE_SECURITY_STATE_ERROR_MEASUREMENT_AUTH_FAILURE;
      return EFI_SECURITY_VIOLATION;
    }

    if (SpdmMeasurementBlockCommonHeader->MeasurementSize != MeasurementRecordLength - sizeof (SPDM_MEASUREMENT_BLOCK_COMMON_HEADER)) {
      SecurityState->MeasurementState = EDKII_DEVICE_SECURITY_STATE_ERROR_MEASUREMENT_AUTH_FAILURE;
      return EFI_SECURITY_VIOLATION;
    }

    if (SpdmMeasurementBlockDmtfHeader->DMTFSpecMeasurementValueSize != SpdmMeasurementBlockCommonHeader->MeasurementSize - sizeof (SPDM_MEASUREMENT_BLOCK_DMTF_HEADER)) {
      SecurityState->MeasurementState = EDKII_DEVICE_SECURITY_STATE_ERROR_MEASUREMENT_AUTH_FAILURE;
      return EFI_SECURITY_VIOLATION;
    }

    //
    // Use PCR 2 for Firmware Blob code.
    //
    switch (SpdmMeasurementBlockDmtfHeader->DMTFSpecMeasurementValueType & 0x7F) {
      case SPDM_MEASUREMENT_BLOCK_MEASUREMENT_TYPE_IMMUTABLE_ROM:
      case SPDM_MEASUREMENT_BLOCK_MEASUREMENT_TYPE_MUTABLE_FIRMWARE:
      case SPDM_MEASUREMENT_BLOCK_MEASUREMENT_TYPE_VERSION:
      case SPDM_MEASUREMENT_BLOCK_MEASUREMENT_TYPE_SECURE_VERSION_NUMBER:
        if (SpdmDeviceContext->IsEmbeddedDevice) {
          PcrIndex = 0;
        } else {
          PcrIndex = 2;
        }

        EventType = EV_EFI_SPDM_FIRMWARE_BLOB;
        break;
      case SPDM_MEASUREMENT_BLOCK_MEASUREMENT_TYPE_HARDWARE_CONFIGURATION:
      case SPDM_MEASUREMENT_BLOCK_MEASUREMENT_TYPE_FIRMWARE_CONFIGURATION:
      case SPDM_MEASUREMENT_BLOCK_MEASUREMENT_TYPE_DEVICE_MODE:
        if (SpdmDeviceContext->IsEmbeddedDevice) {
          PcrIndex = 1;
        } else {
          PcrIndex = 3;
        }

        EventType = EV_EFI_SPDM_FIRMWARE_CONFIG;
        break;
      case SPDM_MEASUREMENT_BLOCK_MEASUREMENT_TYPE_MEASUREMENT_MANIFEST:
      // skip manifest, because manifest doesn't belong to the EV_EFI_SPDM_FIRMWARE_BLOB and EV_EFI_SPDM_FIRMWARE_CONFIG
      default:
        return EFI_SUCCESS;
    }
  } else {
    if (SpdmDeviceContext->IsEmbeddedDevice) {
      PcrIndex = 0;
    } else {
      PcrIndex = 2;
    }

    EventType = EV_EFI_SPDM_FIRMWARE_BLOB;
  }

  DeviceContextSize = GetDeviceMeasurementContextSize (SpdmDeviceContext);
  DevicePathSize    = GetDevicePathSize (SpdmDeviceContext->DevicePath);

  switch (AuthState) {
    case TCG_DEVICE_SECURITY_EVENT_DATA_DEVICE_AUTH_STATE_SUCCESS:
      EventLogSize = (UINT32)(sizeof (TCG_DEVICE_SECURITY_EVENT_DATA_HEADER2) +
                              sizeof (UINT64) + DevicePathSize +
                              sizeof (TCG_DEVICE_SECURITY_EVENT_DATA_SUB_HEADER_SPDM_MEASUREMENT_BLOCK) +
                              MeasurementRecordLength +
                              DeviceContextSize);
      EventLog = AllocatePool (EventLogSize);
      if (EventLog == NULL) {
        SecurityState->MeasurementState = EDKII_DEVICE_SECURITY_STATE_ERROR_UEFI_OUT_OF_RESOURCE;
        return EFI_OUT_OF_RESOURCES;
      }

      EventLogPtr = EventLog;

      EventData2 = (VOID *)EventLogPtr;
      CopyMem (EventData2->Signature, TCG_DEVICE_SECURITY_EVENT_DATA_SIGNATURE_2, sizeof (EventData2->Signature));
      EventData2->Version    = TCG_DEVICE_SECURITY_EVENT_DATA_VERSION_2;
      EventData2->AuthState  = AuthState;
      EventData2->Reserved   = 0;
      EventData2->Length     = (UINT32)EventLogSize;
      EventData2->DeviceType = GetSpdmDeviceType (SpdmDeviceContext);

      EventData2->SubHeaderType   = TCG_DEVICE_SECURITY_EVENT_DATA_DEVICE_SUB_HEADER_TYPE_SPDM_MEASUREMENT_BLOCK;
      EventData2->SubHeaderLength = sizeof (TCG_DEVICE_SECURITY_EVENT_DATA_SUB_HEADER_SPDM_MEASUREMENT_BLOCK) + MeasurementRecordLength;
      EventData2->SubHeaderUID    = SpdmDeviceContext->DeviceUID;

      EventLogPtr = (VOID *)(EventData2 + 1);

      *(UINT64 *)EventLogPtr = (UINT64)DevicePathSize;
      EventLogPtr           += sizeof (UINT64);
      CopyMem (EventLogPtr, SpdmDeviceContext->DevicePath, DevicePathSize);
      EventLogPtr += DevicePathSize;

      TcgSpdmMeasurementBlock                            = (VOID *)EventLogPtr;
      TcgSpdmMeasurementBlock->SpdmVersion               = SpdmDeviceContext->SpdmVersion;
      TcgSpdmMeasurementBlock->SpdmMeasurementBlockCount = 1;
      TcgSpdmMeasurementBlock->Reserved                  = 0;
      TcgSpdmMeasurementBlock->SpdmMeasurementHashAlgo   = MeasurementHashAlgo;
      EventLogPtr                                       += sizeof (TCG_DEVICE_SECURITY_EVENT_DATA_SUB_HEADER_SPDM_MEASUREMENT_BLOCK);

      if ((MeasurementRecord != NULL) && (MeasurementRecordLength != 0)) {
        CopyMem (EventLogPtr, MeasurementRecord, MeasurementRecordLength);
        EventLogPtr += MeasurementRecordLength;
      }

      if (DeviceContextSize != 0) {
        DeviceContext = (VOID *)EventLogPtr;
        Status        = CreateDeviceMeasurementContext (SpdmDeviceContext, DeviceContext, DeviceContextSize);
        if (Status != EFI_SUCCESS) {
          SecurityState->MeasurementState = EDKII_DEVICE_SECURITY_STATE_ERROR_DEVICE_ERROR;
          Status                          = EFI_DEVICE_ERROR;
          goto Exit;
        }
      }

      Status = TpmMeasureAndLogData (
                 PcrIndex,
                 EventType,
                 EventLog,
                 EventLogSize,
                 EventLog,
                 EventLogSize
                 );
      if (EFI_ERROR (Status)) {
        SecurityState->MeasurementState = EDKII_DEVICE_SECURITY_STATE_ERROR_TCG_EXTEND_TPM_PCR;
      }

      DEBUG ((DEBUG_INFO, "TpmMeasureAndLogData (Measurement) - %r\n", Status));
      break;
    case TCG_DEVICE_SECURITY_EVENT_DATA_DEVICE_AUTH_STATE_FAIL_INVALID:
      EventLogSize = (UINT32)(sizeof (TCG_DEVICE_SECURITY_EVENT_DATA_HEADER2) +
                              sizeof (UINT64) + DevicePathSize +
                              DeviceContextSize);
      EventLog = AllocatePool (EventLogSize);
      if (EventLog == NULL) {
        SecurityState->MeasurementState = EDKII_DEVICE_SECURITY_STATE_ERROR_UEFI_OUT_OF_RESOURCE;
        return EFI_OUT_OF_RESOURCES;
      }

      EventLogPtr = EventLog;

      EventData2 = (VOID *)EventLogPtr;
      CopyMem (EventData2->Signature, TCG_DEVICE_SECURITY_EVENT_DATA_SIGNATURE_2, sizeof (EventData2->Signature));
      EventData2->Version    = TCG_DEVICE_SECURITY_EVENT_DATA_VERSION_2;
      EventData2->AuthState  = AuthState;
      EventData2->Reserved   = 0;
      EventData2->Length     = (UINT32)EventLogSize;
      EventData2->DeviceType = GetSpdmDeviceType (SpdmDeviceContext);

      EventData2->SubHeaderType   = TCG_DEVICE_SECURITY_EVENT_DATA_DEVICE_SUB_HEADER_TYPE_SPDM_MEASUREMENT_BLOCK;
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
          SecurityState->MeasurementState = EDKII_DEVICE_SECURITY_STATE_ERROR_DEVICE_ERROR;
          Status                          = EFI_DEVICE_ERROR;
          goto Exit;
        }
      }

      Status = TpmMeasureAndLogData (
                 PcrIndex,
                 EventType,
                 EventLog,
                 EventLogSize,
                 EventLog,
                 EventLogSize
                 );
      if (EFI_ERROR (Status)) {
        SecurityState->MeasurementState = EDKII_DEVICE_SECURITY_STATE_ERROR_TCG_EXTEND_TPM_PCR;
      }

      DEBUG ((DEBUG_INFO, "TpmMeasureAndLogData (Measurement) - %r\n", Status));
      goto Exit;
    default:
      SecurityState->MeasurementState = EDKII_DEVICE_SECURITY_STATE_ERROR_UEFI_UNSUPPORTED;
      return EFI_UNSUPPORTED;
  }

  if (RequesterNonce != NULL) {
    TCG_NV_INDEX_DYNAMIC_EVENT_LOG_STRUCT_SPDM_GET_MEASUREMENTS  DynamicEventLogSpdmGetMeasurementsEvent;

    CopyMem (DynamicEventLogSpdmGetMeasurementsEvent.Header.Signature, TCG_NV_EXTEND_INDEX_FOR_DYNAMIC_SIGNATURE, sizeof (TCG_NV_EXTEND_INDEX_FOR_DYNAMIC_SIGNATURE));
    DynamicEventLogSpdmGetMeasurementsEvent.Header.Version = TCG_NV_INDEX_DYNAMIC_EVENT_LOG_STRUCT_VERSION;
    ZeroMem (DynamicEventLogSpdmGetMeasurementsEvent.Header.Reserved, sizeof (DynamicEventLogSpdmGetMeasurementsEvent.Header.Reserved));
    DynamicEventLogSpdmGetMeasurementsEvent.Header.Uid      = SpdmDeviceContext->DeviceUID;
    DynamicEventLogSpdmGetMeasurementsEvent.DescriptionSize = sizeof (TCG_SPDM_GET_MEASUREMENTS_DESCRIPTION);
    CopyMem (DynamicEventLogSpdmGetMeasurementsEvent.Description, TCG_SPDM_GET_MEASUREMENTS_DESCRIPTION, sizeof (TCG_SPDM_GET_MEASUREMENTS_DESCRIPTION));
    DynamicEventLogSpdmGetMeasurementsEvent.DataSize = SPDM_NONCE_SIZE;
    CopyMem (DynamicEventLogSpdmGetMeasurementsEvent.Data, RequesterNonce, SPDM_NONCE_SIZE);

    Status = TpmMeasureAndLogData (
               TCG_NV_EXTEND_INDEX_FOR_DYNAMIC,
               EV_NO_ACTION,
               &DynamicEventLogSpdmGetMeasurementsEvent,
               sizeof (DynamicEventLogSpdmGetMeasurementsEvent),
               &DynamicEventLogSpdmGetMeasurementsEvent,
               sizeof (DynamicEventLogSpdmGetMeasurementsEvent)
               );
    if (EFI_ERROR (Status)) {
      SecurityState->MeasurementState = EDKII_DEVICE_SECURITY_STATE_ERROR_TCG_EXTEND_TPM_PCR;
    }

    DEBUG ((DEBUG_INFO, "TpmMeasureAndLogData (Dynamic) - %r\n", Status));
  }

  if (ResponderNonce != NULL) {
    TCG_NV_INDEX_DYNAMIC_EVENT_LOG_STRUCT_SPDM_MEASUREMENTS  DynamicEventLogSpdmMeasurementsEvent;

    CopyMem (DynamicEventLogSpdmMeasurementsEvent.Header.Signature, TCG_NV_EXTEND_INDEX_FOR_DYNAMIC_SIGNATURE, sizeof (TCG_NV_EXTEND_INDEX_FOR_DYNAMIC_SIGNATURE));
    DynamicEventLogSpdmMeasurementsEvent.Header.Version = TCG_NV_INDEX_DYNAMIC_EVENT_LOG_STRUCT_VERSION;
    ZeroMem (DynamicEventLogSpdmMeasurementsEvent.Header.Reserved, sizeof (DynamicEventLogSpdmMeasurementsEvent.Header.Reserved));
    DynamicEventLogSpdmMeasurementsEvent.Header.Uid      = SpdmDeviceContext->DeviceUID;
    DynamicEventLogSpdmMeasurementsEvent.DescriptionSize = sizeof (TCG_SPDM_MEASUREMENTS_DESCRIPTION);
    CopyMem (DynamicEventLogSpdmMeasurementsEvent.Description, TCG_SPDM_MEASUREMENTS_DESCRIPTION, sizeof (TCG_SPDM_MEASUREMENTS_DESCRIPTION));
    DynamicEventLogSpdmMeasurementsEvent.DataSize = SPDM_NONCE_SIZE;
    CopyMem (DynamicEventLogSpdmMeasurementsEvent.Data, ResponderNonce, SPDM_NONCE_SIZE);

    Status = TpmMeasureAndLogData (
               TCG_NV_EXTEND_INDEX_FOR_DYNAMIC,
               EV_NO_ACTION,
               &DynamicEventLogSpdmMeasurementsEvent,
               sizeof (DynamicEventLogSpdmMeasurementsEvent),
               &DynamicEventLogSpdmMeasurementsEvent,
               sizeof (DynamicEventLogSpdmMeasurementsEvent)
               );
    if (EFI_ERROR (Status)) {
      SecurityState->MeasurementState = EDKII_DEVICE_SECURITY_STATE_ERROR_TCG_EXTEND_TPM_PCR;
    }

    DEBUG ((DEBUG_INFO, "TpmMeasureAndLogData (Dynamic) - %r\n", Status));
  }

Exit:
  if (EventLog != NULL) {
    FreePool (EventLog);
  }

  return Status;
}

/**
  This function gets SPDM measurement and extend to TPM.

  @param[in]  SpdmDeviceContext            The SPDM context for the device.
  @param[in]  SlotId                       The number of slot id of the certificate.
  @param[out] SecurityState                A poniter to security state of the requester.

  @retval EFI_SUCCESS            Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES   Out of memory.
  @retval EFI_DEVICE_ERROR       The operation was unsuccessful.

**/
EFI_STATUS
EFIAPI
DoDeviceMeasurement (
  IN  SPDM_DEVICE_CONTEXT          *SpdmDeviceContext,
  IN  UINT8                        SlotId,
  OUT EDKII_DEVICE_SECURITY_STATE  *SecurityState
  )
{
  EFI_STATUS                   Status;
  SPDM_RETURN                  SpdmReturn;
  VOID                         *SpdmContext;
  UINT32                       CapabilityFlags;
  UINTN                        DataSize;
  SPDM_DATA_PARAMETER          Parameter;
  UINT8                        NumberOfBlocks;
  UINT32                       MeasurementRecordLength;
  UINT8                        MeasurementRecord[LIBSPDM_MAX_MEASUREMENT_RECORD_SIZE];
  UINT8                        Index;
  UINT8                        RequesterNonce[SPDM_NONCE_SIZE];
  UINT8                        ResponderNonce[SPDM_NONCE_SIZE];
  UINT8                        RequestAttribute;
  UINT32                       MeasurementsBlockSize;
  SPDM_MEASUREMENT_BLOCK_DMTF  *MeasurementBlock;
  UINT8                        NumberOfBlock;
  UINT8                        ReceivedNumberOfBlock;
  UINT8                        AuthState;
  UINT8                        ContentChanged;
  UINT8                        ContentChangedCount;

  SpdmContext = SpdmDeviceContext->SpdmContext;

  ZeroMem (&Parameter, sizeof (Parameter));
  Parameter.location = SpdmDataLocationConnection;
  DataSize           = sizeof (CapabilityFlags);
  SpdmGetData (SpdmContext, SpdmDataCapabilityFlags, &Parameter, &CapabilityFlags, &DataSize);

  if ((CapabilityFlags & SPDM_GET_CAPABILITIES_RESPONSE_FLAGS_MEAS_CAP_SIG) == 0) {
    AuthState                       = TCG_DEVICE_SECURITY_EVENT_DATA_DEVICE_AUTH_STATE_FAIL_NO_SIG;
    Status                          = ExtendCertificate (SpdmDeviceContext, AuthState, 0, NULL, NULL, 0, 0, SecurityState);
    SecurityState->MeasurementState = EDKII_DEVICE_SECURITY_STATE_ERROR_DEVICE_NO_CAPABILITIES;
    if (Status != EFI_SUCCESS) {
      return Status;
    } else {
      return EFI_UNSUPPORTED;
    }
  }

  RequestAttribute  = 0;
  RequestAttribute |= SPDM_GET_MEASUREMENTS_REQUEST_ATTRIBUTES_GENERATE_SIGNATURE;

  MeasurementRecordLength = sizeof (MeasurementRecord);
  ZeroMem (RequesterNonce, sizeof (RequesterNonce));
  ZeroMem (ResponderNonce, sizeof (ResponderNonce));

  //
  // get all measurement once, with signature.
  //
  SpdmReturn = SpdmGetMeasurementEx (
                 SpdmContext,
                 NULL,
                 RequestAttribute,
                 SPDM_GET_MEASUREMENTS_REQUEST_MEASUREMENT_OPERATION_ALL_MEASUREMENTS,
                 SlotId,
                 NULL,
                 &NumberOfBlocks,
                 &MeasurementRecordLength,
                 MeasurementRecord,
                 NULL,
                 RequesterNonce,
                 ResponderNonce,
                 NULL,
                 0
                 );
  if (LIBSPDM_STATUS_IS_SUCCESS (SpdmReturn)) {
    DEBUG ((DEBUG_INFO, "NumberOfBlocks %d\n", NumberOfBlocks));

    MeasurementBlock = (VOID *)MeasurementRecord;
    for (Index = 0; Index < NumberOfBlocks; Index++) {
      MeasurementsBlockSize =
        sizeof (SPDM_MEASUREMENT_BLOCK_DMTF) +
        MeasurementBlock
          ->MeasurementBlockDmtfHeader
          .DMTFSpecMeasurementValueSize;

      AuthState                       = TCG_DEVICE_SECURITY_EVENT_DATA_DEVICE_AUTH_STATE_SUCCESS;
      SecurityState->MeasurementState = EDKII_DEVICE_SECURITY_STATE_SUCCESS;
      if (Index == NumberOfBlocks - 1) {
        Status = ExtendMeasurement (SpdmDeviceContext, AuthState, MeasurementsBlockSize, (UINT8 *)MeasurementBlock, RequesterNonce, ResponderNonce, SecurityState);
      } else {
        Status = ExtendMeasurement (SpdmDeviceContext, AuthState, MeasurementsBlockSize, (UINT8 *)MeasurementBlock, NULL, NULL, SecurityState);
      }

      MeasurementBlock = (VOID *)((size_t)MeasurementBlock + MeasurementsBlockSize);
      if (Status != EFI_SUCCESS) {
        return Status;
      }
    }
  } else if (SpdmReturn == LIBSPDM_STATUS_VERIF_FAIL) {
    AuthState                       = TCG_DEVICE_SECURITY_EVENT_DATA_DEVICE_AUTH_STATE_FAIL_INVALID;
    SecurityState->MeasurementState = EDKII_DEVICE_SECURITY_STATE_ERROR_MEASUREMENT_AUTH_FAILURE;
    Status                          = ExtendMeasurement (SpdmDeviceContext, AuthState, 0, NULL, NULL, NULL, SecurityState);
    return Status;
  } else {
    ContentChangedCount = 0;
ContentChangedFlag:
    RequestAttribute      = 0;
    ContentChanged        = SPDM_MEASUREMENTS_RESPONSE_CONTENT_NO_CHANGE_DETECTED;
    ReceivedNumberOfBlock = 0;

    //
    // 1. Query the total number of measurements available.
    //
    SpdmReturn = SpdmGetMeasurement (
                   SpdmContext,
                   NULL,
                   RequestAttribute,
                   SPDM_GET_MEASUREMENTS_REQUEST_MEASUREMENT_OPERATION_TOTAL_NUMBER_OF_MEASUREMENTS,
                   SlotId,
                   NULL,
                   &NumberOfBlocks,
                   NULL,
                   NULL
                   );
    if (LIBSPDM_STATUS_IS_ERROR (SpdmReturn)) {
      SecurityState->MeasurementState = EDKII_DEVICE_SECURITY_STATE_ERROR_DEVICE_ERROR;
      return EFI_DEVICE_ERROR;
    }

    DEBUG ((DEBUG_INFO, "NumberOfBlocks - 0x%x\n", NumberOfBlocks));

    ReceivedNumberOfBlock = 0;
    for (Index = 1; Index <= 0xFE; Index++) {
      if (ReceivedNumberOfBlock == NumberOfBlocks) {
        break;
      }

      DEBUG ((DEBUG_INFO, "Index - 0x%x\n", Index));
      //
      // 2. query measurement one by one
      //    get signature in last message only.
      //
      if (ReceivedNumberOfBlock == NumberOfBlocks - 1) {
        RequestAttribute |= SPDM_GET_MEASUREMENTS_REQUEST_ATTRIBUTES_GENERATE_SIGNATURE;
      }

      MeasurementRecordLength = sizeof (MeasurementRecord);
      ZeroMem (RequesterNonce, sizeof (RequesterNonce));
      ZeroMem (ResponderNonce, sizeof (ResponderNonce));
      SpdmReturn = SpdmGetMeasurementEx (
                     SpdmContext,
                     NULL,
                     RequestAttribute,
                     Index,
                     SlotId,
                     &ContentChanged,
                     &NumberOfBlock,
                     &MeasurementRecordLength,
                     MeasurementRecord,
                     NULL,
                     RequesterNonce,
                     ResponderNonce,
                     NULL,
                     0
                     );
      if (LIBSPDM_STATUS_IS_ERROR (SpdmReturn)) {
        if (SpdmReturn == LIBSPDM_STATUS_VERIF_FAIL) {
          AuthState                       = TCG_DEVICE_SECURITY_EVENT_DATA_DEVICE_AUTH_STATE_FAIL_INVALID;
          SecurityState->MeasurementState = EDKII_DEVICE_SECURITY_STATE_ERROR_DEVICE_ERROR;
          Status                          = ExtendMeasurement (SpdmDeviceContext, AuthState, 0, NULL, NULL, NULL, SecurityState);
          return Status;
        } else {
          continue;
        }
      }

      if ((ReceivedNumberOfBlock == NumberOfBlocks - 1) &&
          (ContentChanged == SPDM_MEASUREMENTS_RESPONSE_CONTENT_CHANGE_DETECTED))
      {
        if (ContentChangedCount == 0) {
          ContentChangedCount++;
          goto ContentChangedFlag;
        } else {
          AuthState                       = TCG_DEVICE_SECURITY_EVENT_DATA_DEVICE_AUTH_STATE_FAIL_INVALID;
          SecurityState->MeasurementState = EDKII_DEVICE_SECURITY_STATE_ERROR_DEVICE_ERROR;
          Status                          = ExtendMeasurement (SpdmDeviceContext, AuthState, 0, NULL, NULL, NULL, SecurityState);
          return Status;
        }
      }

      DEBUG ((DEBUG_INFO, "ExtendMeasurement...\n"));
      AuthState                       = TCG_DEVICE_SECURITY_EVENT_DATA_DEVICE_AUTH_STATE_SUCCESS;
      SecurityState->MeasurementState = EDKII_DEVICE_SECURITY_STATE_SUCCESS;
      if (ReceivedNumberOfBlock == NumberOfBlocks - 1) {
        Status = ExtendMeasurement (SpdmDeviceContext, AuthState, MeasurementRecordLength, MeasurementRecord, RequesterNonce, ResponderNonce, SecurityState);
      } else {
        Status = ExtendMeasurement (SpdmDeviceContext, AuthState, MeasurementRecordLength, MeasurementRecord, NULL, ResponderNonce, SecurityState);
      }

      if (Status != EFI_SUCCESS) {
        return Status;
      }

      ReceivedNumberOfBlock += 1;
    }

    if (ReceivedNumberOfBlock != NumberOfBlocks) {
      SecurityState->MeasurementState = EDKII_DEVICE_SECURITY_STATE_ERROR_MEASUREMENT_AUTH_FAILURE;
      return EFI_DEVICE_ERROR;
    }
  }

  return EFI_SUCCESS;
}
