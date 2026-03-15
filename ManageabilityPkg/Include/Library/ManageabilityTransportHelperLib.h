/** @file

  This file defines the manageability transport interface library and functions.

  Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MANAGEABILITY_TRANSPORT_HELPER_LIB_H_
#define MANAGEABILITY_TRANSPORT_HELPER_LIB_H_

#include <Library/ManageabilityTransportLib.h>

#define DEBUG_MANAGEABILITY_INFO  DEBUG_MANAGEABILITY

typedef struct _MANAGEABILITY_PROTOCOL_NAME MANAGEABILITY_PROTOCOL_NAME;

typedef struct {
  UINT8     *PayloadPointer;
  UINT32    PayloadSize;
} MANAGEABILITY_TRANSMISSION_PACKAGE_ATTR;

//
// The information of multi portions of payload it is
// splitted according to transport interface Maximum
// Transfer Unit.
typedef struct {
  UINT16                                     NumberOfPackages; ///< Number of packages in MultiPackages.
  MANAGEABILITY_TRANSMISSION_PACKAGE_ATTR    MultiPackages[];
} MANAGEABILITY_TRANSMISSION_MULTI_PACKAGES;

/**
  Helper function returns the human readable name of Manageability specification.

  @param[out]  SpecificationGuid         The Manageability specification GUID

  @retval      !NULL  Human readable name is returned;
  @retval       NULL  No string found, the given Manageability specification is
                      not supported.
**/
CHAR16 *
HelperManageabilitySpecName (
  IN EFI_GUID  *SpecificationGuid
  );

/**
  Helper function to check if the Manageability specification is supported
  by transport interface or not.

  @param[in]  TransportGuid                         GUID of the transport interface.
  @param[in]  SupportedManageabilityProtocolArray   The Manageability protocols supported
                                                    by the transport interface.
  @param[in]  NumberOfSupportedProtocolInArray      Number of protocols in the array.
  @param[in]  ManageabilityProtocolToCheck          The Manageability specification to check.

  @retval      EFI_SUCCESS            Token is created successfully.
  @retval      EFI_INVALID_PARAMETER  Either NumberOfSupportedProtocolInArray = 0 or
                                      SupportedManageabilityProtocolArray = NULL.
  @retval      EFI_UNSUPPORTED        Out of resource to create a new transport session.
               Otherwise              Other errors.
**/
EFI_STATUS
HelperManageabilityCheckSupportedSpec (
  IN  EFI_GUID  *TransportGuid,
  IN  EFI_GUID  **SupportedManageabilityProtocolArray,
  IN  UINT8     NumberOfSupportedProtocolInArray,
  IN  EFI_GUID  *ManageabilityProtocolToCheck
  );

/**
  Helper function to acquire the Manageability transport token.

  @param[in]  ManageabilityProtocolSpec   The Manageability protocol specification.
  @param[out] TransportToken              Pointer to receive Manageability transport
                                          token.

  @retval      EFI_SUCCESS            Token is created successfully.
  @retval      EFI_OUT_OF_RESOURCES   Out of resource to create a new transport session.
  @retval      EFI_UNSUPPORTED        Token is created successfully.
  @retval      EFI_DEVICE_ERROR       The transport interface has problems
  @retval      EFI_INVALID_PARAMETER  INput parameter is not valid.
               Otherwise              Other errors.
**/
EFI_STATUS
HelperAcquireManageabilityTransport (
  IN  EFI_GUID                       *ManageabilityProtocolSpec,
  OUT MANAGEABILITY_TRANSPORT_TOKEN  **TransportToken
  );

/**
  Helper function to initial the transport interface.

  @param[in]  TransportToken              Transport token.
  @param[in]  HardwareInfo                Optional hardware information of transport interface.
  @param[out] TransportAdditionalStatus   Transport additional status.

  @retval      EFI_SUCCESS            Transport interface is initiated successfully.
  @retval      EFI_DEVICE_ERROR       The transport interface has problems
  @retval      EFI_INVALID_PARAMETER  INput parameter is not valid.
               Otherwise              Other errors.
**/
EFI_STATUS
HelperInitManageabilityTransport (
  IN  MANAGEABILITY_TRANSPORT_TOKEN                 *TransportToken,
  IN  MANAGEABILITY_TRANSPORT_HARDWARE_INFORMATION  HardwareInfo OPTIONAL,
  OUT MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS     *TransportAdditionalStatus OPTIONAL
  );

/**
  This function splits payload into multiple packages according to
  the given transport interface Maximum Transfer Unit (MTU).

  @param[in]  PreambleSize         The additional data size precedes
                                   each package.
  @param[in]  PostambleSize        The additional data size succeeds
                                   each package.
  @param[in]  Payload              Pointer to payload.
  @param[in]  PayloadSize          Payload size in byte.
  @param[in]  MaximumTransferUnit  MTU of transport interface.
  @param[out] MultiplePackages     Pointer to receive
                                   MANAGEABILITY_TRANSMISSION_MULTI_PACKAGES
                                   structure. Caller has to free the memory
                                   allocated for MANAGEABILITY_TRANSMISSION_MULTI_PACKAGES.

  @retval   EFI_SUCCESS          MANAGEABILITY_TRANSMISSION_MULTI_PACKAGES structure
                                 is returned successfully.
  @retval   EFI_OUT_OF_RESOURCE  Not enough resource to create
                                 MANAGEABILITY_TRANSMISSION_MULTI_PACKAGES structure.
**/
EFI_STATUS
HelperManageabilitySplitPayload (
  IN UINT16                                      PreambleSize,
  IN UINT16                                      PostambleSize,
  IN UINT8                                       *Payload,
  IN UINT32                                      PayloadSize,
  IN UINT32                                      MaximumTransferUnit,
  OUT MANAGEABILITY_TRANSMISSION_MULTI_PACKAGES  **MultiplePackages
  );

/**
  This function generates CRC8 with given polynomial.

  @param[in]  Polynomial       Polynomial in 8-bit.
  @param[in]  CrcInitialValue  CRC initial value.
  @param[in]  BufferStart      Pointer to buffer starts the CRC calculation.
  @param[in]  BufferSize       Size of buffer.

  @retval  UINT8 CRC value.
**/
UINT8
HelperManageabilityGenerateCrc8 (
  IN UINT8   Polynomial,
  IN UINT8   CrcInitialValue,
  IN UINT8   *BufferStart,
  IN UINT32  BufferSize
  );

/**
  Print out manageability transmit payload to the debug output device.

  @param[in]  Payload      Payload to print.
  @param[in]  PayloadSize  Payload size.

**/
VOID
EFIAPI
HelperManageabilityPayLoadDebugPrint (
  IN  VOID    *Payload,
  IN  UINT32  PayloadSize
  );

/**
  Prints a debug message and manageability payload to the debug output device.

  @param[in]  Payload      Payload to print.
  @param[in]  PayloadSize  Payload size.
  @param[in]  Format       The format string for the debug message to print.
  @param[in]  ...          The variable argument list whose contents are accessed
                           based on the format string specified by Format.

**/
VOID
HelperManageabilityDebugPrint (
  IN  VOID         *Payload,
  IN  UINT32       PayloadSize,
  IN  CONST CHAR8  *Format,
  ...
  );

///
/// IPMI Helper Functions.
///

/**
  This function returns a human readable string of IPMI KCS Completion Code
  and returns the corresponding additional status of transport interface.

  @param [in]  CompletionCode     The Completion Code returned from KCS.
  @param [out] CompletionCodeStr  Human readable string of IPMI Completion Code.
  @param [out] AdditionalStatus   Return the addtional status.

  @retval  EFI_SUCCESS            The information of Completion Code is returned.
  @retval  EFI_NOT_FOUND          No information of Completion Code is returned.
  @retval  EFI_INVALID_PARAMETER  The given parameter is incorrect.

**/
EFI_STATUS
IpmiHelperCheckCompletionCode (
  IN   UINT8                                      CompletionCode,
  OUT  CHAR16                                     **CompletionCodeStr,
  OUT  MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS  *AdditionalStatus
  );

#endif
