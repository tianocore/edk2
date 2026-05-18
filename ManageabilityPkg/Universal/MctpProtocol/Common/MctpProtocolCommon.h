/** @file
    MCTP Manageability Protocol common header file.

  Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MANAGEABILITY_MCTP_COMMON_H_
#define MANAGEABILITY_MCTP_COMMON_H_

#include <IndustryStandard/IpmiKcs.h>
#include <Library/ManageabilityTransportLib.h>

#define MCTP_KCS_BASE_ADDRESS  PcdGet32(PcdMctpKcsBaseAddress)

// For I/O mapped I/O
#define MCTP_KCS_REG_DATA_IN_IO   MCTP_KCS_BASE_ADDRESS + IPMI_KCS_DATA_IN_REGISTER_OFFSET
#define MCTP_KCS_REG_DATA_OUT_IO  MCTP_KCS_BASE_ADDRESS + IPMI_KCS_DATA_OUT_REGISTER_OFFSET
#define MCTP_KCS_REG_COMMAND_IO   MCTP_KCS_BASE_ADDRESS + IPMI_KCS_COMMAND_REGISTER_OFFSET
#define MCTP_KCS_REG_STATUS_IO    MCTP_KCS_BASE_ADDRESS + IPMI_KCS_STATUS_REGISTER_OFFSET

// For memory mapped I/O
#define MCTP_KCS_REG_DATA_IN_MEMMAP   MCTP_KCS_BASE_ADDRESS + (IPMI_KCS_DATA_IN_REGISTER_OFFSET * 4)
#define MCTP_KCS_REG_DATA_OUT_MEMMAP  MCTP_KCS_BASE_ADDRESS + (IPMI_KCS_DATA_OUT_REGISTER_OFFSET * 4)
#define MCTP_KCS_REG_COMMAND_MEMMAP   MCTP_KCS_BASE_ADDRESS + (IPMI_KCS_COMMAND_REGISTER_OFFSET * 4)
#define MCTP_KCS_REG_STATUS_MEMMAP    MCTP_KCS_BASE_ADDRESS + (IPMI_KCS_STATUS_REGISTER_OFFSET * 4)

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
SetupMctpTransportHardwareInformation (
  IN   MANAGEABILITY_TRANSPORT_TOKEN                 *TransportToken,
  OUT  MANAGEABILITY_TRANSPORT_HARDWARE_INFORMATION  *HardwareInformation
  );

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
  IN OUT UINT8                          **PacketBody,
  IN OUT UINT32                         *PacketBodySize,
  OUT  MANAGEABILITY_TRANSPORT_TRAILER  *PacketTrailer,
  OUT  UINT16                           *PacketTrailerSize
  );

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
  );

#endif
