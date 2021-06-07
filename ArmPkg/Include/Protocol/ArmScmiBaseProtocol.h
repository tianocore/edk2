/** @file

  Copyright (c) 2017-2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Specification Reference:
  - Arm System Control and Management Interface - Platform Design Document
    (https://developer.arm.com/documentation/den0056/)
**/

#ifndef ARM_SCMI_BASE_PROTOCOL_H_
#define ARM_SCMI_BASE_PROTOCOL_H_

#include <Protocol/ArmScmi.h>

#define BASE_PROTOCOL_VERSION_V1  0x10000
#define BASE_PROTOCOL_VERSION_V2  0x20000

#define NUM_PROTOCOL_MASK      0xFFU
#define NUM_AGENT_MASK         0xFFU

#define NUM_AGENT_SHIFT        0x8

/** Returns total number of protocols that are
  implemented (excluding the Base protocol)
*/
#define SCMI_TOTAL_PROTOCOLS(Attr) (Attr & NUM_PROTOCOL_MASK)

// Returns total number of agents in the system.
#define SCMI_TOTAL_AGENTS(Attr)    ((Attr >> NUM_AGENT_SHIFT) & NUM_AGENT_MASK)

#define ARM_SCMI_BASE_PROTOCOL_GUID  { \
  0xd7e5abe9, 0x33ab, 0x418e, {0x9f, 0x91, 0x72, 0xda, 0xe2, 0xba, 0x8e, 0x2f} \
  }

extern EFI_GUID gArmScmiBaseProtocolGuid;

typedef struct _SCMI_BASE_PROTOCOL SCMI_BASE_PROTOCOL;

/** Return version of the Base protocol supported by SCP firmware.

  @param[in]  This     A Pointer to SCMI_BASE_PROTOCOL Instance.

  @param[out] Version  Version of the supported SCMI Base protocol.

  @retval EFI_SUCCESS       The version of the protocol is returned.
  @retval EFI_DEVICE_ERROR  SCP returns an SCMI error.
  @retval !(EFI_SUCCESS)    Other errors.
**/
typedef
EFI_STATUS
(EFIAPI *SCMI_BASE_GET_VERSION) (
  IN  SCMI_BASE_PROTOCOL  *This,
  OUT UINT32              *Version
  );

/** Return total number of SCMI protocols supported by the SCP firmware.

  @param[in]  This           A Pointer to SCMI_BASE_PROTOCOL Instance.

  @param[out] TotalProtocols Total number of SCMI protocols supported.

  @retval EFI_SUCCESS       Total number of protocols supported are returned.
  @retval EFI_DEVICE_ERROR  SCP returns a SCMI error.
  @retval !(EFI_SUCCESS)    Other errors.
**/
typedef
EFI_STATUS
(EFIAPI *SCMI_BASE_GET_TOTAL_PROTOCOLS) (
  IN  SCMI_BASE_PROTOCOL  *This,
  OUT UINT32              *TotalProtocols
  );

/** Return vendor name.

  @param[in] This           A Pointer to SCMI_BASE_PROTOCOL Instance.

  @param[out] VendorIdentifier Null terminated ASCII string of up to
                               16 bytes with a vendor name.

  @retval EFI_SUCCESS       VendorIdentifier is returned.
  @retval EFI_DEVICE_ERROR  SCP returns a SCMI error.
  @retval !(EFI_SUCCESS)    Other errors.
**/
typedef
EFI_STATUS
(EFIAPI *SCMI_BASE_DISCOVER_VENDOR) (
  IN  SCMI_BASE_PROTOCOL  *This,
  OUT UINT8               VendorIdentifier[SCMI_MAX_STR_LEN]
  );

/** Return sub vendor name.

  @param[in]  This           A Pointer to SCMI_BASE_PROTOCOL Instance.

  @param[out] VendorIdentifier Null terminated ASCII string of up to
                               16 bytes with a vendor name.

  @retval EFI_SUCCESS       VendorIdentifier is returned.
  @retval EFI_DEVICE_ERROR  SCP returns a SCMI error.
  @retval !(EFI_SUCCESS)    Other errors.
**/
typedef
EFI_STATUS
(EFIAPI *SCMI_BASE_DISCOVER_SUB_VENDOR) (
  IN  SCMI_BASE_PROTOCOL  *This,
  OUT UINT8               VendorIdentifier[SCMI_MAX_STR_LEN]
  );

/** Return implementation version.

  @param[in] This           A Pointer to SCMI_BASE_PROTOCOL Instance.

  @param[out] ImplementationVersion Vendor specific implementation version.

  @retval EFI_SUCCESS       Implementation version is returned.
  @retval EFI_DEVICE_ERROR  SCP returns a SCMI error.
  @retval !(EFI_SUCCESS)    Other errors.
**/
typedef
EFI_STATUS
(EFIAPI *SCMI_BASE_DISCOVER_IMPLEMENTATION_VERSION) (
  IN  SCMI_BASE_PROTOCOL  *This,
  OUT UINT32              *ImplementationVersion
  );

/** Return list of protocols.

  @param[in]  This           A Pointer to SCMI_BASE_PROTOCOL Instance.

  @param[out] ProtocolListSize  Size of the ProtocolList.

  @param[out] ProtocolList   Protocol list.

  @retval EFI_SUCCESS          List of protocols is returned.
  @retval EFI_BUFFER_TOO_SMALL ProtocolListSize is too small for the result.
                                It has been updated to the size needed.
  @retval EFI_DEVICE_ERROR     SCP returns a SCMI error.
  @retval !(EFI_SUCCESS)       Other errors.
**/
typedef
EFI_STATUS
(EFIAPI *SCMI_BASE_DISCOVER_LIST_PROTOCOLS) (
  IN     SCMI_BASE_PROTOCOL  *This,
  IN OUT UINT32              *ProtocolListSize,
  OUT    UINT8               *ProtocolList
  );

// Base protocol.
typedef struct _SCMI_BASE_PROTOCOL {
  SCMI_BASE_GET_VERSION                      GetVersion;
  SCMI_BASE_GET_TOTAL_PROTOCOLS              GetTotalProtocols;
  SCMI_BASE_DISCOVER_VENDOR                  DiscoverVendor;
  SCMI_BASE_DISCOVER_SUB_VENDOR              DiscoverSubVendor;
  SCMI_BASE_DISCOVER_IMPLEMENTATION_VERSION  DiscoverImplementationVersion;
  SCMI_BASE_DISCOVER_LIST_PROTOCOLS          DiscoverListProtocols;
} SCMI_BASE_PROTOCOL;

// SCMI Message IDs for Base protocol.
typedef enum {
  ScmiMessageIdBaseDiscoverVendor                 = 0x3,
  ScmiMessageIdBaseDiscoverSubVendor              = 0x4,
  ScmiMessageIdBaseDiscoverImplementationVersion  = 0x5,
  ScmiMessageIdBaseDiscoverListProtocols          = 0x6
} SCMI_MESSAGE_ID_BASE;

#endif /* ARM_SCMI_BASE_PROTOCOL_H_ */
