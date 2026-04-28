/** @file

  This file defines the manageability MCTP protocol specific transport data.

  Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MANAGEABILITY_TRANSPORT_MCTP_LIB_H_
#define MANAGEABILITY_TRANSPORT_MCTP_LIB_H_

#include <Library/ManageabilityTransportLib.h>

#define MCTP_KCS_HEADER_VERSION  0x01

// According to SMBUS spec, the polynomial is:
// C(x) = X^8 + X^2 + X^1 + 1, which is 0x107,
// just ignore bit8 in definition.
#define MCTP_KCS_PACKET_ERROR_CODE_POLY  0x07

///
/// The MCTP Message header which is apart from
/// the payload.
///

typedef struct {
  UINT8    IntegrityCheck : 1; ///< Message integrity check.
  UINT8    MessageType    : 7; ///< Message type.
} MANAGEABILITY_MCTP_MESSAGE_HEADER;

typedef struct {
  UINT8                                SourceEndpointId;
  UINT8                                DestinationEndpointId;
  MANAGEABILITY_MCTP_MESSAGE_HEADER    MessageHeader;
} MANAGEABILITY_MCTP_TRANSPORT_HEADER;

typedef struct {
  UINT8    NetFunc;      ///< Message integrity check.
  UINT8    DefiningBody; ///< Message type.
  UINT8    ByteCount;    ///< Byte count of payload.
} MANAGEABILITY_MCTP_KCS_HEADER;

typedef struct {
  UINT8    Pec;  ///< MCTP over KCS Packet Error Code.
} MANAGEABILITY_MCTP_KCS_TRAILER;

#define MCTP_KCS_NETFN_LUN                       0xb0
#define DEFINING_BODY_DMTF_PRE_OS_WORKING_GROUP  0x01

// This is used to track the response message. This value
// is not defined by the specification.
#define MCTP_MESSAGE_TAG  0x1

#define MCTP_MESSAGE_TAG_OWNER_REQUEST   1
#define MCTP_MESSAGE_TAG_OWNER_RESPONSE  0

#define MCTP_PACKET_SEQUENCE_MASK  0x3

#endif // MANAGEABILITY_TRANSPORT_MCTP_LIB_H_
