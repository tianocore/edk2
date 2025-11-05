/** @file

  Generate ACPI HPET table for AMD platforms.

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier BSD-2-Clause-Patent
**/

#include <IndustryStandard/HighPrecisionEventTimerTable.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/AcpiTable.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Library/AcpiHelperLib.h>
#include <Library/TableHelperLib.h>
#include <Library/AmlLib/AmlLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>
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

/** Update HPET table information.

  @param [in]  CfgMgrProtocol   Pointer to the Configuration Manager
                                Protocol Interface.
  @param [in, out] ScopeNode    The Scope Node for the HPET table.

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
SsdtHpetUpdateTableInfo (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN  OUT   AML_OBJECT_NODE_HANDLE                        ScopeNode
  )
{
  CM_X64_HPET_INFO                              *HpetInfo;
  EFI_ACPI_HIGH_PRECISION_EVENT_TIMER_BLOCK_ID  HpetBlockId;
  EFI_STATUS                                    Status;
  AML_OBJECT_NODE_HANDLE                        CrsNode;
  AML_OBJECT_NODE_HANDLE                        HpetNode;
  UINT32                                        EisaId;

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

  Status = AmlCodeGenDevice ("HPET", ScopeNode, &HpetNode);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  Status = AmlGetEisaIdFromString ("PNP0103", &EisaId);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  Status = AmlCodeGenNameInteger ("_HID", EisaId, HpetNode, NULL);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  Status = AmlCodeGenNameInteger ("_UID", 0x00, HpetNode, NULL);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  Status = AmlCodeGenNameResourceTemplate ("_CRS", HpetNode, &CrsNode);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  Status = AmlCodeGenRdMemory32Fixed (
             FALSE,
             HpetInfo->BaseAddressLower32Bit,
             SIZE_1KB,
             CrsNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  if ((HpetInfo->PageProtectionAndOemAttribute & 0xF) != 0) {
    Status = AmlCodeGenNameInteger (
               "PAGE",
               (HpetInfo->PageProtectionAndOemAttribute & 0xF),
               HpetNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }
  }

  if ((HpetInfo->PageProtectionAndOemAttribute >> 4) != 0) {
    Status = AmlCodeGenNameInteger (
               "ATTR",
               (HpetInfo->PageProtectionAndOemAttribute >> 4),
               HpetNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }
  }

  return Status;
}

/** Construct the SSDT HPET table.

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
BuildSsdtHpetTable (
  IN  CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  EFI_STATUS              Status;
  AML_ROOT_NODE_HANDLE    RootNode;
  AML_OBJECT_NODE_HANDLE  ScopeNode;

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

  Status = AddSsdtAcpiHeader (
             CfgMgrProtocol,
             This,
             AcpiTableInfo,
             &RootNode
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = AmlCodeGenScope ("\\_SB_", RootNode, &ScopeNode);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto exit_handler;
  }

  // Update HPET table info
  Status = SsdtHpetUpdateTableInfo (CfgMgrProtocol, ScopeNode);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto exit_handler;
  }

  Status = AmlSerializeDefinitionBlock (
             RootNode,
             Table
             );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-HPET: Failed to Serialize SSDT Table Data."
      " Status = %r\n",
      Status
      ));
  }

exit_handler:
  // Delete the RootNode and its attached children.
  AmlDeleteTree (RootNode);
  return Status;
}

/** Free any resources allocated for constructing the
    SSDT HPET ACPI table.

  @param [in]      This           Pointer to the table generator.
  @param [in]      AcpiTableInfo  Pointer to the ACPI Table Info.
  @param [in]      CfgMgrProtocol Pointer to the Configuration Manager
                                  Protocol Interface.
  @param [in, out] Table          Pointer to the ACPI Table.

  @retval EFI_SUCCESS           The resources were freed successfully.
  @retval EFI_INVALID_PARAMETER The table pointer is NULL or invalid.
**/
STATIC
EFI_STATUS
EFIAPI
FreeSsdtHpetTableResources (
  IN      CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN OUT        EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  if ((Table == NULL) || (*Table == NULL)) {
    DEBUG ((DEBUG_ERROR, "ERROR: SSDT-HPET: Invalid Table Pointer\n"));
    ASSERT ((Table != NULL) && (*Table != NULL));
    return EFI_INVALID_PARAMETER;
  }

  FreePool (*Table);
  *Table = NULL;
  return EFI_SUCCESS;
}

/** This macro defines the HPET Table Generator revision.
*/
#define HPET_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** The interface for the HPET Table Generator.
*/
STATIC
CONST
ACPI_TABLE_GENERATOR  SsdtHpetGenerator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSsdtHpet),
  // Generator Description
  L"ACPI.STD.SSDT.HPET.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_6_5_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
  // ACPI Table Revision supported by this Generator
  0,
  // Minimum supported ACPI Table Revision
  0,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID,
  // Creator Revision
  HPET_GENERATOR_REVISION,
  // Build Table function
  BuildSsdtHpetTable,
  // Free Resource function
  FreeSsdtHpetTableResources,
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
AcpiSsdtHpetLibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&SsdtHpetGenerator);
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
AcpiSsdtHpetLibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&SsdtHpetGenerator);
  DEBUG ((DEBUG_INFO, "HPET: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
