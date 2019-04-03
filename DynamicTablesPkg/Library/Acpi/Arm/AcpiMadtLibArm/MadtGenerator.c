/** @file
  MADT Table Generator

  Copyright (c) 2017 - 2019, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - ACPI 6.2 Specification - Errata A, September 2017

**/

#include <Library/AcpiLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/AcpiTable.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

/** ARM standard MADT Generator

Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - EArmObjGicCInfo
  - EArmObjGicDInfo
  - EArmObjGicMsiFrameInfo (OPTIONAL)
  - EArmObjGicRedistributorInfo (OPTIONAL)
  - EArmObjGicItsInfo (OPTIONAL)
*/

/** This macro expands to a function that retrieves the GIC
    CPU interface Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjGicCInfo,
  CM_ARM_GICC_INFO
  );

/** This macro expands to a function that retrieves the GIC
    Distributor Information from the Configuration Manager.
*/

GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjGicDInfo,
  CM_ARM_GICD_INFO
  );

/** This macro expands to a function that retrieves the GIC
    MSI Frame Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjGicMsiFrameInfo,
  CM_ARM_GIC_MSI_FRAME_INFO
  );

/** This macro expands to a function that retrieves the GIC
    Redistributor Information from the Configuration Manager.
*/

GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjGicRedistributorInfo,
  CM_ARM_GIC_REDIST_INFO
  );

/** This macro expands to a function that retrieves the GIC
    Interrupt Translation Service Information from the
    Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjGicItsInfo,
  CM_ARM_GIC_ITS_INFO
  );

/** This function updates the GIC CPU Interface Information in the
    EFI_ACPI_6_2_GIC_STRUCTURE structure.

  @param [in]  Gicc      Pointer to GIC CPU Interface structure.
  @param [in]  GicCInfo  Pointer to the GIC CPU Interface Information.
**/
STATIC
VOID
AddGICC (
  IN  EFI_ACPI_6_2_GIC_STRUCTURE  * CONST Gicc,
  IN  CONST CM_ARM_GICC_INFO      * CONST GicCInfo
  )
{
  ASSERT (Gicc != NULL);
  ASSERT (GicCInfo != NULL);

  // UINT8 Type
  Gicc->Type = EFI_ACPI_6_2_GIC;
  // UINT8 Length
  Gicc->Length = sizeof (EFI_ACPI_6_2_GIC_STRUCTURE);
  // UINT16 Reserved
  Gicc->Reserved = EFI_ACPI_RESERVED_WORD;

  // UINT32 CPUInterfaceNumber
  Gicc->CPUInterfaceNumber = GicCInfo->CPUInterfaceNumber;
  // UINT32 AcpiProcessorUid
  Gicc->AcpiProcessorUid = GicCInfo->AcpiProcessorUid;
  // UINT32 Flags
  Gicc->Flags = GicCInfo->Flags;
  // UINT32 ParkingProtocolVersion
  Gicc->ParkingProtocolVersion = GicCInfo->ParkingProtocolVersion;
  // UINT32 PerformanceInterruptGsiv
  Gicc->PerformanceInterruptGsiv = GicCInfo->PerformanceInterruptGsiv;
  // UINT64 ParkedAddress
  Gicc->ParkedAddress = GicCInfo->ParkedAddress;

  // UINT64 PhysicalBaseAddress
  Gicc->PhysicalBaseAddress = GicCInfo->PhysicalBaseAddress;
  // UINT64 GICV
  Gicc->GICV = GicCInfo->GICV;
  // UINT64 GICH
  Gicc->GICH = GicCInfo->GICH;

  // UINT32 VGICMaintenanceInterrupt
  Gicc->VGICMaintenanceInterrupt = GicCInfo->VGICMaintenanceInterrupt;
  // UINT64 GICRBaseAddress
  Gicc->GICRBaseAddress = GicCInfo->GICRBaseAddress;

  // UINT64 MPIDR
  Gicc->MPIDR = GicCInfo->MPIDR;
  // UINT8 ProcessorPowerEfficiencyClass
  Gicc->ProcessorPowerEfficiencyClass =
    GicCInfo->ProcessorPowerEfficiencyClass;
  // UINT8 Reserved2[3]
  Gicc->Reserved2[0] = EFI_ACPI_RESERVED_BYTE;
  Gicc->Reserved2[1] = EFI_ACPI_RESERVED_BYTE;
  Gicc->Reserved2[2] = EFI_ACPI_RESERVED_BYTE;
}

/** Add the GIC CPU Interface Information to the MADT Table.

  @param [in]  Gicc      Pointer to GIC CPU Interface
                         structure list.
  @param [in]  GicCInfo  Pointer to the GIC CPU
                         Information list.
  @param [in]  GicCCount Count of GIC CPU Interfaces.
**/
STATIC
VOID
AddGICCList (
  IN  EFI_ACPI_6_2_GIC_STRUCTURE  * Gicc,
  IN  CONST CM_ARM_GICC_INFO      * GicCInfo,
  IN        UINT32                  GicCCount
  )
{
  ASSERT (Gicc != NULL);
  ASSERT (GicCInfo != NULL);

  while (GicCCount-- != 0) {
    AddGICC (Gicc++, GicCInfo++);
  }
}

/** Update the GIC Distributor Information in the MADT Table.

  @param [in]  Gicd      Pointer to GIC Distributor structure.
  @param [in]  GicDInfo  Pointer to the GIC Distributor Information.
**/
STATIC
VOID
AddGICD (
  EFI_ACPI_6_2_GIC_DISTRIBUTOR_STRUCTURE  * CONST Gicd,
  CONST CM_ARM_GICD_INFO                  * CONST GicDInfo
)
{
  ASSERT (Gicd != NULL);
  ASSERT (GicDInfo != NULL);

  // UINT8 Type
  Gicd->Type = EFI_ACPI_6_2_GICD;
  // UINT8 Length
  Gicd->Length = sizeof (EFI_ACPI_6_2_GIC_DISTRIBUTOR_STRUCTURE);
  // UINT16 Reserved
  Gicd->Reserved1 = EFI_ACPI_RESERVED_WORD;
  // UINT32 Identifier
  // One, and only one, GIC distributor structure must be present
  // in the MADT for an ARM based system
  Gicd->GicId = 0;
  // UINT64 PhysicalBaseAddress
  Gicd->PhysicalBaseAddress = GicDInfo->PhysicalBaseAddress;
  // UINT32 VectorBase
  Gicd->SystemVectorBase = EFI_ACPI_RESERVED_DWORD;
  // UINT8  GicVersion
  Gicd->GicVersion = GicDInfo->GicVersion;
  // UINT8  Reserved2[3]
  Gicd->Reserved2[0] = EFI_ACPI_RESERVED_BYTE;
  Gicd->Reserved2[1] = EFI_ACPI_RESERVED_BYTE;
  Gicd->Reserved2[2] = EFI_ACPI_RESERVED_BYTE;
}

/** Update the GIC MSI Frame Information.

  @param [in]  GicMsiFrame      Pointer to GIC MSI Frame structure.
  @param [in]  GicMsiFrameInfo  Pointer to the GIC MSI Frame Information.
**/
STATIC
VOID
AddGICMsiFrame (
  IN  EFI_ACPI_6_2_GIC_MSI_FRAME_STRUCTURE  * CONST GicMsiFrame,
  IN  CONST CM_ARM_GIC_MSI_FRAME_INFO       * CONST GicMsiFrameInfo
)
{
  ASSERT (GicMsiFrame != NULL);
  ASSERT (GicMsiFrameInfo != NULL);

  GicMsiFrame->Type = EFI_ACPI_6_2_GIC_MSI_FRAME;
  GicMsiFrame->Length = sizeof (EFI_ACPI_6_2_GIC_MSI_FRAME_STRUCTURE);
  GicMsiFrame->Reserved1 = EFI_ACPI_RESERVED_WORD;
  GicMsiFrame->GicMsiFrameId = GicMsiFrameInfo->GicMsiFrameId;
  GicMsiFrame->PhysicalBaseAddress = GicMsiFrameInfo->PhysicalBaseAddress;

  GicMsiFrame->Flags = GicMsiFrameInfo->Flags;
  GicMsiFrame->SPICount = GicMsiFrameInfo->SPICount;
  GicMsiFrame->SPIBase = GicMsiFrameInfo->SPIBase;
}

/** Add the GIC MSI Frame Information to the MADT Table.

  @param [in]  GicMsiFrame      Pointer to GIC MSI Frame structure list.
  @param [in]  GicMsiFrameInfo  Pointer to the GIC MSI Frame info list.
  @param [in]  GicMsiFrameCount Count of GIC MSI Frames.
**/
STATIC
VOID
AddGICMsiFrameInfoList (
  IN  EFI_ACPI_6_2_GIC_MSI_FRAME_STRUCTURE  * GicMsiFrame,
  IN  CONST CM_ARM_GIC_MSI_FRAME_INFO       * GicMsiFrameInfo,
  IN        UINT32                            GicMsiFrameCount
)
{
  ASSERT (GicMsiFrame != NULL);
  ASSERT (GicMsiFrameInfo != NULL);

  while (GicMsiFrameCount-- != 0) {
    AddGICMsiFrame (GicMsiFrame++, GicMsiFrameInfo++);
  }
}

/** Update the GIC Redistributor Information.

  @param [in]  Gicr                 Pointer to GIC Redistributor structure.
  @param [in]  GicRedisributorInfo  Pointer to the GIC Redistributor Info.
**/
STATIC
VOID
AddGICRedistributor (
  IN  EFI_ACPI_6_2_GICR_STRUCTURE   * CONST Gicr,
  IN  CONST CM_ARM_GIC_REDIST_INFO  * CONST GicRedisributorInfo
  )
{
  ASSERT (Gicr != NULL);
  ASSERT (GicRedisributorInfo != NULL);

  Gicr->Type = EFI_ACPI_6_2_GICR;
  Gicr->Length = sizeof (EFI_ACPI_6_2_GICR_STRUCTURE);
  Gicr->Reserved = EFI_ACPI_RESERVED_WORD;
  Gicr->DiscoveryRangeBaseAddress =
    GicRedisributorInfo->DiscoveryRangeBaseAddress;
  Gicr->DiscoveryRangeLength = GicRedisributorInfo->DiscoveryRangeLength;
}

/** Add the GIC Redistributor Information to the MADT Table.

  @param [in]  Gicr      Pointer to GIC Redistributor structure list.
  @param [in]  GicRInfo  Pointer to the GIC Distributor info list.
  @param [in]  GicRCount Count of GIC Distributors.
**/
STATIC
VOID
AddGICRedistributorList (
  IN  EFI_ACPI_6_2_GICR_STRUCTURE   * Gicr,
  IN  CONST CM_ARM_GIC_REDIST_INFO  * GicRInfo,
  IN        UINT32                    GicRCount
)
{
  ASSERT (Gicr != NULL);
  ASSERT (GicRInfo != NULL);

  while (GicRCount-- != 0) {
    AddGICRedistributor (Gicr++, GicRInfo++);
  }
}

/** Update the GIC Interrupt Translation Service Information

  @param [in]  GicIts      Pointer to GIC ITS structure.
  @param [in]  GicItsInfo  Pointer to the GIC ITS Information.
**/
STATIC
VOID
AddGICInterruptTranslationService (
  IN  EFI_ACPI_6_2_GIC_ITS_STRUCTURE  * CONST GicIts,
  IN  CONST CM_ARM_GIC_ITS_INFO       * CONST GicItsInfo
)
{
  ASSERT (GicIts != NULL);
  ASSERT (GicItsInfo != NULL);

  GicIts->Type = EFI_ACPI_6_2_GIC_ITS;
  GicIts->Length = sizeof (EFI_ACPI_6_2_GIC_ITS_STRUCTURE);
  GicIts->Reserved = EFI_ACPI_RESERVED_WORD;
  GicIts->GicItsId = GicItsInfo->GicItsId;
  GicIts->PhysicalBaseAddress = GicItsInfo->PhysicalBaseAddress;
  GicIts->Reserved2 = EFI_ACPI_RESERVED_DWORD;
}

/** Add the GIC Interrupt Translation Service Information
    to the MADT Table.

  @param [in]  GicIts       Pointer to GIC ITS structure list.
  @param [in]  GicItsInfo   Pointer to the GIC ITS list.
  @param [in]  GicItsCount  Count of GIC ITS.
**/
STATIC
VOID
AddGICItsList (
  IN  EFI_ACPI_6_2_GIC_ITS_STRUCTURE  * GicIts,
  IN  CONST CM_ARM_GIC_ITS_INFO       * GicItsInfo,
  IN        UINT32                      GicItsCount
)
{
  ASSERT (GicIts != NULL);
  ASSERT (GicItsInfo != NULL);

  while (GicItsCount-- != 0) {
    AddGICInterruptTranslationService (GicIts++, GicItsInfo++);
  }
}

/** Construct the MADT ACPI table.

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
BuildMadtTable (
  IN  CONST ACPI_TABLE_GENERATOR                  * CONST This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            * CONST AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  * CONST CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER          ** CONST Table
  )
{
  EFI_STATUS                   Status;
  UINT32                       TableSize;
  UINT32                       GicCCount;
  UINT32                       GicDCount;
  UINT32                       GicMSICount;
  UINT32                       GicRedistCount;
  UINT32                       GicItsCount;
  CM_ARM_GICC_INFO           * GicCInfo;
  CM_ARM_GICD_INFO           * GicDInfo;
  CM_ARM_GIC_MSI_FRAME_INFO  * GicMSIInfo;
  CM_ARM_GIC_REDIST_INFO     * GicRedistInfo;
  CM_ARM_GIC_ITS_INFO        * GicItsInfo;
  UINT32                       GicCOffset;
  UINT32                       GicDOffset;
  UINT32                       GicMSIOffset;
  UINT32                       GicRedistOffset;
  UINT32                       GicItsOffset;

  EFI_ACPI_6_2_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER  * Madt;

  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Table != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  if ((AcpiTableInfo->AcpiTableRevision < This->MinAcpiTableRevision) ||
      (AcpiTableInfo->AcpiTableRevision > This->AcpiTableRevision)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: MADT: Requested table revision = %d, is not supported."
      "Supported table revision: Minimum = %d, Maximum = %d\n",
      AcpiTableInfo->AcpiTableRevision,
      This->MinAcpiTableRevision,
      This->AcpiTableRevision
      ));
    return EFI_INVALID_PARAMETER;
  }

  *Table = NULL;

  Status = GetEArmObjGicCInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &GicCInfo,
             &GicCCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: MADT: Failed to get GICC Info. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  if (GicCCount == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: MADT: GIC CPU Interface information not provided.\n"
      ));
    ASSERT (GicCCount != 0);
    Status = EFI_INVALID_PARAMETER;
    goto error_handler;
  }

  Status = GetEArmObjGicDInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &GicDInfo,
             &GicDCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: MADT: Failed to get GICD Info. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  if (GicDCount == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: MADT: GIC Distributor information not provided.\n"
      ));
    ASSERT (GicDCount != 0);
    Status = EFI_INVALID_PARAMETER;
    goto error_handler;
  }

  if (GicDCount > 1) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: MADT: One, and only one, GIC distributor must be present."
      "GicDCount = %d\n",
      GicDCount
      ));
    ASSERT (GicDCount <= 1);
    Status = EFI_INVALID_PARAMETER;
    goto error_handler;
  }

  Status = GetEArmObjGicMsiFrameInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &GicMSIInfo,
             &GicMSICount
             );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: MADT: Failed to get GIC MSI Info. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  Status = GetEArmObjGicRedistributorInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &GicRedistInfo,
             &GicRedistCount
             );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: MADT: Failed to get GIC Redistributor Info. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  Status = GetEArmObjGicItsInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &GicItsInfo,
             &GicItsCount
             );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: MADT: Failed to get GIC ITS Info. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  TableSize = sizeof (EFI_ACPI_6_2_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER);

  GicCOffset = TableSize;
  TableSize += (sizeof (EFI_ACPI_6_2_GIC_STRUCTURE) * GicCCount);

  GicDOffset = TableSize;
  TableSize += (sizeof (EFI_ACPI_6_2_GIC_DISTRIBUTOR_STRUCTURE) * GicDCount);

  GicMSIOffset = TableSize;
  TableSize += (sizeof (EFI_ACPI_6_2_GIC_MSI_FRAME_STRUCTURE) * GicMSICount);

  GicRedistOffset = TableSize;
  TableSize += (sizeof (EFI_ACPI_6_2_GICR_STRUCTURE) * GicRedistCount);

  GicItsOffset = TableSize;
  TableSize += (sizeof (EFI_ACPI_6_2_GIC_ITS_STRUCTURE) * GicItsCount);

  // Allocate the Buffer for MADT table
  *Table = (EFI_ACPI_DESCRIPTION_HEADER*)AllocateZeroPool (TableSize);
  if (*Table == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: MADT: Failed to allocate memory for MADT Table, Size = %d," \
      " Status = %r\n",
      TableSize,
      Status
      ));
    goto error_handler;
  }

  Madt = (EFI_ACPI_6_2_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER*)*Table;

  DEBUG ((
    DEBUG_INFO,
    "MADT: Madt = 0x%p TableSize = 0x%x\n",
    Madt,
    TableSize
    ));

  Status = AddAcpiHeader (
             CfgMgrProtocol,
             This,
             &Madt->Header,
             AcpiTableInfo,
             TableSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: MADT: Failed to add ACPI header. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  AddGICCList (
    (EFI_ACPI_6_2_GIC_STRUCTURE*)((UINT8*)Madt + GicCOffset),
    GicCInfo,
    GicCCount
    );

  AddGICD (
    (EFI_ACPI_6_2_GIC_DISTRIBUTOR_STRUCTURE*)((UINT8*)Madt + GicDOffset),
    GicDInfo
    );

  if (GicMSICount != 0) {
    AddGICMsiFrameInfoList (
      (EFI_ACPI_6_2_GIC_MSI_FRAME_STRUCTURE*)((UINT8*)Madt + GicMSIOffset),
      GicMSIInfo,
      GicMSICount
      );
  }

  if (GicRedistCount != 0) {
    AddGICRedistributorList (
      (EFI_ACPI_6_2_GICR_STRUCTURE*)((UINT8*)Madt + GicRedistOffset),
      GicRedistInfo,
      GicRedistCount
      );
  }

  if (GicItsCount != 0) {
    AddGICItsList (
      (EFI_ACPI_6_2_GIC_ITS_STRUCTURE*)((UINT8*)Madt + GicItsOffset),
      GicItsInfo,
      GicItsCount
      );
  }

  return EFI_SUCCESS;

error_handler:
  if (*Table != NULL) {
    FreePool (*Table);
    *Table = NULL;
  }
  return Status;
}

/** Free any resources allocated for constructing the MADT

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
FreeMadtTableResources (
  IN      CONST ACPI_TABLE_GENERATOR                  * CONST This,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO            * CONST AcpiTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  * CONST CfgMgrProtocol,
  IN OUT        EFI_ACPI_DESCRIPTION_HEADER          ** CONST Table
  )
{
  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  if ((Table == NULL) || (*Table == NULL)) {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: Invalid Table Pointer\n"));
    ASSERT ((Table != NULL) && (*Table != NULL));
    return EFI_INVALID_PARAMETER;
  }

  FreePool (*Table);
  *Table = NULL;
  return EFI_SUCCESS;
}

/** The MADT Table Generator revision.
*/
#define MADT_GENERATOR_REVISION CREATE_REVISION (1, 0)

/** The interface for the MADT Table Generator.
*/
STATIC
CONST
ACPI_TABLE_GENERATOR MadtGenerator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdMadt),
  // Generator Description
  L"ACPI.STD.MADT.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_6_2_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE,
  // ACPI Table Revision supported by this Generator
  EFI_ACPI_6_2_MULTIPLE_APIC_DESCRIPTION_TABLE_REVISION,
  // Minimum supported ACPI Table Revision
  EFI_ACPI_6_2_MULTIPLE_APIC_DESCRIPTION_TABLE_REVISION,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID_ARM,
  // Creator Revision
  MADT_GENERATOR_REVISION,
  // Build Table function
  BuildMadtTable,
  // Free Resource function
  FreeMadtTableResources,
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
AcpiMadtLibConstructor (
  IN CONST EFI_HANDLE                ImageHandle,
  IN       EFI_SYSTEM_TABLE  * CONST SystemTable
  )
{
  EFI_STATUS  Status;
  Status = RegisterAcpiTableGenerator (&MadtGenerator);
  DEBUG ((DEBUG_INFO, "MADT: Register Generator. Status = %r\n", Status));
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
AcpiMadtLibDestructor (
  IN CONST EFI_HANDLE                ImageHandle,
  IN       EFI_SYSTEM_TABLE  * CONST SystemTable
  )
{
  EFI_STATUS  Status;
  Status = DeregisterAcpiTableGenerator (&MadtGenerator);
  DEBUG ((DEBUG_INFO, "MADT: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
