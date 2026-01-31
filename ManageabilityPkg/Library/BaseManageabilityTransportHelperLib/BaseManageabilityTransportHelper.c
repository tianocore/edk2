/** @file
  Manageability Transport Helper Library

  Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ManageabilityTransportHelperLib.h>

//
// BaseManageabilityTransportHelper is used by PEI, DXE and SMM.
// Make sure the global variables added here should be unchangeable.
//
MANAGEABILITY_SPECIFICATION_NAME  ManageabilitySpecNameTable[] = {
  { &gManageabilityTransportKcsGuid,         L"KCS"      },
  { &gManageabilityTransportSmbusI2cGuid,    L"SMBUS I2C"},
  { &gManageabilityTransportPciVdmGuid,      L"PCI VDM"  },
  { &gManageabilityTransportSerialGuid,      L"SERIAL"   },
  { &gManageabilityTransportMctpGuid,        L"MCTP"     },
  { &gManageabilityProtocolIpmiGuid,         L"IPMI"     },
  { &gManageabilityProtocolMctpGuid,         L"MCTP"     },
  { &gManageabilityProtocolPldmGuid,         L"PLDM"     }
};

UINT16  mManageabilitySpecNum = sizeof (ManageabilitySpecNameTable)/ sizeof (MANAGEABILITY_SPECIFICATION_NAME);

/**
  Helper function returns the human readable name of Manageability specification.

  @param[in]  SpecificationGuid         The Manageability specification GUID

  @retval      !NULL  Human readable name is returned;
  @retval       NULL  No string found, the given Manageability specification is
                      not supported.
**/
CHAR16 *
HelperManageabilitySpecName (
  IN EFI_GUID  *SpecificationGuid
  )
{
  UINT16                            Index;
  MANAGEABILITY_SPECIFICATION_NAME  *ThisSpec;

  if (mManageabilitySpecNum == 0) {
    return NULL;
  }

  if ((SpecificationGuid == NULL) || IsZeroGuid (SpecificationGuid)) {
    DEBUG ((DEBUG_ERROR, "%a: Improper input GUIDs, could be NULL or zero GUID.\n", __func__));
    return NULL;
  }

  ThisSpec = ManageabilitySpecNameTable;
  for (Index = 0; Index < mManageabilitySpecNum; Index++) {
    if (CompareGuid (
          SpecificationGuid,
          ThisSpec->SpecificationGuid
          ))
    {
      return ThisSpec->SpecificationName;
    }

    ThisSpec++;
  }

  return NULL;
}

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
  )
{
  UINT16    Index;
  EFI_GUID  **ThisSpecGuid;

  if ((NumberOfSupportedProtocolInArray == 0) || (SupportedManageabilityProtocolArray == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((TransportGuid == NULL) ||
      IsZeroGuid (TransportGuid) ||
      (ManageabilityProtocolToCheck == NULL) ||
      IsZeroGuid (ManageabilityProtocolToCheck)
      )
  {
    DEBUG ((DEBUG_ERROR, "%a: Improper input GUIDs, could be NULL or zero GUID.\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  ThisSpecGuid = SupportedManageabilityProtocolArray;
  for (Index = 0; Index < NumberOfSupportedProtocolInArray; Index++) {
    if (CompareGuid (
          *ThisSpecGuid,
          ManageabilityProtocolToCheck
          ))
    {
      DEBUG ((
        DEBUG_MANAGEABILITY_INFO,
        "%a: Transport interface %s supports %s manageability specification.\n",
        __func__,
        HelperManageabilitySpecName (TransportGuid),
        HelperManageabilitySpecName (ManageabilityProtocolToCheck)
        ));
      return EFI_SUCCESS;
    }

    ThisSpecGuid++;
  }

  DEBUG ((
    DEBUG_ERROR,
    "%a: Transport interface %s doesn't support %s manageability specification.\n",
    __func__,
    HelperManageabilitySpecName (TransportGuid),
    HelperManageabilitySpecName (ManageabilityProtocolToCheck)
    ));
  return EFI_UNSUPPORTED;
}

/**
  Helper function to acquire the Manageability transport token.

  @param[in]  ManageabilityProtocolSpec   The Manageability protocol specification.
  @param[out] TransportToken              Pointer to receive Manageability transport
                                          token.

  @retval      EFI_SUCCESS            Token is created successfully.
  @retval      EFI_OUT_OF_RESOURCES   Out of resource to create a new transport session.
  @retval      EFI_UNSUPPORTED        Token is created successfully.
  @retval      EFI_INVALID_PARAMETER  Input parameter is not valid.
               Otherwise              Other errors.
**/
EFI_STATUS
HelperAcquireManageabilityTransport (
  IN  EFI_GUID                       *ManageabilityProtocolSpec,
  OUT MANAGEABILITY_TRANSPORT_TOKEN  **TransportToken
  )
{
  EFI_STATUS  Status;
  CHAR16      *ManageabilityProtocolName;
  CHAR16      *ManageabilityTransportName;

  DEBUG ((DEBUG_MANAGEABILITY_INFO, "%a: Entry\n", __func__));
  if ((TransportToken == NULL) || (ManageabilityProtocolSpec == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: One of the required input parameters is NULL.\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  *TransportToken           = NULL;
  ManageabilityProtocolName = HelperManageabilitySpecName (ManageabilityProtocolSpec);
  if (ManageabilityProtocolName == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Unsupported Manageability Protocol Specification.\n", __func__));
    return EFI_UNSUPPORTED;
  }

  DEBUG ((DEBUG_MANAGEABILITY_INFO, "  Manageability protocol %s is going to acquire transport interface token...\n", ManageabilityProtocolName));

  Status = AcquireTransportSession (ManageabilityProtocolSpec, TransportToken);
  if (Status == EFI_UNSUPPORTED) {
    DEBUG ((DEBUG_ERROR, "%a: No supported transport interface for %s packet.\n", __func__, ManageabilityProtocolName));
    return Status;
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Fail to acquire Manageability transport token for %s (%r).\n",
      __func__,
      ManageabilityProtocolName,
      Status
      ));
    return Status;
  }

  ManageabilityTransportName = HelperManageabilitySpecName ((*TransportToken)->Transport->ManageabilityTransportSpecification);
  if (ManageabilityTransportName == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Unsupported Manageability Transport Interface Specification\n", __func__));
    return EFI_UNSUPPORTED;
  }

  DEBUG ((DEBUG_MANAGEABILITY_INFO, "%a: This is the transfer session for %s over %s\n", __func__, ManageabilityProtocolName, ManageabilityTransportName));
  return Status;
}

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
  )
{
  EFI_STATUS  Status;

  if (TransportToken == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: TransportToken is invalid.\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  // Initial transport interface.
  Status = TransportToken->Transport->Function.Version1_0->TransportInit (TransportToken, HardwareInfo);
  if ((Status != EFI_SUCCESS) && (Status != EFI_ALREADY_STARTED)) {
    if (Status == EFI_DEVICE_ERROR) {
      // Try to reset the transport and initialize it again.
      Status = TransportToken->Transport->Function.Version1_0->TransportReset (
                                                                 TransportToken,
                                                                 TransportAdditionalStatus
                                                                 );
      if (EFI_ERROR (Status)) {
        if (Status == EFI_UNSUPPORTED) {
          DEBUG ((DEBUG_ERROR, "%a: Transport interface doesn't have reset capability.\n", __func__));
        } else {
          DEBUG ((DEBUG_ERROR, "%a: Fail to reset transport interface (%r).\n", __func__, Status));
        }

        Status = EFI_DEVICE_ERROR;
      } else {
        Status = TransportToken->Transport->Function.Version1_0->TransportInit (TransportToken, HardwareInfo);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "%a: Transport interface is not able to use after the reset (%r).\n", __func__, Status));
        }
      }
    } else {
      DEBUG ((DEBUG_ERROR, "%a: Transport interface is not able to use (%r).\n", __func__, Status));
    }
  }

  return Status;
}

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
  )
{
  UINT8   BitIndex;
  UINT32  BufferIndex;

  BufferIndex = 0;
  while (BufferIndex < BufferSize) {
    CrcInitialValue = CrcInitialValue ^ *(BufferStart + BufferIndex);
    BufferIndex++;

    for (BitIndex = 0; BitIndex < 8; BitIndex++) {
      if ((CrcInitialValue & 0x80) != 0) {
        CrcInitialValue = (CrcInitialValue << 1) ^ Polynomial;
      } else {
        CrcInitialValue <<= 1;
      }
    }
  }

  return CrcInitialValue;
}

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
  )
{
  UINT16                                     NumberOfPackages;
  UINT16                                     IndexOfPackage;
  UINT32                                     PackagePayloadSize;
  UINT32                                     TotalPayloadRemaining;
  MANAGEABILITY_TRANSMISSION_MULTI_PACKAGES  *ThisMultiplePackages;
  MANAGEABILITY_TRANSMISSION_PACKAGE_ATTR    *ThisPackage;

  if ((INT16)(MaximumTransferUnit - PreambleSize - PostambleSize) < 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: (Preamble 0x%x + PostambleSize 0x%x) is greater than MaximumTransferUnit 0x%x.\n",
      __func__,
      PreambleSize,
      PostambleSize,
      MaximumTransferUnit
      ));
    return EFI_INVALID_PARAMETER;
  }

  PackagePayloadSize   = MaximumTransferUnit -PreambleSize - PostambleSize;
  NumberOfPackages     = (UINT16)((PayloadSize + (PackagePayloadSize - 1)) / PackagePayloadSize);
  ThisMultiplePackages = (MANAGEABILITY_TRANSMISSION_MULTI_PACKAGES *)AllocateZeroPool (
                                                                        sizeof (MANAGEABILITY_TRANSMISSION_MULTI_PACKAGES) +
                                                                        sizeof (MANAGEABILITY_TRANSMISSION_PACKAGE_ATTR) * NumberOfPackages
                                                                        );
  if (ThisMultiplePackages == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Not enough memory for MANAGEABILITY_TRANSMISSION_MULTI_PACKAGES\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  ThisMultiplePackages->NumberOfPackages = NumberOfPackages;
  ThisPackage                            = (MANAGEABILITY_TRANSMISSION_PACKAGE_ATTR *)(ThisMultiplePackages + 1);
  TotalPayloadRemaining                  = PayloadSize;
  for (IndexOfPackage = 0; IndexOfPackage < NumberOfPackages; IndexOfPackage++) {
    ThisPackage->PayloadPointer = Payload + (IndexOfPackage * PackagePayloadSize);
    ThisPackage->PayloadSize    = MIN (TotalPayloadRemaining, PackagePayloadSize);
    TotalPayloadRemaining      -= ThisPackage->PayloadSize;
    ThisPackage++;
  }

  if (TotalPayloadRemaining != 0) {
    DEBUG ((DEBUG_ERROR, "%a: Error processing multiple packages (TotalPayloadRemaining != 0)\n", __func__));
    FreePool (ThisMultiplePackages);
    return EFI_INVALID_PARAMETER;
  }

  *MultiplePackages = ThisMultiplePackages;
  return EFI_SUCCESS;
}

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
  )
{
  UINT16  Page256;
  UINT16  Row16;
  UINT16  Column16;
  UINT32  RemainingBytes;
  UINT32  TotalBytePrinted;

  RemainingBytes   = PayloadSize;
  TotalBytePrinted = 0;
  while (TRUE) {
    if (TotalBytePrinted % 256 == 0) {
      Page256 = (UINT16)TotalBytePrinted / 256;
      DEBUG ((DEBUG_MANAGEABILITY_INFO, "======== Manageability Payload %04xH - %04xH =========\n", Page256 * 256, Page256 * 256 + MIN (RemainingBytes, 256) - 1));
      DEBUG ((DEBUG_MANAGEABILITY_INFO, "       "));
      for (Column16 = 0; Column16 < 16; Column16++) {
        DEBUG ((DEBUG_MANAGEABILITY_INFO, "%02x ", Column16));
      }

      DEBUG ((DEBUG_MANAGEABILITY_INFO, "\n       -----------------------------------------------\n"));
    }

    for (Row16 = 0; Row16 < 16; Row16++) {
      DEBUG ((DEBUG_MANAGEABILITY_INFO, "%04x | ", Page256 * 256 + Row16 * 16));
      for (Column16 = 0; Column16 < MIN (RemainingBytes, 16); Column16++) {
        DEBUG ((DEBUG_MANAGEABILITY_INFO, "%02x ", *((UINT8 *)Payload + Page256 * 256 + Row16 * 16 + Column16)));
      }

      RemainingBytes   -= Column16;
      TotalBytePrinted += Column16;
      if (RemainingBytes == 0) {
        DEBUG ((DEBUG_MANAGEABILITY_INFO, "\n\n"));
        return;
      }

      DEBUG ((DEBUG_MANAGEABILITY_INFO, "\n"));
    }

    DEBUG ((DEBUG_MANAGEABILITY_INFO, "\n"));
  }

  DEBUG ((DEBUG_MANAGEABILITY_INFO, "\n\n"));
}

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
  )
{
  VA_LIST  Marker;

  VA_START (Marker, Format);
  DEBUG ((DEBUG_MANAGEABILITY_INFO, "Manageability Transmission: "));
  DebugVPrint ((UINTN)DEBUG_MANAGEABILITY_INFO, Format, Marker);
  HelperManageabilityPayLoadDebugPrint (Payload, PayloadSize);
  VA_END (Marker);
}
