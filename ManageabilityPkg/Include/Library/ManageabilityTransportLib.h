/** @file

  This file defines the manageability transport interface library and functions.

  Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MANAGEABILITY_TRANSPORT_LIB_H_
#define MANAGEABILITY_TRANSPORT_LIB_H_

#define MANAGEABILITY_TRANSPORT_TOKEN_VERSION_MAJOR  1
#define MANAGEABILITY_TRANSPORT_TOKEN_VERSION_MINOR  0
#define MANAGEABILITY_TRANSPORT_TOKEN_VERSION        ((MANAGEABILITY_TRANSPORT_TOKEN_VERSION_MAJOR << 8) |\
                                                MANAGEABILITY_TRANSPORT_TOKEN_VERSION_MINOR)

#define MANAGEABILITY_TRANSPORT_PAYLOAD_SIZE_FROM_CAPABILITY(a)  (1 << ((a & MANAGEABILITY_TRANSPORT_CAPABILITY_MAXIMUM_PAYLOAD_MASK) >>\
           MANAGEABILITY_TRANSPORT_CAPABILITY_MAXIMUM_PAYLOAD_BIT_POSITION))

typedef struct  _MANAGEABILITY_TRANSPORT_FUNCTION_V1_0  MANAGEABILITY_TRANSPORT_FUNCTION_V1_0;
typedef struct  _MANAGEABILITY_TRANSPORT                MANAGEABILITY_TRANSPORT;
typedef struct  _MANAGEABILITY_TRANSPORT_TOKEN          MANAGEABILITY_TRANSPORT_TOKEN;
typedef struct  _MANAGEABILITY_TRANSFER_TOKEN           MANAGEABILITY_TRANSFER_TOKEN;

///
/// Optional transport header and trailer required
/// for the transport interface.
///
typedef VOID  *MANAGEABILITY_TRANSPORT_HEADER;
typedef VOID  *MANAGEABILITY_TRANSPORT_TRAILER;

///
/// The transport interface specific hardware information.
///

typedef union {
  UINT16    IoAddress16;
  UINT32    IoAddress32;
} MANAGEABILITY_TRANSPORT_HARDWARE_IO;

///
/// Manageability KCS protocol interface hardware information.
///
typedef struct {
  BOOLEAN                                MemoryMap;
  MANAGEABILITY_TRANSPORT_HARDWARE_IO    IoBaseAddress;
  MANAGEABILITY_TRANSPORT_HARDWARE_IO    IoDataInAddress;
  MANAGEABILITY_TRANSPORT_HARDWARE_IO    IoDataOutAddress;
  MANAGEABILITY_TRANSPORT_HARDWARE_IO    IoCommandAddress;
  MANAGEABILITY_TRANSPORT_HARDWARE_IO    IoStatusAddress;
} MANAGEABILITY_TRANSPORT_KCS_HARDWARE_INFO;
#define MANAGEABILITY_TRANSPORT_KCS_IO_MAP_IO      FALSE
#define MANAGEABILITY_TRANSPORT_KCS_MEMORY_MAP_IO  TRUE

///
/// Manageability SSIF protocol interface hardware information
///
typedef struct {
  UINT32    BmcSlaveAddress;
} MANAGEABILITY_TRANSPORT_SSIF_HARDWARE_INFO;

///
/// Manageability Serial protocol interface hardware information.
///
typedef struct {
  UINT8    IpmiRequesterAddress;
  UINT8    IpmiResponderAddress;
  UINT8    IpmiRequesterLUN;
  UINT8    IpmiResponderLUN;
} MANAGEABILITY_TRANSPORT_SERIAL_HARDWARE_INFO;

typedef union {
  VOID                                            *Pointer;
  MANAGEABILITY_TRANSPORT_KCS_HARDWARE_INFO       *Kcs;
  MANAGEABILITY_TRANSPORT_SSIF_HARDWARE_INFO      *Ssif;
  MANAGEABILITY_TRANSPORT_SERIAL_HARDWARE_INFO    *Serial;
} MANAGEABILITY_TRANSPORT_HARDWARE_INFORMATION;

///
/// Additional transport interface status.
///
typedef UINT32 MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS;
#define MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS_NO_ERRORS        0x00000000
#define MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS_ERROR            0x00000001
#define MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS_BUSY_IN_READ     0x00000002
#define MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS_BUSY_IN_WRITE    0x00000004
#define MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS_INVALID_COMMAND  0x00000008
#define MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS_NOT_AVAILABLE    0xffffffff

///
/// Additional transport interface features.
///
typedef UINT32 MANAGEABILITY_TRANSPORT_CAPABILITY;
/// Bit 0
#define MANAGEABILITY_TRANSPORT_CAPABILITY_MULTIPLE_TRANSFER_TOKENS  0x00000001
/// Bit 1
#define MANAGEABILITY_TRANSPORT_CAPABILITY_ASYNCHRONOUS_TRANSFER  0x00000002
/// Bit 2   - Reserved
/// Bit 7:3 - Transport interface maximum payload size, which is (2 ^ bit[7:3] - 1)
///           bit[7:3] means no maximum payload.
#define MANAGEABILITY_TRANSPORT_CAPABILITY_MAXIMUM_PAYLOAD_MASK           0x000000f8
#define MANAGEABILITY_TRANSPORT_CAPABILITY_MAXIMUM_PAYLOAD_BIT_POSITION   3
#define MANAGEABILITY_TRANSPORT_CAPABILITY_MAXIMUM_PAYLOAD_NOT_AVAILABLE  0x00
/// Bit 8:31 - Reserved

///
/// Definitions of Manageability transport interface functions.
/// This is a union that can accommodate the new functions
/// introduced to the Manageability transport library in the future.
/// The new added function must has its own MANAGEABILITY_TRANSPORT_FUNCTION
/// structure with the incremental version number.
///   e.g., MANAGEABILITY_TRANSPORT_FUNCTION_V1_1.
///
/// The new function must be added base on the last version of
/// MANAGEABILITY_TRANSPORT_FUNCTION to keep the backward compatability.
///
typedef union {
  MANAGEABILITY_TRANSPORT_FUNCTION_V1_0    *Version1_0;
} MANAGEABILITY_TRANSPORT_FUNCTION;

///
/// Manageability specification GUID/Name table structure
///
typedef struct {
  EFI_GUID    *SpecificationGuid;
  CHAR16      *SpecificationName;
} MANAGEABILITY_SPECIFICATION_NAME;

///
/// Definitions of Transmit/Receive package
///
typedef struct {
  UINT8     *TransmitPayload;
  UINT32    TransmitSizeInByte;
  UINT32    TransmitTimeoutInMillisecond;
} MANAGEABILITY_TRANSMIT_PACKAGE;

typedef struct {
  UINT8     *ReceiveBuffer;
  UINT32    ReceiveSizeInByte;
  UINT32    TransmitTimeoutInMillisecond;
} MANAGEABILITY_RECEIVE_PACKAGE;

///
/// Definitions of Manageability transport interface.
///
struct _MANAGEABILITY_TRANSPORT {
  EFI_GUID                            *ManageabilityTransportSpecification; ///< The Manageability Transport Interface spec.
  UINT16                              TransportVersion;                     ///< The version of transport interface
                                                                            ///< function that indicates which version
                                                                            ///< of MANAGEABILITY_TRANSPORT_FUNCTION
                                                                            ///< is unsupported by this library.
  CHAR16                              *TransportName;                       ///< Human readable string of
                                                                            ///< this transport interface.
  MANAGEABILITY_TRANSPORT_FUNCTION    Function;                             ///< Transport functions
};

///
/// Definitions of Manageability transport token.
///
struct _MANAGEABILITY_TRANSPORT_TOKEN {
  EFI_GUID                   *ManageabilityProtocolSpecification; ///< The Manageability Protocol spec.
  MANAGEABILITY_TRANSPORT    *Transport;
};

#define MANAGEABILITY_TRANSPORT_NO_TIMEOUT  0

///
/// The Manageability transport receive token used to receive
/// the response from transport interface after transmitting the
/// request.
///
struct _MANAGEABILITY_TRANSFER_TOKEN {
  EFI_EVENT    ReceiveEvent;                                              ///< The EFI event is created to
                                                                          ///< wait the signal for the readiness
                                                                          ///< of response data.
                                                                          ///< If NULL, transport library should
                                                                          ///< just return the response data in
                                                                          ///< ReceiveBuffer then the process returns
                                                                          ///< to caller. Otherwise the transport
                                                                          ///< library can signal event when the
                                                                          ///< response is ready for caller. That
                                                                          ///< means the transport library can
                                                                          ///< optionally implement the asynchronous
                                                                          ///< transfer mechanism or when the multiple
                                                                          ///< transport sessions are acquired.
  MANAGEABILITY_TRANSPORT_HEADER               TransmitHeader;            ///< This is the transport-specific header
                                                                          ///< which is sent discretely of payload.
                                                                          ///< This field can be NULL if the transport
                                                                          ///< doesn't require this.
  UINT16                                       TransmitHeaderSize;        ///< Transmit header size in byte.
  MANAGEABILITY_TRANSPORT_TRAILER              TransmitTrailer;           ///< This is the transport-specific trailer
                                                                          ///< which is sent discretely of payload.
                                                                          ///< This field can be NULL if the transport
                                                                          ///< doesn't require this.
  UINT16                                       TransmitTrailerSize;       ///< Transmit trailer size in byte.
  MANAGEABILITY_TRANSMIT_PACKAGE               TransmitPackage;           ///< The payload sent to transport interface.
  MANAGEABILITY_RECEIVE_PACKAGE                ReceivePackage;            ///< The buffer to receive the response.
  EFI_STATUS                                   TransferStatus;            ///< The EFI Status of the transfer.
  MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS    TransportAdditionalStatus; ///< The additional status of transport
                                                                          ///< interface.
};

/**
  This function acquires to create a transport session to transmit manageability
  packet. A transport token is returned to caller for the follow up operations.

  @param [in]   ManageabilityProtocolSpec  The protocol spec the transport interface is acquired for.
  @param [out]  TransportToken             The pointer to receive the transport token created by
                                           the target transport interface library.
  @retval       EFI_SUCCESS                Token is created successfully.
  @retval       EFI_OUT_OF_RESOURCES       Out of resource to create a new transport session.
  @retval       EFI_UNSUPPORTED            Protocol is not supported on this transport interface.
  @retval       Otherwise                  Other errors.

**/
EFI_STATUS
AcquireTransportSession (
  IN  EFI_GUID                       *ManageabilityProtocolSpec,
  OUT MANAGEABILITY_TRANSPORT_TOKEN  **TransportToken
  );

/**
  This function returns the transport capabilities according to
  the manageability protocol.

  @param [in]   TransportToken             Transport token acquired from manageability
                                           transport library.
  @param [out]  TransportFeature           Pointer to receive transport capabilities.
                                           See the definitions of
                                           MANAGEABILITY_TRANSPORT_CAPABILITY.
  @retval       EFI_SUCCESS                TransportCapability is returned successfully.
  @retval       EFI_INVALID_PARAMETER      TransportToken is not a valid token.
**/
EFI_STATUS
GetTransportCapability (
  IN MANAGEABILITY_TRANSPORT_TOKEN        *TransportToken,
  OUT MANAGEABILITY_TRANSPORT_CAPABILITY  *TransportCapability
  );

/**
  This function releases the manageability transport session.

  @param [in]  TransportToken   The transport token acquired through
                                AcquireTransportSession.
  @retval      EFI_SUCCESS      Token is released successfully.
  @retval      EFI_INVALID_PARAMETER  Invalid TransportToken.
  @retval      Otherwise        Other errors.

**/
EFI_STATUS
ReleaseTransportSession (
  IN MANAGEABILITY_TRANSPORT_TOKEN  *TransportToken
  );

/**
  This function initializes the transport interface.

  @param [in]  TransportToken           The transport token acquired through
                                        AcquireTransportSession function.
  @param [in]  HardwareInfo             This is the optional hardware information
                                        assigned to this transport interface.

  @retval      EFI_SUCCESS              Transport interface is initialized
                                        successfully.
  @retval      EFI_INVALID_PARAMETER    The invalid transport token.
  @retval      EFI_NOT_READY            The transport interface works fine but
  @retval                               is not ready.
  @retval      EFI_DEVICE_ERROR         The transport interface has problems.
  @retval      EFI_ALREADY_STARTED      Teh protocol interface has already initialized.
  @retval      Otherwise                Other errors.

**/
typedef
EFI_STATUS
(EFIAPI *MANAGEABILITY_TRANSPORT_INIT)(
  IN  MANAGEABILITY_TRANSPORT_TOKEN                 *TransportToken,
  IN  MANAGEABILITY_TRANSPORT_HARDWARE_INFORMATION  HardwareInfo OPTIONAL
  );

/**
  This function returns the transport interface status.
  The generic EFI_STATUS is returned to caller directly, The additional
  information of transport interface could be optionally returned in
  TransportAdditionalStatus to describes the status that can't be
  described obviously through EFI_STATUS.
  See the definition of MANAGEABILITY_TRANSPORT_STATUS.

  @param [in]   TransportToken             The transport token acquired through
                                           AcquireTransportSession function.
  @param [out]  TransportAdditionalStatus  The additional status of transport
                                           interface.
                                           NULL means no additional status of this
                                           transport interface.

  @retval      EFI_SUCCESS              Transport interface status is returned.
  @retval      EFI_INVALID_PARAMETER    The invalid transport token.
  @retval      EFI_DEVICE_ERROR         The transport interface has problems to return
  @retval                               status.
               Otherwise                Other errors.

**/
typedef
EFI_STATUS
(EFIAPI *MANAGEABILITY_TRANSPORT_STATUS)(
  IN  MANAGEABILITY_TRANSPORT_TOKEN               *TransportToken,
  OUT MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS   *TransportAdditionalStatus OPTIONAL
  );

/**
  This function resets the transport interface.
  The generic EFI_STATUS is returned to caller directly after reseting transport
  interface. The additional information of transport interface could be optionally
  returned in TransportAdditionalStatus to describes the status that can't be
  described obviously through EFI_STATUS.
  See the definition of MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS.

  @param [in]   TransportToken             The transport token acquired through
                                           AcquireTransportSession function.
  @param [out]  TransportAdditionalStatus  The additional status of specific transport
                                           interface after the reset.
                                           NULL means no additional status of this
                                           transport interface.

  @retval      EFI_SUCCESS              Transport interface status is returned.
  @retval      EFI_INVALID_PARAMETER    The invalid transport token.
  @retval      EFI_TIMEOUT              The reset process is time out.
  @retval      EFI_DEVICE_ERROR         The transport interface has problems to return
                                        status.
               Otherwise                Other errors.

**/
typedef
EFI_STATUS
(EFIAPI *MANAGEABILITY_TRANSPORT_RESET)(
  IN  MANAGEABILITY_TRANSPORT_TOKEN              *TransportToken,
  OUT MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS  *TransportAdditionalStatus OPTIONAL
  );

/**
  This function transmit the request over target transport interface.
  The generic EFI_STATUS is returned to caller directly after reseting transport
  interface. The additional information of transport interface could be optionally
  returned in TransportAdditionalStatus to describes the status that can't be
  described obviously through EFI_STATUS.
  See the definition of MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS.

  @param [in]  TransportToken           The transport token acquired through
                                        AcquireTransportSession function.
  @param [in]  TransferToken            The transfer token, see the definition of
                                        MANAGEABILITY_TRANSFER_TOKEN.

  @retval      The EFI status is returned in MANAGEABILITY_TRANSFER_TOKEN.

**/
typedef
VOID
(EFIAPI *MANAGEABILITY_TRANSPORT_TRANSMIT_RECEIVE)(
  IN  MANAGEABILITY_TRANSPORT_TOKEN       *TransportToken,
  IN  MANAGEABILITY_TRANSFER_TOKEN        *TransferToken
  );

///
/// The first version of Manageability transport interface function.
///
struct _MANAGEABILITY_TRANSPORT_FUNCTION_V1_0 {
  MANAGEABILITY_TRANSPORT_INIT                TransportInit;            ///< Initial the transport.
  MANAGEABILITY_TRANSPORT_STATUS              TransportStatus;          ///< Get the transport status.
  MANAGEABILITY_TRANSPORT_RESET               TransportReset;           ///< Reset the transport.
  MANAGEABILITY_TRANSPORT_TRANSMIT_RECEIVE    TransportTransmitReceive; ///< Transmit the packet over
                                                                        ///< transport and get the
                                                                        ///< response back.
};

#endif
