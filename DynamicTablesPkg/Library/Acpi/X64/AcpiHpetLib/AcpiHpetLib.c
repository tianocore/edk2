/** @file

  Generate ACPI HPET table for AMD platforms.

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier BSD-2-Clause-Patent
**/

#include <IndustryStandard/HighPrecisionEventTimerTable.h>
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerHelper.h>
#include <ConfigurationManagerObject.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/TableHelperLib.h>
#include <Protocol/AcpiTable.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <X64NameSpaceObjects.h>
#include <Library/IoLib.h>

/** This macro defines supported HPET page protection flags
*/
#define HPET_VALID_PAGE_PROTECTION \
  (EFI_ACPI_NO_PAGE_PROTECTION | \
   EFI_ACPI_4KB_PAGE_PROTECTION | \
   EFI_ACPI_64KB_PAGE_PROTECTION)

/** This macro expands to a function that retrieves the
    HPET device information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceX64,
  EX64ObjHpetInfo,
  CM_X64_HPET_INFO
  );

/** The ACPI HPET Table.
*/
STATIC
EFI_ACPI_HIGH_PRECISION_EVENT_TIMER_TABLE_HEADER  AcpiHpet = {
  ACPI_HEADER (
    EFI_ACPI_6_5_HIGH_PRECISION_EVENT_TIMER_TABLE_SIGNATURE,
    EFI_ACPI_HIGH_PRECISION_EVENT_TIMER_TABLE_HEADER,
    EFI_ACPI_HIGH_PRECISION_EVENT_TIMER_TABLE_REVISION
    ),
  // EventTimerBlockId,
  0,
  // BaseAddressLower32Bit
  { EFI_ACPI_6_5_SYSTEM_MEMORY,                             64,0, EFI_ACPI_RESERVED_BYTE, 0 },
  // HpetNumber
  0,
  // MainCounterMinimumClockTickInPeriodicMode
  0,
  // PageProtectionAndOemAttribute
  EFI_ACPI_NO_PAGE_PROTECTION
};

/** Update HPET table information.

  @param [in]  CfgMgrProtocol Pointer to the Configuration Manager
                              Protocol Interface.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object was not found or
                                the HPET is not enabled.
  @retval EFI_BAD_BUFFER_SIZE   The size returned by the Configuration
                                Manager is less than the Object size for the
                                requested object.
  @retval EFI_UNSUPPORTED       If invalid protection and oem flags provided.
**/
STATIC
EFI_STATUS
EFIAPI
HpetUpdateTableInfo (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol
  )
{
  CM_X64_HPET_INFO                              *HpetInfo;
  EFI_ACPI_HIGH_PRECISION_EVENT_TIMER_BLOCK_ID  HpetBlockId;
  EFI_STATUS                                    Status;

  ASSERT (CfgMgrProtocol != NULL);

  // Get the HPET information from the Platform Configuration Manager
  Status = GetEX64ObjHpetInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &HpetInfo,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: HPET: Failed to get HPET information." \
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  DEBUG ((
    DEBUG_INFO,
    "HPET: Device base address = 0x%x\n"
    "    : Minimum clock tick in periodic mode = 0x%x\n"
    "    : Page protection and Oem flags = 0x%x\n",
    HpetInfo->BaseAddressLower32Bit,
    HpetInfo->MainCounterMinimumClockTickInPeriodicMode,
    HpetInfo->PageProtectionAndOemAttribute
    ));

  // Validate the page protection flags bit0 to bit3
  if (((HpetInfo->PageProtectionAndOemAttribute & 0xF) & ~HPET_VALID_PAGE_PROTECTION) != 0) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: HPET: unsupported page protection flags = 0x%x\n",
      HpetInfo->PageProtectionAndOemAttribute
      ));
    ASSERT_EFI_ERROR (EFI_UNSUPPORTED);
    return EFI_UNSUPPORTED;
  }

  // Get HPET Capabilities ID register value and test if HPET is enabled
  HpetBlockId.Uint32 = MmioRead32 (HpetInfo->BaseAddressLower32Bit);

  // If mmio address is not mapped
  if ((HpetBlockId.Uint32 == MAX_UINT32) || (HpetBlockId.Uint32 == 0)) {
    DEBUG ((DEBUG_ERROR, "HPET Capabilities register read failed.\n"));
    ASSERT_EFI_ERROR (EFI_NOT_FOUND);
    return EFI_NOT_FOUND;
  }

  // Validate Reserved and Revision ID
  if (HpetBlockId.Bits.Reserved != 0) {
    DEBUG ((DEBUG_ERROR, "HPET Reserved bit is set.\n"));
    ASSERT_EFI_ERROR (EFI_UNSUPPORTED);
    return EFI_UNSUPPORTED;
  }

  if (HpetBlockId.Bits.Revision == 0) {
    DEBUG ((DEBUG_ERROR, "HPET Revision is not set.\n"));
    ASSERT_EFI_ERROR (EFI_UNSUPPORTED);
    return EFI_UNSUPPORTED;
  }

  // Fill the Event Timer Block ID
  AcpiHpet.EventTimerBlockId = HpetBlockId.Uint32;

  // Fill the Base Address
  AcpiHpet.BaseAddressLower32Bit.Address = HpetInfo->BaseAddressLower32Bit;

  // Minimum clock tick in periodic mode
  AcpiHpet.MainCounterMinimumClockTickInPeriodicMode = HpetInfo->MainCounterMinimumClockTickInPeriodicMode;

  // Page protection and OEM attribute
  AcpiHpet.PageProtectionAndOemAttribute = HpetInfo->PageProtectionAndOemAttribute;

  return Status;
}

/** Construct the HPET table.

  This function invokes the Configuration Manager protocol interface
  to get the required information for generating the ACPI table.

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
             (EFI_ACPI_DESCRIPTION_HEADER *)&AcpiHpet,
             AcpiTableInfo,
             sizeof (EFI_ACPI_HIGH_PRECISION_EVENT_TIMER_TABLE_HEADER)
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: HPET: Failed to add ACPI header. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  // Update HPET table info
  Status = HpetUpdateTableInfo (CfgMgrProtocol);
  if (EFI_ERROR (Status)) {
    goto error_handler;
  }

  *Table = (EFI_ACPI_DESCRIPTION_HEADER *)&AcpiHpet;
error_handler:
  return Status;
}

/** This macro defines the HPET Table Generator revision.
*/
#define HPET_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** The interface for the HPET Table Generator.
*/
STATIC
CONST
ACPI_TABLE_GENERATOR  HpetGenerator = {
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
  TABLE_GENERATOR_CREATOR_ID,
  // Creator Revision
  HPET_GENERATOR_REVISION,
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

  Status = RegisterAcpiTableGenerator (&HpetGenerator);
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

  Status = DeregisterAcpiTableGenerator (&HpetGenerator);
  DEBUG ((DEBUG_INFO, "HPET: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
