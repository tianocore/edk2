/** @file

  IPMI Manageability Protocol common file.

  Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ManageabilityTransportIpmiLib.h>
#include <Library/ManageabilityTransportLib.h>

#include "IpmiProtocolCommon.h"

/**
  This functions setup the IPMI transport hardware information according
  to the specification of transport token acquired from transport library.

  @param[in]         TransportToken       The transport interface.
  @param[out]        HardwareInformation  Pointer to receive the hardware information.

  @retval EFI_SUCCESS            Hardware information is returned in HardwareInformation.
                                 Caller must free the memory allocated for HardwareInformation
                                 once it doesn't need it.
  @retval EFI_UNSUPPORTED        No hardware information for the specification specified
                                 in the transport token.
  @retval EFI_OUT_OF_RESOURCES   Not enough memory for MANAGEABILITY_TRANSPORT_KCS_HARDWARE_INFO
                                 or MANAGEABILITY_TRANSPORT_SSIF_HARDWARE_INFO.
**/
EFI_STATUS
SetupIpmiTransportHardwareInformation (
  IN   MANAGEABILITY_TRANSPORT_TOKEN                 *TransportToken,
  OUT  MANAGEABILITY_TRANSPORT_HARDWARE_INFORMATION  *HardwareInformation
  )
{
  MANAGEABILITY_TRANSPORT_KCS_HARDWARE_INFO     *KcsHardwareInfo;
  MANAGEABILITY_TRANSPORT_SSIF_HARDWARE_INFO    *SsifHardwareInfo;
  MANAGEABILITY_TRANSPORT_SERIAL_HARDWARE_INFO  *SerialHardwareInfo;

  if (CompareGuid (&gManageabilityTransportKcsGuid, TransportToken->Transport->ManageabilityTransportSpecification)) {
    // This is KCS transport interface.
    KcsHardwareInfo = AllocatePool (sizeof (MANAGEABILITY_TRANSPORT_KCS_HARDWARE_INFO));
    if (KcsHardwareInfo == NULL) {
      DEBUG ((DEBUG_ERROR, "%a: Not enough memory for MANAGEABILITY_TRANSPORT_KCS_HARDWARE_INFO.\n", __func__));
      return EFI_OUT_OF_RESOURCES;
    }

    KcsHardwareInfo->MemoryMap                    = MANAGEABILITY_TRANSPORT_KCS_IO_MAP_IO;
    KcsHardwareInfo->IoBaseAddress.IoAddress16    = IPMI_KCS_BASE_ADDRESS;
    KcsHardwareInfo->IoDataInAddress.IoAddress16  = IPMI_KCS_REG_DATA_IN;
    KcsHardwareInfo->IoDataOutAddress.IoAddress16 = IPMI_KCS_REG_DATA_OUT;
    KcsHardwareInfo->IoCommandAddress.IoAddress16 = IPMI_KCS_REG_COMMAND;
    KcsHardwareInfo->IoStatusAddress.IoAddress16  = IPMI_KCS_REG_STATUS;
    HardwareInformation->Kcs                      = KcsHardwareInfo;
    return EFI_SUCCESS;
  } else if (CompareGuid (&gManageabilityTransportSmbusI2cGuid, TransportToken->Transport->ManageabilityTransportSpecification)) {
    // This is SSIF transport interface.
    SsifHardwareInfo = AllocatePool (sizeof (MANAGEABILITY_TRANSPORT_SSIF_HARDWARE_INFO));
    if (SsifHardwareInfo == NULL) {
      DEBUG ((DEBUG_ERROR, "%a: Not enough memory for MANAGEABILITY_TRANSPORT_SSIF_HARDWARE_INFO.\n", __func__));
      return EFI_OUT_OF_RESOURCES;
    }

    SsifHardwareInfo->BmcSlaveAddress = IPMI_SSIF_BMC_SLAVE_ADDRESS;
    HardwareInformation->Ssif         = SsifHardwareInfo;
    return EFI_SUCCESS;
  } else if (CompareGuid (&gManageabilityTransportSerialGuid, TransportToken->Transport->ManageabilityTransportSpecification)) {
    // This is Serial transport interface.
    SerialHardwareInfo = AllocatePool (sizeof (MANAGEABILITY_TRANSPORT_SERIAL_HARDWARE_INFO));
    if (SerialHardwareInfo == NULL) {
      DEBUG ((DEBUG_ERROR, "%a: Not enough memory for MANAGEABILITY_TRANSPORT_SERIAL_HARDWARE_INFO.\n", __func__));
      return EFI_OUT_OF_RESOURCES;
    }

    SerialHardwareInfo->IpmiRequesterAddress = IPMI_SERIAL_REQUESTER_ADDRESS;
    SerialHardwareInfo->IpmiResponderAddress = IPMI_SERIAL_RESPONDER_ADDRESS;
    SerialHardwareInfo->IpmiRequesterLUN     = IPMI_SERIAL_REQUESTER_LUN;
    SerialHardwareInfo->IpmiResponderLUN     = IPMI_SERIAL_RESPONDER_LUN;
    HardwareInformation->Serial              = SerialHardwareInfo;
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

  @param[in]         TransportToken     The transport interface.
  @param[in]         NetFunction        IPMI function.
  @param[in]         Command            IPMI command.
  @param[out]        PacketHeader       The pointer to receive header of request.
  @param[out]        PacketHeaderSize   Pinter to receive packet header size in byte.
  @param[in, out]    PacketBody         The request body.
                                        When IN, it is the caller's request body.
                                        When OUT and NULL, the request body is not
                                        changed.
                                        Whee out and non-NULL, the request body is
                                        changed to comfort the transport interface.
  @param[in, out]    PacketBodySize     The request body size.
                                        When IN and non-zero, it is the new data
                                        length of request body.
                                        When IN and zero, the request body is unchanged.
  @param[out]        PacketTrailer      The pointer to receive trailer of request.
  @param[out]        PacketTrailerSize  Pinter to receive packet trailer size in byte.

  @retval EFI_SUCCESS            Request packet is returned.
  @retval EFI_UNSUPPORTED        Request packet is not returned because
                                 the unsupported transport interface.
**/
EFI_STATUS
SetupIpmiRequestTransportPacket (
  IN   MANAGEABILITY_TRANSPORT_TOKEN    *TransportToken,
  IN   UINT8                            NetFunction,
  IN   UINT8                            Command,
  OUT  MANAGEABILITY_TRANSPORT_HEADER   *PacketHeader OPTIONAL,
  OUT  UINT16                           *PacketHeaderSize,
  IN OUT UINT8                          **PacketBody OPTIONAL,
  IN OUT UINT32                         *PacketBodySize OPTIONAL,
  OUT  MANAGEABILITY_TRANSPORT_TRAILER  *PacketTrailer OPTIONAL,
  OUT  UINT16                           *PacketTrailerSize
  )
{
  MANAGEABILITY_IPMI_TRANSPORT_HEADER  *IpmiHeader;

  if (  CompareGuid (&gManageabilityTransportKcsGuid, TransportToken->Transport->ManageabilityTransportSpecification)
     || CompareGuid (&gManageabilityTransportSmbusI2cGuid, TransportToken->Transport->ManageabilityTransportSpecification)
     || CompareGuid (&gManageabilityTransportSerialGuid, TransportToken->Transport->ManageabilityTransportSpecification))
  {
    IpmiHeader = AllocateZeroPool (sizeof (MANAGEABILITY_IPMI_TRANSPORT_HEADER));
    if (IpmiHeader == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    *PacketHeaderSize   = 0;
    *PacketTrailerSize  = 0;
    IpmiHeader->Command = Command;
    IpmiHeader->Lun     = MANAGEABILITY_IPMI_BMC_LUN;
    IpmiHeader->NetFn   = NetFunction;
    if (PacketHeader != NULL) {
      *PacketHeader     = (MANAGEABILITY_TRANSPORT_HEADER *)IpmiHeader;
      *PacketHeaderSize = sizeof (MANAGEABILITY_IPMI_TRANSPORT_HEADER);
    }

    if (PacketTrailer != NULL) {
      *PacketTrailer = NULL;
    }

    if (PacketBody != NULL) {
      *PacketBody = NULL;
    }

    if (PacketBodySize != NULL) {
      *PacketBodySize = 0;
    }
  } else {
    DEBUG ((DEBUG_ERROR, "%a: No implementation of building up packet.", __func__));
    ASSERT (FALSE);
  }

  return EFI_SUCCESS;
}

/**
  Common code to submit IPMI commands

  @param[in]         TransportToken    TRansport token.
  @param[in]         NetFunction       Net function of the command.
  @param[in]         Command           IPMI Command.
  @param[in]         RequestData       Command Request Data.
  @param[in]         RequestDataSize   Size of Command Request Data.
  @param[out]        ResponseData      Command Response Data. The completion code is the first byte of response data.
  @param[in, out]    ResponseDataSize  Size of Command Response Data.

  @retval EFI_SUCCESS            The command byte stream was successfully submit to the device and a response was successfully received.
  @retval EFI_NOT_FOUND          The command was not successfully sent to the device or a response was not successfully received from the device.
  @retval EFI_NOT_READY          Ipmi Device is not ready for Ipmi command access.
  @retval EFI_DEVICE_ERROR       Ipmi Device hardware error.
  @retval EFI_TIMEOUT            The command time out.
  @retval EFI_UNSUPPORTED        The command was not successfully sent to the device.
  @retval EFI_OUT_OF_RESOURCES   The resource allocation is out of resource or data size error.
**/
EFI_STATUS
CommonIpmiSubmitCommand (
  IN     MANAGEABILITY_TRANSPORT_TOKEN  *TransportToken,
  IN     UINT8                          NetFunction,
  IN     UINT8                          Command,
  IN     UINT8                          *RequestData OPTIONAL,
  IN     UINT32                         RequestDataSize,
  OUT    UINT8                          *ResponseData OPTIONAL,
  IN OUT UINT32                         *ResponseDataSize OPTIONAL
  )
{
  EFI_STATUS                                 Status;
  UINT8                                      *ThisRequestData;
  UINT32                                     ThisRequestDataSize;
  MANAGEABILITY_TRANSFER_TOKEN               TransferToken;
  MANAGEABILITY_TRANSPORT_HEADER             IpmiTransportHeader;
  MANAGEABILITY_TRANSPORT_TRAILER            IpmiTransportTrailer;
  MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS  TransportAdditionalStatus;
  UINT16                                     HeaderSize;
  UINT16                                     TrailerSize;

  if (TransportToken == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: No transport toke for IPMI\n", __func__));
    return EFI_UNSUPPORTED;
  }

  Status = TransportToken->Transport->Function.Version1_0->TransportStatus (
                                                             TransportToken,
                                                             &TransportAdditionalStatus
                                                             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Transport for IPMI has problem - (%r)\n", __func__, Status));
    return Status;
  }

  ThisRequestData      = RequestData;
  ThisRequestDataSize  = RequestDataSize;
  IpmiTransportHeader  = NULL;
  IpmiTransportTrailer = NULL;
  Status               = SetupIpmiRequestTransportPacket (
                           TransportToken,
                           NetFunction,
                           Command,
                           &IpmiTransportHeader,
                           &HeaderSize,
                           &ThisRequestData,
                           &ThisRequestDataSize,
                           &IpmiTransportTrailer,
                           &TrailerSize
                           );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Fail to build packets - (%r)\n", __func__, Status));
    return Status;
  }

  ZeroMem (&TransferToken, sizeof (MANAGEABILITY_TRANSFER_TOKEN));
  TransferToken.TransmitHeader      = IpmiTransportHeader;
  TransferToken.TransmitHeaderSize  = HeaderSize;
  TransferToken.TransmitTrailer     = IpmiTransportTrailer;
  TransferToken.TransmitTrailerSize = TrailerSize;

  // Transmit packet.
  if ((ThisRequestData == NULL) || (ThisRequestDataSize == 0)) {
    // Transmit parameter were not changed by SetupIpmiRequestTransportPacket().
    TransferToken.TransmitPackage.TransmitPayload    = RequestData;
    TransferToken.TransmitPackage.TransmitSizeInByte = RequestDataSize;
  } else {
    TransferToken.TransmitPackage.TransmitPayload    = ThisRequestData;
    TransferToken.TransmitPackage.TransmitSizeInByte = ThisRequestDataSize;
  }

  TransferToken.TransmitPackage.TransmitTimeoutInMillisecond = MANAGEABILITY_TRANSPORT_NO_TIMEOUT;

  // Receive packet.
  TransferToken.ReceivePackage.ReceiveBuffer                = ResponseData;
  TransferToken.ReceivePackage.ReceiveSizeInByte            = *ResponseDataSize;
  TransferToken.ReceivePackage.TransmitTimeoutInMillisecond = MANAGEABILITY_TRANSPORT_NO_TIMEOUT;
  TransportToken->Transport->Function.Version1_0->TransportTransmitReceive (
                                                    TransportToken,
                                                    &TransferToken
                                                    );

  if (IpmiTransportHeader != NULL) {
    FreePool ((VOID *)IpmiTransportHeader);
  }

  if (IpmiTransportTrailer != NULL) {
    FreePool ((VOID *)IpmiTransportTrailer);
  }

  if (ThisRequestData != NULL) {
    FreePool ((VOID *)ThisRequestData);
  }

  // Return transfer status.
  //
  Status                    = TransferToken.TransferStatus;
  TransportAdditionalStatus = TransferToken.TransportAdditionalStatus;
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to send IPMI command.\n", __func__));
    return Status;
  }

  if (ResponseDataSize != NULL) {
    *ResponseDataSize = TransferToken.ReceivePackage.ReceiveSizeInByte;
  }

  return Status;
}
