/** @file
  HPET Table Generator

  Copyright (c) 2017 - 2023, Arm Limited. All rights reserved.
  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - ACPI 6.5 Specification, Aug 29, 2022
  - HPET spec, version 1.0a

**/

#include <AcpiTableGenerator.h>
#include <ConfigurationManagerHelper.h>
#include <ConfigurationManagerObject.h>
#include <IndustryStandard/HighPrecisionEventTimerTable.h>
#include <Library/AcpiLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/TableHelperLib.h>
#include <Protocol/AcpiTable.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <Register/Hpet.h>

/** The AcpiHpet is a template EFI_ACPI_HIGH_PRECISION_EVENT_TIMER_TABLE_HEADER
    structure used for generating the HPET Table.
*/
STATIC
EFI_ACPI_HIGH_PRECISION_EVENT_TIMER_TABLE_HEADER  mAcpiHpet = {
  ACPI_HEADER (
    EFI_ACPI_6_5_HIGH_PRECISION_EVENT_TIMER_TABLE_SIGNATURE,
    EFI_ACPI_HIGH_PRECISION_EVENT_TIMER_TABLE_HEADER,
    EFI_ACPI_HIGH_PRECISION_EVENT_TIMER_TABLE_REVISION
    ),
  // EventTimerBlockId
  0,
  // BaseAddressLower32Bit
  { EFI_ACPI_6_5_SYSTEM_MEMORY,                             0,0, EFI_ACPI_RESERVED_BYTE, 0 },
  // HpetNumber
  0,
  // MainCounterMinimumClockTickInPeriodicMode
  0,
  // PageProtectionAndOemAttribute
  EFI_ACPI_NO_PAGE_PROTECTION
};

/** Construct the HPET table.

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
BuildHpetTable (
  IN  CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  EFI_STATUS  Status;

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
      "ERROR: HPET: Requested table revision = %d, is not supported."
      "Supported table revision: Minimum = %d, Maximum = %d\n",
      AcpiTableInfo->AcpiTableRevision,
      This->MinAcpiTableRevision,
      This->AcpiTableRevision
      ));
    return EFI_INVALID_PARAMETER;
  }

  *Table = NULL;

  Status = AddAcpiHeader (
             CfgMgrProtocol,
             This,
             (EFI_ACPI_DESCRIPTION_HEADER *)&mAcpiHpet,
             AcpiTableInfo,
             sizeof (EFI_ACPI_HIGH_PRECISION_EVENT_TIMER_TABLE_HEADER)
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: HPET: Failed to add ACPI header. Status = %r\n",
      Status
      ));
    return Status;
  }

  mAcpiHpet.BaseAddressLower32Bit.Address             = PcdGet32 (PcdHpetBaseAddress);
  mAcpiHpet.EventTimerBlockId                         = MmioRead32 (PcdGet32 (PcdHpetBaseAddress) + HPET_GENERAL_CAPABILITIES_ID_OFFSET);
  mAcpiHpet.MainCounterMinimumClockTickInPeriodicMode = (UINT16)MmioRead32 (PcdGet32 (PcdHpetBaseAddress) + HPET_GENERAL_CAPABILITIES_ID_OFFSET + 4);
  *Table                                              = (EFI_ACPI_DESCRIPTION_HEADER *)&mAcpiHpet;

  return Status;
}

/** The interface for the HPET Table Generator.
*/
STATIC
CONST
ACPI_TABLE_GENERATOR  mHpetGenerator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdHpet),
  // Generator Description
  L"ACPI.STD.HPET.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_6_5_HIGH_PRECISION_EVENT_TIMER_TABLE_SIGNATURE,
  // ACPI Table Revision supported by this Generator
  EFI_ACPI_HIGH_PRECISION_EVENT_TIMER_TABLE_REVISION,
  // Minimum supported ACPI Table Revision
  EFI_ACPI_HIGH_PRECISION_EVENT_TIMER_TABLE_REVISION,
  // Creator ID
  FixedPcdGet32 (PcdAcpiDefaultCreatorId),
  // Creator Revision
  FixedPcdGet32 (PcdAcpiDefaultCreatorRevision),
  // Build Table function
  BuildHpetTable,
  // No additional resources are allocated by the generator.
  // Hence the Free Resource function is not required.
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
AcpiHpetLibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&mHpetGenerator);
  DEBUG ((DEBUG_INFO, "HPET: Register Generator. Status = %r\n", Status));
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
AcpiHpetLibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&mHpetGenerator);
  DEBUG ((DEBUG_INFO, "HPET: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
