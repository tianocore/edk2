/** @file

  MCTP Manageability Protocol common file.

  Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ManageabilityTransportHelperLib.h>
#include <Library/ManageabilityTransportMctpLib.h>
#include <Library/ManageabilityTransportLib.h>

#include <IndustryStandard/Mctp.h>

#include "MctpProtocolCommon.h"

extern CHAR16  *mTransportName;
extern UINT32  mTransportMaximumPayload;

MANAGEABILITY_TRANSPORT_HARDWARE_INFORMATION  mHardwareInformation;
UINT8                                         mMctpPacketSequence;
BOOLEAN                                       mStartOfMessage;
BOOLEAN                                       mEndOfMessage;

/**
  This functions setup the MCTP transport hardware information according
  to the specification of transport token acquired from transport library.

  @param[in]         TransportToken       The transport interface.
  @param[out]        HardwareInformation  Pointer to receive the hardware information.

  @retval EFI_SUCCESS            Hardware information is returned in HardwareInformation.
                                 Caller must free the memory allocated for HardwareInformation
                                 once it doesn't need it.
  @retval EFI_UNSUPPORTED        No hardware information for the specification specified
                                 in the transport token.
**/
EFI_STATUS
SetupMctpTransportHardwareInformation (
  IN   MANAGEABILITY_TRANSPORT_TOKEN                 *TransportToken,
  OUT  MANAGEABILITY_TRANSPORT_HARDWARE_INFORMATION  *HardwareInformation
  )
{
  MANAGEABILITY_TRANSPORT_KCS_HARDWARE_INFO  *KcsHardwareInfo;
  BOOLEAN                                    MctpKcsMemMapIo;

  KcsHardwareInfo = NULL;
  if (CompareGuid (&gManageabilityTransportKcsGuid, TransportToken->Transport->ManageabilityTransportSpecification)) {
    KcsHardwareInfo = AllocatePool (sizeof (MANAGEABILITY_TRANSPORT_KCS_HARDWARE_INFO));
    if (KcsHardwareInfo == NULL) {
      DEBUG ((DEBUG_ERROR, "%a: Not enough memory for MANAGEABILITY_TRANSPORT_KCS_HARDWARE_INFO.\n", __func__));
      return EFI_OUT_OF_RESOURCES;
    }

    MctpKcsMemMapIo = PcdGetBool (PcdMctpKcsMemoryMappedIo);
    if (MctpKcsMemMapIo) {
      KcsHardwareInfo->MemoryMap                    = MANAGEABILITY_TRANSPORT_KCS_MEMORY_MAP_IO;
      KcsHardwareInfo->IoBaseAddress.IoAddress32    = MCTP_KCS_BASE_ADDRESS;
      KcsHardwareInfo->IoDataInAddress.IoAddress32  = MCTP_KCS_REG_DATA_IN_MEMMAP;
      KcsHardwareInfo->IoDataOutAddress.IoAddress32 = MCTP_KCS_REG_DATA_OUT_MEMMAP;
      KcsHardwareInfo->IoCommandAddress.IoAddress32 = MCTP_KCS_REG_COMMAND_MEMMAP;
      KcsHardwareInfo->IoStatusAddress.IoAddress32  = MCTP_KCS_REG_STATUS_MEMMAP;
    } else {
      KcsHardwareInfo->MemoryMap                    = MANAGEABILITY_TRANSPORT_KCS_IO_MAP_IO;
      KcsHardwareInfo->IoBaseAddress.IoAddress16    = (UINT16)MCTP_KCS_BASE_ADDRESS;
      KcsHardwareInfo->IoDataInAddress.IoAddress16  = (UINT16)MCTP_KCS_REG_DATA_IN_IO;
      KcsHardwareInfo->IoDataOutAddress.IoAddress16 = (UINT16)MCTP_KCS_REG_DATA_OUT_IO;
      KcsHardwareInfo->IoCommandAddress.IoAddress16 = (UINT16)MCTP_KCS_REG_COMMAND_IO;
      KcsHardwareInfo->IoStatusAddress.IoAddress16  = (UINT16)MCTP_KCS_REG_STATUS_IO;
    }

    HardwareInformation->Kcs = KcsHardwareInfo;
    return EFI_SUCCESS;
  } else {
    DEBUG ((DEBUG_ERROR, "%a: No implementation of setting hardware information.", __func__));
    ASSERT (FALSE);
  }

  return EFI_UNSUPPORTED;
}

/**
  This functions setup the final header/body/trailer packets for
  the acquired transport interface.

  @param[in]         TransportToken             The transport interface.
  @param[in]         MctpType                   MCTP message type.
  @param[in]         MctpSourceEndpointId       MCTP source endpoint ID.
  @param[in]         MctpDestinationEndpointId  MCTP source endpoint ID.
  @param[in]         RequestDataIntegrityCheck  Indicates whether MCTP message has
                                                integrity check byte.
  @param[out]        PacketHeader               The pointer to receive header of request.
  @param[out]        PacketHeaderSize           Packet header size.
  @param[in, out]    PacketBody                 The request body.
                                                When IN, it is the caller's request body.
                                                When OUT and NULL, the request body is not
                                                changed.
                                                Whee out and non-NULL, the request body is
                                                changed to comfort the transport interface.
  @param[in, out]    PacketBodySize             The request body size.
                                                When IN and non-zero, it is the new data
                                                length of request body.
                                                When IN and zero, the request body is unchanged.
  @param[out]        PacketTrailer              The pointer to receive trailer of request.
  @param[out]        PacketTrailerSize          Packet trailer size.

  @retval EFI_SUCCESS            Request packet is returned.
  @retval EFI_OUT_OF_RESOURCE    Not enough resource to create the request
                                 transport packets.
  @retval EFI_UNSUPPORTED        Request packet is not returned because
                                 the unsupported transport interface.
**/
EFI_STATUS
SetupMctpRequestTransportPacket (
  IN   MANAGEABILITY_TRANSPORT_TOKEN    *TransportToken,
  IN   UINT8                            MctpType,
  IN   UINT8                            MctpSourceEndpointId,
  IN   UINT8                            MctpDestinationEndpointId,
  IN   BOOLEAN                          RequestDataIntegrityCheck,
  OUT  MANAGEABILITY_TRANSPORT_HEADER   *PacketHeader,
  OUT  UINT16                           *PacketHeaderSize,
  IN   OUT UINT8                        **PacketBody,
  IN   OUT UINT32                       *PacketBodySize,
  OUT  MANAGEABILITY_TRANSPORT_TRAILER  *PacketTrailer,
  OUT  UINT16                           *PacketTrailerSize
  )
{
  MANAGEABILITY_MCTP_KCS_HEADER   *MctpKcsHeader;
  MCTP_TRANSPORT_HEADER           *MctpTransportHeader;
  MCTP_MESSAGE_HEADER             *MctpMessageHeader;
  MANAGEABILITY_MCTP_KCS_TRAILER  *MctpKcsTrailer;
  UINT8                           *ThisPackage;

  if ((PacketHeader == NULL) || (PacketHeaderSize == NULL) ||
      (PacketBody == NULL) || (PacketBodySize == NULL) ||
      (PacketTrailer == NULL) || (PacketTrailerSize == NULL)
      )
  {
    DEBUG ((DEBUG_ERROR, "%a: One or more than one of the input parameter is invalid.\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if (CompareGuid (&gManageabilityTransportKcsGuid, TransportToken->Transport->ManageabilityTransportSpecification)) {
    MctpKcsHeader = (MANAGEABILITY_MCTP_KCS_HEADER *)AllocateZeroPool (sizeof (MANAGEABILITY_MCTP_KCS_HEADER));
    if (MctpKcsHeader == NULL) {
      DEBUG ((DEBUG_ERROR, "%a: Not enough resource for MANAGEABILITY_MCTP_KCS_HEADER.\n", __func__));
      return EFI_OUT_OF_RESOURCES;
    }

    MctpKcsTrailer = (MANAGEABILITY_MCTP_KCS_TRAILER *)AllocateZeroPool (sizeof (MANAGEABILITY_MCTP_KCS_TRAILER));
    if (MctpKcsTrailer == NULL) {
      DEBUG ((DEBUG_ERROR, "%a: Not enough resource for PEC.\n", __func__));
      FreePool (MctpKcsHeader);
      return EFI_OUT_OF_RESOURCES;
    }

    // Generate MCTP KCS transport header
    MctpKcsHeader->DefiningBody = DEFINING_BODY_DMTF_PRE_OS_WORKING_GROUP;
    MctpKcsHeader->NetFunc      = MCTP_KCS_NETFN_LUN;
    MctpKcsHeader->ByteCount    = (UINT8)(MIN (mTransportMaximumPayload, *PacketBodySize + (UINT8)sizeof (MCTP_MESSAGE_HEADER) + (UINT8)sizeof (MCTP_TRANSPORT_HEADER)));

    ThisPackage = (UINT8 *)AllocateZeroPool (MctpKcsHeader->ByteCount);
    if (ThisPackage == NULL) {
      DEBUG ((DEBUG_ERROR, "%a: Not enough resource for package.\n", __func__));
      FreePool (MctpKcsHeader);
      FreePool (MctpKcsTrailer);
      return EFI_OUT_OF_RESOURCES;
    }

    // Setup MCTP transport header
    MctpTransportHeader                             = (MCTP_TRANSPORT_HEADER *)ThisPackage;
    MctpTransportHeader->Bits.Reserved              = 0;
    MctpTransportHeader->Bits.HeaderVersion         = MCTP_KCS_HEADER_VERSION;
    MctpTransportHeader->Bits.DestinationEndpointId = MctpDestinationEndpointId;
    MctpTransportHeader->Bits.SourceEndpointId      = MctpSourceEndpointId;
    MctpTransportHeader->Bits.MessageTag            = MCTP_MESSAGE_TAG;
    MctpTransportHeader->Bits.TagOwner              = MCTP_MESSAGE_TAG_OWNER_REQUEST;
    MctpTransportHeader->Bits.PacketSequence        = mMctpPacketSequence & MCTP_PACKET_SEQUENCE_MASK;
    MctpTransportHeader->Bits.StartOfMessage        = mStartOfMessage ? 1 : 0;
    MctpTransportHeader->Bits.EndOfMessage          = mEndOfMessage ? 1 : 0;

    // Setup MCTP message header
    MctpMessageHeader                      = (MCTP_MESSAGE_HEADER *)(MctpTransportHeader + 1);
    MctpMessageHeader->Bits.MessageType    = MctpType;
    MctpMessageHeader->Bits.IntegrityCheck = RequestDataIntegrityCheck ? 1 : 0;

    // Copy payload
    CopyMem ((VOID *)(MctpMessageHeader + 1), (VOID *)*PacketBody, *PacketBodySize);

    //
    // Generate PEC follow SMBUS 2.0 specification.
    MctpKcsTrailer->Pec = HelperManageabilityGenerateCrc8 (MCTP_KCS_PACKET_ERROR_CODE_POLY, 0, ThisPackage, MctpKcsHeader->ByteCount);

    *PacketBody        = (UINT8 *)ThisPackage;
    *PacketBodySize    = MctpKcsHeader->ByteCount;
    *PacketTrailer     = (MANAGEABILITY_TRANSPORT_TRAILER)MctpKcsTrailer;
    *PacketHeader      = (MANAGEABILITY_TRANSPORT_HEADER)MctpKcsHeader;
    *PacketHeaderSize  = sizeof (MANAGEABILITY_MCTP_KCS_HEADER);
    *PacketTrailerSize = sizeof (MANAGEABILITY_MCTP_KCS_TRAILER);
    return EFI_SUCCESS;
  } else {
    DEBUG ((DEBUG_ERROR, "%a: No implementation of building up packet.", __func__));
    ASSERT (FALSE);
  }

  return EFI_SUCCESS;
}

/**
  Common code to submit MCTP message

  @param[in]         TransportToken             Transport token.
  @param[in]         MctpType                   MCTP message type.
  @param[in]         MctpSourceEndpointId       MCTP source endpoint ID.
  @param[in]         MctpDestinationEndpointId  MCTP source endpoint ID.
  @param[in]         RequestDataIntegrityCheck  Indicates whether MCTP message has
                                                integrity check byte.
  @param[in]         RequestData                Message Data.
  @param[in]         RequestDataSize            Size of message Data.
  @param[in]         RequestTimeout             Timeout value in milliseconds.
                                                MANAGEABILITY_TRANSPORT_NO_TIMEOUT means no timeout value.
  @param[out]        ResponseData               Message Response Data. The completion code is the first byte of response data.
  @param[in, out]    ResponseDataSize           Size of Message Response Data.
  @param[in]         ResponseTimeout            Timeout value in milliseconds.
                                                MANAGEABILITY_TRANSPORT_NO_TIMEOUT means no timeout value.
  @param[out]        AdditionalTransferError    MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS.

  @retval EFI_SUCCESS            The message was successfully send to transport interface and a
                                 response was successfully received.
  @retval EFI_NOT_FOUND          The message was not successfully sent to transport interface or a response
                                 was not successfully received from transport interface.
  @retval EFI_NOT_READY          MCTP transport interface is not ready for MCTP message.
  @retval EFI_DEVICE_ERROR       MCTP transport interface Device hardware error.
  @retval EFI_TIMEOUT            The message time out.
  @retval EFI_UNSUPPORTED        The message was not successfully sent to the transport interface.
  @retval EFI_OUT_OF_RESOURCES   The resource allocation is out of resource or data size error.
  @retval EFI_INVALID_PARAMETER  Both RequestData and ResponseData are NULL
**/
EFI_STATUS
CommonMctpSubmitMessage (
  IN     MANAGEABILITY_TRANSPORT_TOKEN              *TransportToken,
  IN     UINT8                                      MctpType,
  IN     UINT8                                      MctpSourceEndpointId,
  IN     UINT8                                      MctpDestinationEndpointId,
  IN     BOOLEAN                                    RequestDataIntegrityCheck,
  IN     UINT8                                      *RequestData,
  IN     UINT32                                     RequestDataSize,
  IN     UINT32                                     RequestTimeout,
  OUT    UINT8                                      *ResponseData,
  IN OUT UINT32                                     *ResponseDataSize,
  IN     UINT32                                     ResponseTimeout,
  OUT    MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS  *AdditionalTransferError
  )
{
  EFI_STATUS                                 Status;
  UINT16                                     IndexOfPackage;
  UINT8                                      *ThisRequestData;
  UINT32                                     ThisRequestDataSize;
  UINT16                                     MctpTransportHeaderSize;
  UINT16                                     MctpTransportTrailerSize;
  MANAGEABILITY_TRANSFER_TOKEN               TransferToken;
  MANAGEABILITY_TRANSPORT_HEADER             MctpTransportHeader;
  MANAGEABILITY_TRANSPORT_TRAILER            MctpTransportTrailer;
  MANAGEABILITY_TRANSMISSION_MULTI_PACKAGES  *MultiPackages;
  MANAGEABILITY_TRANSMISSION_PACKAGE_ATTR    *ThisPackage;
  UINT8                                      *ResponseBuffer;
  MCTP_TRANSPORT_HEADER                      *MctpTransportResponseHeader;
  MCTP_MESSAGE_HEADER                        *MctpMessageResponseHeader;

  if (TransportToken == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: No transport toke for MCTP\n", __func__));
    return EFI_UNSUPPORTED;
  }

  Status = TransportToken->Transport->Function.Version1_0->TransportStatus (
                                                             TransportToken,
                                                             AdditionalTransferError
                                                             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Transport %s for MCTP has problem - (%r)\n", __func__, mTransportName, Status));
    return Status;
  }

  MultiPackages = NULL;
  Status        = HelperManageabilitySplitPayload (
                    sizeof (MCTP_TRANSPORT_HEADER) + sizeof (MCTP_MESSAGE_HEADER),
                    0,
                    RequestData,
                    RequestDataSize,
                    mTransportMaximumPayload,
                    &MultiPackages
                    );
  if (EFI_ERROR (Status) || (MultiPackages == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Fails to split payload into multiple packages - (%r)\n", __func__, mTransportName, Status));
    return Status;
  }

  // Print transmission packages info.
  DEBUG ((DEBUG_MANAGEABILITY_INFO, "Manageability Transmission packages:\n"));
  ThisPackage = (MANAGEABILITY_TRANSMISSION_PACKAGE_ATTR *)(MultiPackages + 1);
  for (IndexOfPackage = 0; IndexOfPackage < MultiPackages->NumberOfPackages; IndexOfPackage++) {
    DEBUG ((DEBUG_MANAGEABILITY_INFO, "#%d: \n", IndexOfPackage));
    DEBUG ((DEBUG_MANAGEABILITY_INFO, "    Packet pointer: 0x%08x\n", ThisPackage->PayloadPointer));
    DEBUG ((DEBUG_MANAGEABILITY_INFO, "    Packet size   : 0x%08x\n", ThisPackage->PayloadSize));
  }

  ThisPackage         = (MANAGEABILITY_TRANSMISSION_PACKAGE_ATTR *)(MultiPackages + 1);
  mMctpPacketSequence = 0;
  for (IndexOfPackage = 0; IndexOfPackage < MultiPackages->NumberOfPackages; IndexOfPackage++) {
    MctpTransportHeader  = NULL;
    MctpTransportTrailer = NULL;
    ThisRequestData      = ThisPackage->PayloadPointer;
    ThisRequestDataSize  = ThisPackage->PayloadSize;

    // Setup Start of Message bit and End of Message bit.
    if (MultiPackages->NumberOfPackages == 1) {
      mStartOfMessage = TRUE;
      mEndOfMessage   = TRUE;
    } else if (IndexOfPackage == 0) {
      mStartOfMessage = TRUE;
      mEndOfMessage   = FALSE;
    } else if (IndexOfPackage == MultiPackages->NumberOfPackages - 1) {
      mStartOfMessage = FALSE;
      mEndOfMessage   = TRUE;
    } else {
      mStartOfMessage = FALSE;
      mEndOfMessage   = FALSE;
    }

    Status = SetupMctpRequestTransportPacket (
               TransportToken,
               MctpType,
               MctpSourceEndpointId,
               MctpDestinationEndpointId,
               RequestDataIntegrityCheck,
               &MctpTransportHeader,
               &MctpTransportHeaderSize,
               &ThisRequestData,
               &ThisRequestDataSize,
               &MctpTransportTrailer,
               &MctpTransportTrailerSize
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Fail to build packets - (%r)\n", __func__, Status));
      return Status;
    }

    ZeroMem (&TransferToken, sizeof (MANAGEABILITY_TRANSFER_TOKEN));
    TransferToken.TransmitHeader      = MctpTransportHeader;
    TransferToken.TransmitHeaderSize  = MctpTransportHeaderSize;
    TransferToken.TransmitTrailer     = MctpTransportTrailer;
    TransferToken.TransmitTrailerSize = MctpTransportTrailerSize;

    // Transmit packet.
    if ((ThisRequestData == NULL) || (ThisRequestDataSize == 0)) {
      // Transmit parameter were not changed by SetupMctpRequestTransportPacket().
      TransferToken.TransmitPackage.TransmitPayload    = ThisPackage->PayloadPointer;
      TransferToken.TransmitPackage.TransmitSizeInByte = ThisPackage->PayloadSize;
    } else {
      TransferToken.TransmitPackage.TransmitPayload    = ThisRequestData;
      TransferToken.TransmitPackage.TransmitSizeInByte = ThisRequestDataSize;
    }

    TransferToken.TransmitPackage.TransmitTimeoutInMillisecond = MANAGEABILITY_TRANSPORT_NO_TIMEOUT;

    // Receive packet.
    TransferToken.ReceivePackage.ReceiveBuffer                = NULL;
    TransferToken.ReceivePackage.ReceiveSizeInByte            = 0;
    TransferToken.ReceivePackage.TransmitTimeoutInMillisecond = MANAGEABILITY_TRANSPORT_NO_TIMEOUT;

    // Print out MCTP packet.
    DEBUG ((
      DEBUG_MANAGEABILITY_INFO,
      "%a: Send MCTP message type: 0x%x, from source endpoint ID: 0x%x to destination ID 0x%x: Request size: 0x%x, Response size: 0x%x\n",
      __func__,
      MctpType,
      MctpSourceEndpointId,
      MctpDestinationEndpointId,
      TransferToken.TransmitPackage.TransmitSizeInByte,
      TransferToken.ReceivePackage.ReceiveSizeInByte
      ));

    if ((MctpTransportHeader != NULL) && (MctpTransportHeaderSize != 0)) {
      HelperManageabilityDebugPrint (
        (VOID *)TransferToken.TransmitHeader,
        (UINT32)TransferToken.TransmitHeaderSize,
        "MCTP transport header.\n"
        );
    }

    HelperManageabilityDebugPrint (
      (VOID *)TransferToken.TransmitPackage.TransmitPayload,
      TransferToken.TransmitPackage.TransmitSizeInByte,
      "MCTP full request payload.\n"
      );

    if ((MctpTransportTrailer != NULL) && (MctpTransportTrailerSize != 0)) {
      HelperManageabilityDebugPrint (
        (VOID *)TransferToken.TransmitTrailer,
        (UINT32)TransferToken.TransmitTrailerSize,
        "MCTP transport trailer.\n"
        );
    }

    TransportToken->Transport->Function.Version1_0->TransportTransmitReceive (
                                                      TransportToken,
                                                      &TransferToken
                                                      );
    if (MctpTransportHeader != NULL) {
      FreePool ((VOID *)MctpTransportHeader);
    }

    if (MctpTransportTrailer != NULL) {
      FreePool ((VOID *)MctpTransportTrailer);
    }

    if (ThisRequestData != NULL) {
      FreePool ((VOID *)ThisRequestData);
      ThisRequestData = NULL;
    }

    //
    // Return transfer status.
    //
    Status                   = TransferToken.TransferStatus;
    *AdditionalTransferError = TransferToken.TransportAdditionalStatus;
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to send MCTP command over %s\n", __func__, mTransportName));
      FreePool (MultiPackages);
      return Status;
    }

    mMctpPacketSequence++;
    ThisPackage++;
  }

  ResponseBuffer = (UINT8 *)AllocatePool (*ResponseDataSize + sizeof (MCTP_TRANSPORT_HEADER) + sizeof (MCTP_MESSAGE_HEADER));
  // Receive packet.
  TransferToken.TransmitPackage.TransmitPayload             = NULL;
  TransferToken.TransmitPackage.TransmitSizeInByte          = 0;
  TransferToken.ReceivePackage.ReceiveBuffer                = ResponseBuffer;
  TransferToken.ReceivePackage.ReceiveSizeInByte            = *ResponseDataSize + sizeof (MCTP_TRANSPORT_HEADER) + sizeof (MCTP_MESSAGE_HEADER);
  TransferToken.TransmitHeader                              = NULL;
  TransferToken.TransmitHeaderSize                          = 0;
  TransferToken.TransmitTrailer                             = NULL;
  TransferToken.TransmitTrailerSize                         = 0;
  TransferToken.ReceivePackage.TransmitTimeoutInMillisecond = MANAGEABILITY_TRANSPORT_NO_TIMEOUT;

  DEBUG ((
    DEBUG_MANAGEABILITY_INFO,
    "%a: Retrieve MCTP message Response size: 0x%x\n",
    __func__,
    TransferToken.ReceivePackage.ReceiveSizeInByte
    ));
  TransportToken->Transport->Function.Version1_0->TransportTransmitReceive (
                                                    TransportToken,
                                                    &TransferToken
                                                    );

  *AdditionalTransferError = TransferToken.TransportAdditionalStatus;
  Status                   = TransferToken.TransferStatus;
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to send MCTP command over %s: %r\n", __func__, mTransportName, Status));
    return Status;
  }

  MctpTransportResponseHeader = (MCTP_TRANSPORT_HEADER *)ResponseBuffer;
  if (MctpTransportResponseHeader->Bits.HeaderVersion != MCTP_KCS_HEADER_VERSION) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Error! Response HeaderVersion (0x%02x) doesn't match MCTP_KCS_HEADER_VERSION (0x%02x)\n",
      __func__,
      MctpTransportResponseHeader->Bits.HeaderVersion,
      MCTP_KCS_HEADER_VERSION
      ));
    FreePool (ResponseBuffer);
    return EFI_DEVICE_ERROR;
  }

  if (MctpTransportResponseHeader->Bits.MessageTag != MCTP_MESSAGE_TAG) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Error! Response MessageTag (0x%02x) doesn't match MCTP_MESSAGE_TAG (0x%02x)\n",
      __func__,
      MctpTransportResponseHeader->Bits.MessageTag,
      MCTP_MESSAGE_TAG
      ));
    FreePool (ResponseBuffer);
    return EFI_DEVICE_ERROR;
  }

  if (MctpTransportResponseHeader->Bits.TagOwner != MCTP_MESSAGE_TAG_OWNER_RESPONSE) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Error! Response TagOwner (0x%02x) doesn't match MCTP_MESSAGE_TAG_OWNER_RESPONSE (0x%02x)\n",
      __func__,
      MctpTransportResponseHeader->Bits.TagOwner,
      MCTP_MESSAGE_TAG_OWNER_RESPONSE
      ));
    FreePool (ResponseBuffer);
    return EFI_DEVICE_ERROR;
  }

  if (MctpTransportResponseHeader->Bits.SourceEndpointId != MctpDestinationEndpointId) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Error! Response SrcEID (0x%02x) doesn't match sent EID (0x%02x)\n",
      __func__,
      MctpTransportResponseHeader->Bits.SourceEndpointId,
      MctpDestinationEndpointId
      ));
    FreePool (ResponseBuffer);
    return EFI_DEVICE_ERROR;
  }

  if (MctpTransportResponseHeader->Bits.DestinationEndpointId != MctpSourceEndpointId) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Error! Response DestEID (0x%02x) doesn't match local EID (0x%02x)\n",
      __func__,
      MctpTransportResponseHeader->Bits.DestinationEndpointId,
      MctpSourceEndpointId
      ));
    FreePool (ResponseBuffer);
    return EFI_DEVICE_ERROR;
  }

  if ((MctpTransportResponseHeader->Bits.StartOfMessage != 1) ||
      (MctpTransportResponseHeader->Bits.EndOfMessage != 1) ||
      (MctpTransportResponseHeader->Bits.PacketSequence != 0))
  {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Error! Multiple-packet MCTP responses are not supported by the current driver\n",
      __func__
      ));
    FreePool (ResponseBuffer);
    return EFI_UNSUPPORTED;
  }

  MctpMessageResponseHeader = (MCTP_MESSAGE_HEADER *)(MctpTransportResponseHeader + 1);
  if (MctpMessageResponseHeader->Bits.MessageType != MctpType) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Error! Response MessageType (0x%02x) doesn't match sent MessageType (0x%02x)\n",
      __func__,
      MctpMessageResponseHeader->Bits.MessageType,
      MctpType
      ));
    FreePool (ResponseBuffer);
    return EFI_DEVICE_ERROR;
  }

  if (MctpMessageResponseHeader->Bits.IntegrityCheck != (UINT8)RequestDataIntegrityCheck) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Error! Response IntegrityCheck (%d) doesn't match sent IntegrityCheck (%d)\n",
      __func__,
      MctpMessageResponseHeader->Bits.IntegrityCheck,
      (UINT8)RequestDataIntegrityCheck
      ));
    FreePool (ResponseBuffer);
    return EFI_DEVICE_ERROR;
  }

  *ResponseDataSize = TransferToken.ReceivePackage.ReceiveSizeInByte - sizeof (MCTP_TRANSPORT_HEADER) - sizeof (MCTP_MESSAGE_HEADER);
  CopyMem (ResponseData, ResponseBuffer + sizeof (MCTP_TRANSPORT_HEADER) + sizeof (MCTP_MESSAGE_HEADER), *ResponseDataSize);
  FreePool (ResponseBuffer);

  return Status;
}
