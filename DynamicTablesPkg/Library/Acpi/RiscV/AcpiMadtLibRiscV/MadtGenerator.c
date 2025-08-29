/** @file
  MADT Table Generator for RISC-V

  Copyright (c) 2024, Ventana Micro Systems Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - TBD - ACPI 6.6 Specification

**/

#include <IndustryStandard/Acpi.h>
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

/** RISC-V standard MADT Generator

Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - ERiscVObjRintcInfo
  - ERiscVObjImsicInfo (OPTIONAL)
  - ERiscVObjAplicInfo (OPTIONAL)
  - ERiscVObjPlicInfo  (OPTIONAL)
*/

/** This macro expands to a function that retrieves the RINTC
    Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceRiscV,
  ERiscVObjRintcInfo,
  CM_RISCV_RINTC_INFO
  );

/** This macro expands to a function that retrieves the IMSIC
    Information from the Configuration Manager.
*/

GET_OBJECT_LIST (
  EObjNameSpaceRiscV,
  ERiscVObjImsicInfo,
  CM_RISCV_IMSIC_INFO
  );

/** This macro expands to a function that retrieves the APLIC
    Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceRiscV,
  ERiscVObjAplicInfo,
  CM_RISCV_APLIC_INFO
  );

/** This macro expands to a function that retrieves the PLIC
    Information from the Configuration Manager.
*/

GET_OBJECT_LIST (
  EObjNameSpaceRiscV,
  ERiscVObjPlicInfo,
  CM_RISCV_PLIC_INFO
  );

/** This function updates the RINTC Information in the
    EFI_ACPI_6_6_RINTC_STRUCTURE structure.

  @param [in]  Rintc       Pointer to RINTC structure.
  @param [in]  RintcInfo   Pointer to the RINTC Information.
**/
STATIC
VOID
AddRINTC (
  IN  EFI_ACPI_6_6_RINTC_STRUCTURE  *CONST  Rintc,
  IN  CONST CM_RISCV_RINTC_INFO     *CONST  RintcInfo
  )
{
  ASSERT (Rintc != NULL);
  ASSERT (RintcInfo != NULL);

  // UINT8 Type
  Rintc->Type = EFI_ACPI_6_6_RINTC;
  // UINT8 Length
  Rintc->Length = sizeof (EFI_ACPI_6_6_RINTC_STRUCTURE);
  // UINT8 Version
  Rintc->Version = EFI_ACPI_6_6_RINTC_STRUCTURE_VERSION;
  // UINT8 Reserved
  Rintc->Reserved = EFI_ACPI_RESERVED_BYTE;

  // UINT32 Flags
  Rintc->Flags = RintcInfo->Flags;
  // UINT64 HartID
  Rintc->HartId = RintcInfo->HartId;
  // UINT32 AcpiProcessorUid
  Rintc->Uid = RintcInfo->AcpiProcessorUid;
  // UINT32 ExtIntcId
  Rintc->ExtIntcId = RintcInfo->ExtIntcId;
  // UINT64 ImsicBaseAddress
  Rintc->ImsicAddr = RintcInfo->ImsicBaseAddress;
  // UINT32 ImsicSize
  Rintc->ImsicSize = RintcInfo->ImsicSize;
}

/**
  Function to test if two GIC CPU Interface information structures have the
  same ACPI Processor UID.

  @param [in]  RintcInfo1         Pointer to the first RINTC info structure.
  @param [in]  RintcInfo2         Pointer to the second RINTC info structure.
  @param [in]  Index1             Index of RintcInfo1 in the shared list of GIC
                                  CPU Interface Info structures.
  @param [in]  Index2             Index of RintcInfo2 in the shared list of GIC
                                  CPU Interface Info structures.

  @retval TRUE                    RintcInfo1 and GicCInfo2 have the same UID.
  @retval FALSE                   RintcInfo1 and GicCInfo2 have different UIDs.
**/
BOOLEAN
EFIAPI
IsAcpiUidEqual (
  IN  CONST VOID   *RintcInfo1,
  IN  CONST VOID   *RintcInfo2,
  IN        UINTN  Index1,
  IN        UINTN  Index2
  )
{
  UINT32  Uid1;
  UINT32  Uid2;

  ASSERT (RintcInfo1 != NULL);
  ASSERT (RintcInfo2 != NULL);

  Uid1 = ((CM_RISCV_RINTC_INFO *)RintcInfo1)->AcpiProcessorUid;
  Uid2 = ((CM_RISCV_RINTC_INFO *)RintcInfo2)->AcpiProcessorUid;

  if (Uid1 == Uid2) {
    DEBUG (
      (
       DEBUG_ERROR,
       "ERROR: MADT: RINTC Info Structures %d and %d have the same ACPI " \
       "Processor UID: 0x%x.\n",
       Index1,
       Index2,
       Uid1
      )
      );
    return TRUE;
  }

  return FALSE;
}

/** Add the RINTC Information to the MADT Table.

  This function also checks for duplicate ACPI Processor UIDs.

  @param [in]  Rintc                Pointer to RINTC structure list.
  @param [in]  RintcInfo            Pointer to the RINTC Information list.
  @param [in]  RintcCount           Count of RINTC.

  @retval EFI_SUCCESS               RINTC Information was added successfully.
  @retval EFI_INVALID_PARAMETER     One or more invalid RINTC Info values were
                                    provided and the generator failed to add the
                                    information to the table.
**/
STATIC
EFI_STATUS
AddRINTCList (
  IN  EFI_ACPI_6_6_RINTC_STRUCTURE  *Rintc,
  IN  CONST CM_RISCV_RINTC_INFO     *RintcInfo,
  IN  UINT32                        RintcCount
  )
{
  BOOLEAN  IsAcpiProcUidDuplicated;

  ASSERT (Rintc != NULL);
  ASSERT (RintcInfo != NULL);

  IsAcpiProcUidDuplicated = FindDuplicateValue (
                              RintcInfo,
                              RintcCount,
                              sizeof (CM_RISCV_RINTC_INFO),
                              IsAcpiUidEqual
                              );
  // Duplicate ACPI Processor UID was found so the RINTC info provided
  // is invalid
  if (IsAcpiProcUidDuplicated) {
    return EFI_INVALID_PARAMETER;
  }

  while (RintcCount-- != 0) {
    AddRINTC (Rintc++, RintcInfo++);
  }

  return EFI_SUCCESS;
}

/** Update the IMSIC Information in the MADT Table.

  @param [in]  Imsic      Pointer to IMSIC structure.
  @param [in]  ImsicInfo  Pointer to the IMSIC Information.
**/
STATIC
VOID
AddIMSIC (
  EFI_ACPI_6_6_IMSIC_STRUCTURE  *CONST  Imsic,
  CONST CM_RISCV_IMSIC_INFO     *CONST  ImsicInfo
  )
{
  ASSERT (Imsic != NULL);
  ASSERT (ImsicInfo != NULL);

  // UINT8 Type
  Imsic->Type = EFI_ACPI_6_6_IMSIC;
  // UINT8 Length
  Imsic->Length = sizeof (EFI_ACPI_6_6_IMSIC_STRUCTURE);
  // UINT8 Version
  Imsic->Version = EFI_ACPI_6_6_IMSIC_STRUCTURE_VERSION;
  // UINT8 Reserved
  Imsic->Reserved = EFI_ACPI_RESERVED_BYTE;

  // UINT32 Flags
  Imsic->Flags = ImsicInfo->Flags;
  // UINT16 NumIds
  Imsic->NumIds = ImsicInfo->NumIds;
  // UINT16 NumGuestIds
  Imsic->NumGuestIds = ImsicInfo->NumGuestIds;
  // UINT8 GuestIndexBits
  Imsic->GuestIndexBits = ImsicInfo->GuestIndexBits;
  // UINT8 GuestIndexBits
  Imsic->HartIndexBits = ImsicInfo->HartIndexBits;
  // UINT8 GroupIndexBits
  Imsic->GroupIndexBits = ImsicInfo->GroupIndexBits;
  // UINT8 GroupIndexShift
  Imsic->GroupIndexShift = ImsicInfo->GroupIndexShift;
}

/** Update the APLIC Information.

  @param [in]  Aplic                 Pointer to APLIC structure.
  @param [in]  AplicInfo             Pointer to the APLIC Info.
**/
STATIC
VOID
AddAPLIC (
  IN  EFI_ACPI_6_6_APLIC_STRUCTURE   *CONST  Aplic,
  IN  CONST CM_RISCV_APLIC_INFO  *CONST      AplicInfo
  )
{
  UINTN  Idx;

  ASSERT (Aplic != NULL);
  ASSERT (AplicInfo != NULL);

  Aplic->Type    = EFI_ACPI_6_6_APLIC;
  Aplic->Length  = sizeof (EFI_ACPI_6_6_APLIC_STRUCTURE);
  Aplic->Version = EFI_ACPI_6_6_APLIC_STRUCTURE_VERSION;
  Aplic->Id      = AplicInfo->PlicAplicCommonInfo.Id;
  Aplic->Flags   = AplicInfo->PlicAplicCommonInfo.Flags;
  for (Idx = 0; Idx < RISCV_HWID_LENGTH; Idx++) {
    Aplic->HwId[Idx] = AplicInfo->PlicAplicCommonInfo.HwId[Idx];
  }

  Aplic->NumIdcs    = AplicInfo->NumIdcs;
  Aplic->NumSources = AplicInfo->PlicAplicCommonInfo.NumSources;
  Aplic->GsiBase    = AplicInfo->PlicAplicCommonInfo.GsiBase;
  Aplic->BaseAddr   = AplicInfo->PlicAplicCommonInfo.BaseAddress;
  Aplic->Size       = AplicInfo->PlicAplicCommonInfo.Size;
}

/** Add the APLIC Information to the MADT Table.

  @param [in]  Aplic      Pointer to APLIC structure list.
  @param [in]  AplicInfo  Pointer to the APLIC info list.
  @param [in]  AplicCount Count of APLICs.
**/
STATIC
VOID
AddAPLICList (
  IN  EFI_ACPI_6_6_APLIC_STRUCTURE  *Aplic,
  IN  CONST CM_RISCV_APLIC_INFO     *AplicInfo,
  IN  UINT32                        AplicCount
  )
{
  ASSERT (Aplic != NULL);
  ASSERT (AplicInfo != NULL);

  while (AplicCount-- != 0) {
    AddAPLIC (Aplic++, AplicInfo++);
  }
}

/** Update the PLIC Information.

  @param [in]  Plic                 Pointer to PLIC structure.
  @param [in]  PlicInfo             Pointer to the PLIC Info.
**/
STATIC
VOID
AddPLIC (
  IN  EFI_ACPI_6_6_PLIC_STRUCTURE   *CONST  Plic,
  IN  CONST CM_RISCV_PLIC_INFO  *CONST      PlicInfo
  )
{
  UINTN  Idx;

  ASSERT (Plic != NULL);
  ASSERT (PlicInfo != NULL);

  Plic->Type    = EFI_ACPI_6_6_PLIC;
  Plic->Length  = sizeof (EFI_ACPI_6_6_PLIC_STRUCTURE);
  Plic->Version = EFI_ACPI_6_6_PLIC_STRUCTURE_VERSION;
  Plic->Id      = PlicInfo->PlicAplicCommonInfo.Id;
  for (Idx = 0; Idx < RISCV_HWID_LENGTH; Idx++) {
    Plic->HwId[Idx] = PlicInfo->PlicAplicCommonInfo.HwId[Idx];
  }

  Plic->NumIrqs  = PlicInfo->PlicAplicCommonInfo.NumSources;
  Plic->MaxPrio  = PlicInfo->MaxPriority;
  Plic->Flags    = PlicInfo->PlicAplicCommonInfo.Flags;
  Plic->Size     = PlicInfo->PlicAplicCommonInfo.Size;
  Plic->BaseAddr = PlicInfo->PlicAplicCommonInfo.BaseAddress;
  Plic->GsiBase  = PlicInfo->PlicAplicCommonInfo.GsiBase;
}

/** Add the PLIC Information to the MADT Table.

  @param [in]  Plic      Pointer to PLIC structure list.
  @param [in]  PlicInfo  Pointer to the PLIC info list.
  @param [in]  PlicCount Count of PLICs.
**/
STATIC
VOID
AddPLICList (
  IN  EFI_ACPI_6_6_PLIC_STRUCTURE  *Plic,
  IN  CONST CM_RISCV_PLIC_INFO     *PlicInfo,
  IN  UINT32                       PlicCount
  )
{
  ASSERT (Plic != NULL);
  ASSERT (PlicInfo != NULL);

  while (PlicCount-- != 0) {
    AddPLIC (Plic++, PlicInfo++);
  }
}

/** Construct the MADT ACPI table.

  This function invokes the Configuration Manager protocol interface
  to get the required hardware information for generating the ACPI
  table.

  If this function allocates any resources then they must be freed
  in the FreeXXXXTableResources function.

  @param [in]  This             Pointer to the table generator.
  @param [in]  AcpiTableInfo    Pointer to the ACPI Table Info.
  @param [in]  CfgMgrProtocol   Pointer to the Configuration Manager
                                Protocol Interface.
  @param [out] Table            Pointer to the constructed ACPI Table.

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
  IN  CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  CM_RISCV_RINTC_INFO  *RintcInfo;
  CM_RISCV_IMSIC_INFO  *ImsicInfo;
  CM_RISCV_APLIC_INFO  *AplicInfo;
  CM_RISCV_PLIC_INFO   *PlicInfo;
  EFI_STATUS           Status;
  UINT32               TableSize;
  UINT32               RintcCount;
  UINT32               ImsicCount;
  UINT32               AplicCount;
  UINT32               PlicCount;
  UINT32               RintcOffset;
  UINT32               ImsicOffset;
  UINT32               AplicOffset;
  UINT32               PlicOffset;

  EFI_ACPI_6_6_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER  *Madt;

  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Table != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  if ((AcpiTableInfo->AcpiTableRevision < This->MinAcpiTableRevision) ||
      (AcpiTableInfo->AcpiTableRevision > This->AcpiTableRevision))
  {
    DEBUG (
      (
       DEBUG_ERROR,
       "ERROR: MADT: Requested table revision = %d, is not supported."
       "Supported table revision: Minimum = %d, Maximum = %d\n",
       AcpiTableInfo->AcpiTableRevision,
       This->MinAcpiTableRevision,
       This->AcpiTableRevision
      )
      );
    return EFI_INVALID_PARAMETER;
  }

  *Table = NULL;

  Status = GetERiscVObjRintcInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &RintcInfo,
             &RintcCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG (
      (
       DEBUG_ERROR,
       "ERROR: MADT: Failed to get RINTC Info. Status = %r\n",
       Status
      )
      );
    goto error_handler;
  }

  if (RintcCount == 0) {
    DEBUG (
      (
       DEBUG_ERROR,
       "ERROR: MADT: RINTC information not provided.\n"
      )
      );
    ASSERT (RintcCount != 0);
    Status = EFI_INVALID_PARAMETER;
    goto error_handler;
  }

  Status = GetERiscVObjImsicInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &ImsicInfo,
             &ImsicCount
             );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG (
      (
       DEBUG_ERROR,
       "ERROR: MADT: Failed to get IMSIC Info. Status = %r\n",
       Status
      )
      );
    goto error_handler;
  }

  if (ImsicCount > 1) {
    DEBUG (
      (
       DEBUG_ERROR,
       "ERROR: MADT: One, and only one, IMSIC must be present."
       "GicDCount = %d\n",
       ImsicCount
      )
      );
    ASSERT (ImsicCount <= 1);
    Status = EFI_INVALID_PARAMETER;
    goto error_handler;
  }

  Status = GetERiscVObjAplicInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &AplicInfo,
             &AplicCount
             );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG (
      (
       DEBUG_ERROR,
       "ERROR: MADT: Failed to get APLIC Info. Status = %r\n",
       Status
      )
      );
    goto error_handler;
  }

  Status = GetERiscVObjPlicInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &PlicInfo,
             &PlicCount
             );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG (
      (
       DEBUG_ERROR,
       "ERROR: MADT: Failed to get PLIC Info. Status = %r\n",
       Status
      )
      );
    goto error_handler;
  }

  TableSize = sizeof (EFI_ACPI_6_6_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER);

  RintcOffset = TableSize;
  TableSize  += (sizeof (EFI_ACPI_6_6_RINTC_STRUCTURE) * RintcCount);

  ImsicOffset = TableSize;
  TableSize  += (sizeof (EFI_ACPI_6_6_IMSIC_STRUCTURE) * ImsicCount);

  AplicOffset = TableSize;
  TableSize  += (sizeof (EFI_ACPI_6_6_APLIC_STRUCTURE) * AplicCount);

  PlicOffset = TableSize;
  TableSize += (sizeof (EFI_ACPI_6_6_PLIC_STRUCTURE) * PlicCount);

  // Allocate the Buffer for MADT table
  *Table = (EFI_ACPI_DESCRIPTION_HEADER *)AllocateZeroPool (TableSize);
  if (*Table == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG (
      (
       DEBUG_ERROR,
       "ERROR: MADT: Failed to allocate memory for MADT Table, Size = %d," \
       " Status = %r\n",
       TableSize,
       Status
      )
      );
    goto error_handler;
  }

  Madt = (EFI_ACPI_6_6_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER *)*Table;

  DEBUG (
    (
     DEBUG_INFO,
     "MADT: Madt = 0x%p TableSize = 0x%x\n",
     Madt,
     TableSize
    )
    );

  Status = AddAcpiHeader (
             CfgMgrProtocol,
             This,
             &Madt->Header,
             AcpiTableInfo,
             TableSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG (
      (
       DEBUG_ERROR,
       "ERROR: MADT: Failed to add ACPI header. Status = %r\n",
       Status
      )
      );
    goto error_handler;
  }

  Status = AddRINTCList (
             (EFI_ACPI_6_6_RINTC_STRUCTURE *)((UINT8 *)Madt + RintcOffset),
             RintcInfo,
             RintcCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG (
      (
       DEBUG_ERROR,
       "ERROR: MADT: Failed to add RINTC structures. Status = %r\n",
       Status
      )
      );
    goto error_handler;
  }

  if (ImsicCount != 0) {
    AddIMSIC (
      (EFI_ACPI_6_6_IMSIC_STRUCTURE *)((UINT8 *)Madt + ImsicOffset),
      ImsicInfo
      );
  }

  if (AplicCount != 0) {
    AddAPLICList (
      (EFI_ACPI_6_6_APLIC_STRUCTURE *)((UINT8 *)Madt + AplicOffset),
      AplicInfo,
      AplicCount
      );
  }

  if (PlicCount != 0) {
    AddPLICList (
      (EFI_ACPI_6_6_PLIC_STRUCTURE *)((UINT8 *)Madt + PlicOffset),
      PlicInfo,
      PlicCount
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

  @retval EFI_SUCCESS             The resources were freed successfully.
  @retval EFI_INVALID_PARAMETER   The table pointer is NULL or invalid.
**/
STATIC
EFI_STATUS
FreeMadtTableResources (
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
#define MADT_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** The interface for the MADT Table Generator.
*/
STATIC
CONST
ACPI_TABLE_GENERATOR  MadtGenerator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdMadt),
  // Generator Description
  L"ACPI.STD.MADT.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_6_6_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE,
  // ACPI Table Revision supported by this Generator
  EFI_ACPI_6_6_MULTIPLE_APIC_DESCRIPTION_TABLE_REVISION,
  // Minimum supported ACPI Table Revision
  EFI_ACPI_6_6_MULTIPLE_APIC_DESCRIPTION_TABLE_REVISION,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID_RISCV,
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

  @param [in]  ImageHandle      The handle to the image.
  @param [in]  SystemTable      Pointer to the System Table.

  @retval EFI_SUCCESS           The Generator is registered.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_ALREADY_STARTED   The Generator for the Table ID
                                is already registered.
**/
EFI_STATUS
EFIAPI
AcpiMadtLibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&MadtGenerator);
  DEBUG ((DEBUG_INFO, "MADT: Register Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/** Deregister the Generator from the ACPI Table Factory.

  @param [in]  ImageHandle      The handle to the image.
  @param [in]  SystemTable      Pointer to the System Table.

  @retval EFI_SUCCESS           The Generator is deregistered.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The Generator is not registered.
**/
EFI_STATUS
EFIAPI
AcpiMadtLibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&MadtGenerator);
  DEBUG ((DEBUG_INFO, "MADT: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
