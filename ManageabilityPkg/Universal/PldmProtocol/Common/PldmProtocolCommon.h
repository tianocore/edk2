/** @file

  EDKII PLDM Protocol common header file.

  Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MANAGEABILITY_EDKII_PLDM_COMMON_H_
#define MANAGEABILITY_EDKII_PLDM_COMMON_H_

#include <IndustryStandard/Pldm.h>
#include <Library/ManageabilityTransportLib.h>

#define GET_PLDM_MESSAGE_PAYLOAD_SIZE(PayloadSize)  (PayloadSize - sizeof (PLDM_RESPONSE_HEADER))
#define GET_PLDM_MESSAGE_PAYLOAD_PTR(PayloadPtr)    ((UINT8 *)PayloadPtr + sizeof (PLDM_RESPONSE_HEADER))

typedef struct {
  UINT8     PldmType;
  UINT8     PldmCommand;
  UINT32    ResponseSize;
} PLDM_MESSAGE_PACKET_MAPPING;

/**
  This functions setup the PLDM transport hardware information according
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
SetupPldmTransportHardwareInformation (
  IN   MANAGEABILITY_TRANSPORT_TOKEN                 *TransportToken,
  OUT  MANAGEABILITY_TRANSPORT_HARDWARE_INFORMATION  *HardwareInformation
  );

/**
  This functions setup the final header/body/trailer packets for
  the acquired transport interface.

  @param[in]         TransportToken     The transport interface.
  @param[in]         PldmType           PLDM message type.
  @param[in]         PldmCommand        PLDM command of this PLDM type.
  @param[in]         SourceId           PLDM source teminus ID.
  @param[in]         DestinationId      PLDM destination teminus ID.
  @param[out]        PacketHeader       The pointer to receive header of request.
  @param[out]        PacketHeaderSize   Packet header size in bytes.
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
  @param[out]        PacketTrailerSize  Packet trailer size in bytes.

  @retval EFI_SUCCESS            Request packet is returned.
  @retval EFI_UNSUPPORTED        Request packet is not returned because
                                 the unsupported transport interface.
**/
EFI_STATUS
SetupPldmRequestTransportPacket (
  IN   MANAGEABILITY_TRANSPORT_TOKEN    *TransportToken,
  IN   UINT8                            PldmType,
  IN   UINT8                            PldmCommand,
  IN   UINT8                            SourceId,
  IN   UINT8                            DestinationId,
  OUT  MANAGEABILITY_TRANSPORT_HEADER   *PacketHeader,
  OUT  UINT16                           *PacketHeaderSize,
  IN OUT UINT8                          **PacketBody,
  IN OUT UINT32                         *PacketBodySize,
  OUT  MANAGEABILITY_TRANSPORT_TRAILER  *PacketTrailer,
  OUT  UINT16                           *PacketTrailerSize
  );

/**
  Common code to submit PLDM commands

  @param[in]         TransportToken             Transport token.
  @param[in]         PldmType                   PLDM message type.
  @param[in]         PldmCommand                PLDM command of this PLDM type.
  @param[in]         PldmTerminusSourceId       PLDM source teminus ID.
  @param[in]         PldmTerminusDestinationId  PLDM destination teminus ID.
  @param[in]         RequestData                Command Request Data.
  @param[in]         RequestDataSize            Size of Command Request Data.
  @param[out]        ResponseData               Command Response Data. The completion code is the first byte of response data.
  @param[in, out]    ResponseDataSize           Size of Command Response Data.

  @retval EFI_SUCCESS            The command byte stream was successfully submit to the device and a response was successfully received.
  @retval EFI_NOT_FOUND          The command was not successfully sent to the device or a response was not successfully received from the device.
  @retval EFI_NOT_READY          PLDM transport interface is not ready for PLDM command access.
  @retval EFI_DEVICE_ERROR       PLDM Device hardware error.
  @retval EFI_TIMEOUT            The command time out.
  @retval EFI_UNSUPPORTED        The command was not successfully sent to the device.
  @retval EFI_OUT_OF_RESOURCES   The resource allocation is out of resource or data size error.
**/
EFI_STATUS
CommonPldmSubmitCommand (
  IN     MANAGEABILITY_TRANSPORT_TOKEN  *TransportToken,
  IN     UINT8                          PldmType,
  IN     UINT8                          PldmCommand,
  IN     UINT8                          PldmTerminusSourceId,
  IN     UINT8                          PldmTerminusDestinationId,
  IN     UINT8                          *RequestData OPTIONAL,
  IN     UINT32                         RequestDataSize,
  OUT    UINT8                          *ResponseData OPTIONAL,
  IN OUT UINT32                         *ResponseDataSize
  );

#endif // MANAGEABILITY_EDKII_PLDM_COMMON_H_
