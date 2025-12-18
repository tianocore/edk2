/** @file

    IPMI Manageability Protocol common header file.

  Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MANAGEABILITY_IPMI_COMMON_H_
#define MANAGEABILITY_IPMI_COMMON_H_

#include <IndustryStandard/IpmiKcs.h>
#include <Library/ManageabilityTransportLib.h>

///
/// IPMI KCS hardware information.
///
#define IPMI_KCS_BASE_ADDRESS  PcdGet16 (PcdIpmiKcsIoBaseAddress)
#define IPMI_KCS_REG_DATA_IN   IPMI_KCS_BASE_ADDRESS + IPMI_KCS_DATA_IN_REGISTER_OFFSET
#define IPMI_KCS_REG_DATA_OUT  IPMI_KCS_BASE_ADDRESS + IPMI_KCS_DATA_OUT_REGISTER_OFFSET
#define IPMI_KCS_REG_COMMAND   IPMI_KCS_BASE_ADDRESS + IPMI_KCS_COMMAND_REGISTER_OFFSET
#define IPMI_KCS_REG_STATUS    IPMI_KCS_BASE_ADDRESS + IPMI_KCS_STATUS_REGISTER_OFFSET

///
/// IPMI SSIF hardware information.
///
#define IPMI_SSIF_BMC_SLAVE_ADDRESS FixedPcdGet8 (PcdIpmiSsifSmbusSlaveAddr)

///
/// IPMI Serial hardware information.
///
#define IPMI_SERIAL_REQUESTER_ADDRESS  FixedPcdGet8 (PcdIpmiSerialRequesterAddress)
#define IPMI_SERIAL_RESPONDER_ADDRESS  FixedPcdGet8 (PcdIpmiSerialResponderAddress)
#define IPMI_SERIAL_REQUESTER_LUN      FixedPcdGet8 (PcdIpmiSerialRequesterLun)
#define IPMI_SERIAL_RESPONDER_LUN      FixedPcdGet8 (PcdIpmiSerialResponderLun)

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
**/
EFI_STATUS
SetupIpmiTransportHardwareInformation (
  IN   MANAGEABILITY_TRANSPORT_TOKEN                 *TransportToken,
  OUT  MANAGEABILITY_TRANSPORT_HARDWARE_INFORMATION  *HardwareInformation
  );

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
  );

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
  );

#endif
