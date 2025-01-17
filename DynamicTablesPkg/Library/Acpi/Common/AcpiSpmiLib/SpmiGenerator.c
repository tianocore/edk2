/** @file
  SPMI Table Generator

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - IPMI - Revision 2.0, April 21, 2015.

**/

#include <IndustryStandard/ServiceProcessorManagementInterfaceTable.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/AcpiTable.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Library/TableHelperLib.h>
#include <IndustryStandard/IpmiNetFnApp.h>
#include <Library/IpmiCommandLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

/** Standard SPMI Generator

Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - EArchCommonObjSpmiInterfaceInfo
  - EArchCommonObjSpmiInterruptDeviceInfo (OPTIONAL)
*/

/** Retrieve the SPMI interface information. */
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjSpmiInterfaceInfo,
  CM_ARCH_COMMON_SPMI_INTERFACE_INFO
  );

/** Retrieve the SPMI interrupt and device information. */
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjSpmiInterruptDeviceInfo,
  CM_ARCH_COMMON_SPMI_INTERRUPT_DEVICE_INFO
  );

STATIC
EFI_ACPI_SERVICE_PROCESSOR_MANAGEMENT_INTERFACE_TABLE  AcpiSpmi = {
  ACPI_HEADER (
    EFI_ACPI_6_5_SERVER_PLATFORM_MANAGEMENT_INTERFACE_TABLE_SIGNATURE,
    EFI_ACPI_SERVICE_PROCESSOR_MANAGEMENT_INTERFACE_TABLE,
    EFI_ACPI_SERVICE_PROCESSOR_MANAGEMENT_INTERFACE_5_TABLE_REVISION
    ),
  /// Interface Type
  ///   0 - Reserved
  ///   1 - KCS
  ///   2 - SMIC
  ///   3 - BT
  ///   4 - SSIF
  ///   5-255 - Reserved
  0x00,
  /// Reserved1, must be 0x01 as per the IPMI specification.
  0x01,
  /// Specification Revision
  0x0200,
  /// Interrupt Type
  0x00,
  /// GPE Number
  0x00,
  /// Reserved2
  0x00,
  /// PCI device flag
  0x00,
  /// Global System Interrupt
  0x00,
  /// Base Address
  { 0,                                                                0,0, 0, 0 },
  /// Device ID
  {
    { 0x00 }
  },
  /// Reserved3
  0x00
};

/** Construct the SPMI ACPI table.

  This function invokes the Configuration Manager protocol interface
  to get the required hardware information for generating the ACPI
  table.

  If this function allocates any resources then they must be freed
  in the FreeXXXXTableResources function.

  @param [in]  This           Pointer to the table generator.
  @param [in]  AcpiTableInfo  Pointer to the ACPI Table Info.
  @param [in]  CfgMgrProtocol Pointer to the Configuration Manager
                                Protocol Interface.
  @param [out] Table          Pointer to the constructed ACPI Table.

  @retval EFI_SUCCESS           Table generated successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object was not found.
  @retval EFI_BAD_BUFFER_SIZE   The size returned by the Configuration
                                Manager is less than the Object size for the
                                requested object.
**/
STATIC
EFI_STATUS
EFIAPI
BuildSpmiTable (
  IN  CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  EFI_STATUS                                 Status;
  CM_ARCH_COMMON_SPMI_INTERFACE_INFO         *SpmiInfo;
  CM_ARCH_COMMON_SPMI_INTERRUPT_DEVICE_INFO  *SpmiIntrDeviceInfo;
  IPMI_GET_DEVICE_ID_RESPONSE                DeviceId;

  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Table != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  if ((AcpiTableInfo->AcpiTableRevision < This->MinAcpiTableRevision) ||
      (AcpiTableInfo->AcpiTableRevision > This->AcpiTableRevision))
  {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SPMI: Requested table revision = %d, is not supported."
      "Supported table revision: Minimum = %d, Maximum = %d\n",
      AcpiTableInfo->AcpiTableRevision,
      This->MinAcpiTableRevision,
      This->AcpiTableRevision
      ));
    return EFI_INVALID_PARAMETER;
  }

  *Table = NULL;

  Status = GetEArchCommonObjSpmiInterfaceInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &SpmiInfo,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SPMI: Failed to retrieve interface type and base address.\n"
      ));
    return Status;
  }

  /// Validate interface type.
  if ((SpmiInfo->InterfaceType < EFI_ACPI_SPMI_INTERFACE_TYPE_KCS) ||
      (SpmiInfo->InterfaceType > EFI_ACPI_SPMI_INTERFACE_TYPE_SSIF))
  {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SPMI: The Interface Type is invalid. Type = %d\n",
      SpmiInfo->InterfaceType
      ));
    return EFI_INVALID_PARAMETER;
  }

  /// If the interface type is SSIF, the Address Space ID should be SMBUS.
  if ((SpmiInfo->InterfaceType == EFI_ACPI_SPMI_INTERFACE_TYPE_SSIF) &&
      (SpmiInfo->BaseAddress.AddressSpaceId != EFI_ACPI_6_5_SMBUS))
  {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SPMI: Invalid Address Space ID for SSIF. ID = %d\n",
      SpmiInfo->BaseAddress.AddressSpaceId
      ));
    return EFI_INVALID_PARAMETER;
  }

  /// For non-ssif interface types, the Address Space ID should be System Memory or System I/O.
  if ((SpmiInfo->InterfaceType != EFI_ACPI_SPMI_INTERFACE_TYPE_SSIF) &&
      ((SpmiInfo->BaseAddress.AddressSpaceId != EFI_ACPI_6_5_SYSTEM_MEMORY) &&
       (SpmiInfo->BaseAddress.AddressSpaceId != EFI_ACPI_6_5_SYSTEM_IO)))
  {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SPMI: Invalid Address Space ID. ID = %d\n",
      SpmiInfo->BaseAddress.AddressSpaceId
      ));
    return EFI_INVALID_PARAMETER;
  }

  Status = GetEArchCommonObjSpmiInterruptDeviceInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &SpmiIntrDeviceInfo,
             NULL
             );
  if (!EFI_ERROR (Status)) {
    /// Validate Interrupt Type, bit[7:2] should be zero.
    if ((SpmiIntrDeviceInfo->InterruptType >> 2) != 0 ) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SPMI: The Interrupt Type has non-zero reserved bits. InterruptType = 0x%x\n",
        SpmiIntrDeviceInfo->InterruptType
        ));
      return EFI_INVALID_PARAMETER;
    }

    if (SpmiInfo->InterfaceType == EFI_ACPI_SPMI_INTERFACE_TYPE_SSIF) {
      /// Interrupt Type bit[0] should be zero for SSIF interface type.
      if ((SpmiIntrDeviceInfo->InterruptType & BIT0) != 0) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: SPMI: The Interrupt Type bit0 should be zero for SSIF interface type.\n"
          ));
        return EFI_INVALID_PARAMETER;
      }

      /// PCI device flag bit0 should be zero for SSIF interface type.
      if ((SpmiIntrDeviceInfo->PciDeviceFlag & BIT0) != 0) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: SPMI: PCI Device Flag is invalid for SSIF interface type.\n"
          ));
        return EFI_INVALID_PARAMETER;
      }
    }

    /// Validate SCI GPE bit if GPE number is provided.
    if ((SpmiIntrDeviceInfo->Gpe != 0) &&
        ((SpmiIntrDeviceInfo->InterruptType & BIT0) == 0))
    {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SPMI: The Interrupt Type bit0 should be set if a GPE number is provided.\n"
        ));
      return EFI_INVALID_PARAMETER;
    }

    /// If GlobalSystemInterrupt is provided, the interrupt type should be GSI.
    if ((SpmiIntrDeviceInfo->GlobalSystemInterrupt != 0) &&
        ((SpmiIntrDeviceInfo->InterruptType & BIT1) ==  0))
    {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SPMI: Invalid interrupt type = 0x%x for GSI 0x%x\n",
        SpmiIntrDeviceInfo->InterruptType,
        SpmiIntrDeviceInfo->GlobalSystemInterrupt
        ));
      return EFI_INVALID_PARAMETER;
    }

    AcpiSpmi.InterruptType         = SpmiIntrDeviceInfo->InterruptType;
    AcpiSpmi.Gpe                   = SpmiIntrDeviceInfo->Gpe;
    AcpiSpmi.PciDeviceFlag         = SpmiIntrDeviceInfo->PciDeviceFlag;
    AcpiSpmi.GlobalSystemInterrupt = SpmiIntrDeviceInfo->GlobalSystemInterrupt;
    AcpiSpmi.DeviceId.Uid          = SpmiIntrDeviceInfo->DeviceId;
  } else {
    DEBUG ((
      DEBUG_INFO,
      "INFO: SPMI: The platform does not provide interrupt and PCI device information.\n"
      ));
    DEBUG ((
      DEBUG_INFO,
      "Using default values (0) for the interrupt and PCI device information.\n"
      ));
  }

  /// Update IPMI specification version
  Status = IpmiGetDeviceId (&DeviceId);
  if (!EFI_ERROR (Status) && (DeviceId.CompletionCode == IPMI_COMP_CODE_NORMAL)) {
    AcpiSpmi.SpecificationRevision  = DeviceId.SpecificationVersion & 0xF0;
    AcpiSpmi.SpecificationRevision |= (DeviceId.SpecificationVersion & 0xF) << 8;
  }

  AcpiSpmi.InterfaceType = SpmiInfo->InterfaceType;
  CopyMem (
    &AcpiSpmi.BaseAddress,
    &SpmiInfo->BaseAddress,
    sizeof (EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE)
    );

  Status = AddAcpiHeader (
             CfgMgrProtocol,
             This,
             (EFI_ACPI_DESCRIPTION_HEADER *)&AcpiSpmi,
             AcpiTableInfo,
             sizeof (EFI_ACPI_SERVICE_PROCESSOR_MANAGEMENT_INTERFACE_TABLE)
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SPMI: Failed to add ACPI header. Status = %r\n",
      Status
      ));
  }

  *Table = (EFI_ACPI_DESCRIPTION_HEADER *)&AcpiSpmi;
  return Status;
}

/** This macro defines the SPMI Table Generator revision.
*/
#define SPMI_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** The interface for the SPMI Table Generator.
*/
STATIC
CONST
ACPI_TABLE_GENERATOR  SpmiGenerator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSpmi),
  // Generator Description
  L"ACPI.STD.SPMI.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_6_5_SERVER_PLATFORM_MANAGEMENT_INTERFACE_TABLE_SIGNATURE,
  // ACPI Table Revision supported by this Generator
  EFI_ACPI_SERVICE_PROCESSOR_MANAGEMENT_INTERFACE_5_TABLE_REVISION,
  // Minimum supported ACPI Table Revision
  EFI_ACPI_SERVICE_PROCESSOR_MANAGEMENT_INTERFACE_5_TABLE_REVISION,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID,
  // Creator Revision
  SPMI_GENERATOR_REVISION,
  // Build Table function
  BuildSpmiTable,
  // Free Resource function
  NULL,
  // Extended build function not needed
  NULL,
  // Extended build function not implemented by the generator.
  // Hence extended free resource function is not required.
  NULL
};

/** Register the Generator with the ACPI Table Factory.

  @param [in]  ImageHandle  The handle to the image.
  @param [in]  SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS           The Generator is registered.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_ALREADY_STARTED   The Generator for the Table ID
                                is already registered.
**/
EFI_STATUS
EFIAPI
AcpiSpmiLibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&SpmiGenerator);
  DEBUG ((DEBUG_INFO, "SPMI: Register Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/** Deregister the Generator from the ACPI Table Factory.

  @param [in]  ImageHandle  The handle to the image.
  @param [in]  SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS           The Generator is deregistered.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The Generator is not registered.
**/
EFI_STATUS
EFIAPI
AcpiSpmiLibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&SpmiGenerator);
  DEBUG ((DEBUG_INFO, "SPMI: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
