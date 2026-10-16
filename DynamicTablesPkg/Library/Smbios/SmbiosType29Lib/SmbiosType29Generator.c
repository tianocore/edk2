/** @file
  SMBIOS Type29 Table Generator.

  @par Reference(s):
  - SMBIOS Specification 3.9.0

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SmbiosStringTableLib.h>

// Module specific include files.
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <Protocol/DynamicTableFactoryProtocol.h>
#include <IndustryStandard/SmBios.h>

/** SMBIOS Type 29 Electrical Current Probe Generator

Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - EArchCommonObjElectricalCurrentProbeInfo
*/

/**
  This macro expands to a function that retrieves the Electrical Current Probe
  information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjElectricalCurrentProbeInfo,
  CM_ARCH_COMMON_ELECTRICAL_CURRENT_PROBE_INFO
  );

/**
  Type 29 records expose one optional string field: Description.
*/
#define SMBIOS_TYPE29_MAX_STRINGS  (1)

/**
  Valid SMBIOS Type 29 Status values are 0x01-0x06.
  Valid Location values are 0x01-0x0B.
*/
#define SMBIOS_TYPE29_PROBE_STATUS_MIN    1
#define SMBIOS_TYPE29_PROBE_STATUS_MAX    6
#define SMBIOS_TYPE29_PROBE_LOCATION_MIN  1
#define SMBIOS_TYPE29_PROBE_LOCATION_MAX  11

/** Check whether a Type 29 electrical current probe status value is valid.

  @param [in] Status  Electrical Current probe status value.

  @retval TRUE   The status value is valid.
  @retval FALSE  The status value is invalid.
**/
STATIC
BOOLEAN
IsValidElectricalCurrentProbeStatus (
  IN UINT8  Status
  )
{
  return (Status >= SMBIOS_TYPE29_PROBE_STATUS_MIN) &&
         (Status <= SMBIOS_TYPE29_PROBE_STATUS_MAX);
}

/** Check whether a Type 29 electrical current probe location value is valid.

  @param [in] Location  Electrical Current probe location value.

  @retval TRUE   The location value is valid.
  @retval FALSE  The location value is invalid.
**/
STATIC
BOOLEAN
IsValidElectricalCurrentProbeLocation (
  IN UINT8  Location
  )
{
  return (Location >= SMBIOS_TYPE29_PROBE_LOCATION_MIN) &&
         (Location <= SMBIOS_TYPE29_PROBE_LOCATION_MAX);
}

/**
  Free any resources allocated when installing SMBIOS Type 29 table.

  @param [in]  This                 Pointer to the SMBIOS table generator.
  @param [in]  TableFactoryProtocol Pointer to the SMBIOS Table Factory
                                    Protocol interface.

  @param [in]  SmbiosTableInfo      Pointer to the SMBIOS table information.
  @param [in]  CfgMgrProtocol       Pointer to the Configuration Manager
                                    Protocol interface.
  @param [in] Table                 Pointer to the SMBIOS table.
  @param [in] CmObjectToken         Pointer to the CM ObjectToken Array.
  @param [in] TableCount            Number of SMBIOS tables.

  @retval EFI_SUCCESS            Resources were freed successfully.
  @retval EFI_BAD_BUFFER_SIZE    The size returned by the Configuration
                                 Manager is less than the Object size for
                                 the requested object.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_NOT_FOUND          Could not find information.
  @retval EFI_OUT_OF_RESOURCES   Could not allocate memory.
  @retval EFI_UNSUPPORTED        Unsupported configuration.
**/
STATIC
EFI_STATUS
EFIAPI
FreeSmbiosType29TableEx (
  IN      CONST SMBIOS_TABLE_GENERATOR                    *CONST   This,
  IN      CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL      *CONST   TableFactoryProtocol,
  IN      CONST CM_STD_OBJ_SMBIOS_TABLE_INFO              *CONST   SmbiosTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL      *CONST   CfgMgrProtocol,
  IN      SMBIOS_STRUCTURE                               ***CONST  Table,
  IN      CM_OBJECT_TOKEN                                          **CmObjectToken,
  IN      CONST UINTN                                              TableCount
  )
{
  UINTN             Index;
  SMBIOS_STRUCTURE  **TableList;

  TableList = *Table;
  for (Index = 0; Index < TableCount; Index++) {
    if (TableList[Index] != NULL) {
      FreePool (TableList[Index]);
    }
  }

  if (*CmObjectToken != NULL) {
    FreePool (*CmObjectToken);
  }

  if (TableList != NULL) {
    FreePool (TableList);
  }

  return EFI_SUCCESS;
}

/** Construct SMBIOS Type 29 Table describing electrical current probes.

  If this function allocates any resources then they must be freed
  in the FreeXXXXTableResources function.

  @param [in]  This                 Pointer to the SMBIOS table generator.
  @param [in]  TableFactoryProtocol Pointer to the SMBIOS Table Factory
                                    Protocol interface.
  @param [in]  SmbiosTableInfo      Pointer to the SMBIOS table information.
  @param [in]  CfgMgrProtocol       Pointer to the Configuration Manager
                                    Protocol interface.
  @param [out] Table                Pointer to the SMBIOS table.
  @param [out] CmObjectToken        Pointer to the CM Object Token Array.
  @param [out] TableCount           Number of tables installed.

  @retval EFI_SUCCESS            Table generated successfully.
  @retval EFI_BAD_BUFFER_SIZE    The size returned by the Configuration
                                 Manager is less than the Object size for
                                 the requested object.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_NOT_FOUND          Could not find information.
  @retval EFI_OUT_OF_RESOURCES   Could not allocate memory.
  @retval EFI_UNSUPPORTED        Unsupported configuration.
**/
STATIC
EFI_STATUS
EFIAPI
BuildSmbiosType29TableEx (
  IN  CONST SMBIOS_TABLE_GENERATOR                         *This,
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL   *CONST  TableFactoryProtocol,
  IN        CM_STD_OBJ_SMBIOS_TABLE_INFO           *CONST  SmbiosTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  OUT       SMBIOS_STRUCTURE                               ***Table,
  OUT       CM_OBJECT_TOKEN                                **CmObjectToken,
  OUT       UINTN                                  *CONST  TableCount
  )
{
  EFI_STATUS                                    Status;
  UINTN                                         Index;
  UINTN                                         SmbiosRecordSize;
  UINT32                                        ElectricalCurrentProbeCount;
  UINT8                                         Description;
  STRING_TABLE                                  StrTable;
  SMBIOS_STRUCTURE                              **TableList;
  SMBIOS_TABLE_TYPE29                           *SmbiosRecord;
  CM_OBJECT_TOKEN                               *CmObjectList;
  CM_ARCH_COMMON_ELECTRICAL_CURRENT_PROBE_INFO  *ElectricalCurrentProbeInfo;

  ASSERT (This != NULL);
  ASSERT (SmbiosTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (CmObjectToken != NULL);
  ASSERT (Table != NULL);
  ASSERT (TableCount != NULL);
  ASSERT (SmbiosTableInfo->TableGeneratorId == This->GeneratorID);

  if ((This == NULL) || (SmbiosTableInfo == NULL) || (CfgMgrProtocol == NULL) ||
      (Table == NULL) || (CmObjectToken == NULL) || (TableCount == NULL) ||
      (SmbiosTableInfo->TableGeneratorId != This->GeneratorID))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Parameter\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  *Table         = NULL;
  *CmObjectToken = NULL;
  *TableCount    = 0;
  TableList      = NULL;
  SmbiosRecord   = NULL;
  CmObjectList   = NULL;

  Status = GetEArchCommonObjElectricalCurrentProbeInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &ElectricalCurrentProbeInfo,
             &ElectricalCurrentProbeCount
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get electrical current probe info. Status = %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  if (ElectricalCurrentProbeCount == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: No Electrical Current Probe CM Objects found\n",
      __func__
      ));
    return EFI_NOT_FOUND;
  }

  TableList = (SMBIOS_STRUCTURE **)AllocateZeroPool (sizeof (SMBIOS_STRUCTURE *) * ElectricalCurrentProbeCount);
  if (TableList == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to alloc memory for %u electrical current probe table\n",
      __func__,
      ElectricalCurrentProbeCount
      ));
    Status = EFI_OUT_OF_RESOURCES;
    goto exitErrorBuildSmbiosType29Table;
  }

  CmObjectList = AllocateZeroPool (
                   sizeof (CM_OBJECT_TOKEN) * ElectricalCurrentProbeCount
                   );
  if (CmObjectList == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to alloc memory for %u electrical current probe table\n",
      __func__,
      ElectricalCurrentProbeCount
      ));
    Status = EFI_OUT_OF_RESOURCES;
    goto exitErrorBuildSmbiosType29Table;
  }

  Index = 0;
  for (Index = 0; Index < ElectricalCurrentProbeCount; Index++) {
    Status = StringTableInitialize (&StrTable, SMBIOS_TYPE29_MAX_STRINGS);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to initialize string table for ElectricalCurrentProbeInfo[%u]. Status = %r\n",
        __func__,
        Index,
        Status
        ));
      goto exitErrorBuildSmbiosType29Table;
    }

    Description = 0;
    if (ElectricalCurrentProbeInfo[Index].Description[0]) {
      Status = StringTableAddString (&StrTable, ElectricalCurrentProbeInfo[Index].Description, &Description);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Failed to add Description String %r\n", Status));
        ASSERT (!EFI_ERROR (Status));
        StringTableFree (&StrTable);
        goto exitErrorBuildSmbiosType29Table;
      }
    }

    SmbiosRecordSize = sizeof (SMBIOS_TABLE_TYPE29) + StringTableGetStringSetSize (&StrTable);
    SmbiosRecord     = (SMBIOS_TABLE_TYPE29 *)AllocateZeroPool (SmbiosRecordSize);
    if (SmbiosRecord == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      StringTableFree (&StrTable);
      goto exitErrorBuildSmbiosType29Table;
    }

    // Set up the header
    SmbiosRecord->Hdr.Type   = EFI_SMBIOS_TYPE_ELECTRICAL_CURRENT_PROBE;
    SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE29);

    if (!IsValidElectricalCurrentProbeStatus (
           ElectricalCurrentProbeInfo[Index].LocationAndStatus.ElectricalCurrentProbeStatus
           ))
    {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Invalid Electrical Current Probe Status 0x%x for ElectricalCurrentProbeInfo[%u]\n",
        __func__,
        ElectricalCurrentProbeInfo[Index].LocationAndStatus.ElectricalCurrentProbeStatus,
        Index
        ));
      Status = EFI_INVALID_PARAMETER;
      StringTableFree (&StrTable);
      goto exitErrorBuildSmbiosType29Table;
    }

    if (!IsValidElectricalCurrentProbeLocation (
           ElectricalCurrentProbeInfo[Index].LocationAndStatus.ElectricalCurrentProbeSite
           ))
    {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Invalid Electrical Current Probe Location 0x%x for ElectricalCurrentProbeInfo[%u]\n",
        __func__,
        ElectricalCurrentProbeInfo[Index].LocationAndStatus.ElectricalCurrentProbeSite,
        Index
        ));
      Status = EFI_INVALID_PARAMETER;
      StringTableFree (&StrTable);
      goto exitErrorBuildSmbiosType29Table;
    }

    SmbiosRecord->Description       = Description;
    SmbiosRecord->LocationAndStatus = ElectricalCurrentProbeInfo[Index].LocationAndStatus;
    SmbiosRecord->MaximumValue      = ElectricalCurrentProbeInfo[Index].MaximumValue;
    SmbiosRecord->MinimumValue      = ElectricalCurrentProbeInfo[Index].MinimumValue;
    SmbiosRecord->Resolution        = ElectricalCurrentProbeInfo[Index].Resolution;
    SmbiosRecord->Tolerance         = ElectricalCurrentProbeInfo[Index].Tolerance;
    SmbiosRecord->Accuracy          = ElectricalCurrentProbeInfo[Index].Accuracy;
    SmbiosRecord->OEMDefined        = ElectricalCurrentProbeInfo[Index].OEMDefined;
    SmbiosRecord->NominalValue      = ElectricalCurrentProbeInfo[Index].NominalValue;

    Status = StringTablePublishStringSet (
               &StrTable,
               (CHAR8 *)(SmbiosRecord + 1),
               SmbiosRecordSize - sizeof (SMBIOS_TABLE_TYPE29)
               );
    if (EFI_ERROR (Status)) {
      StringTableFree (&StrTable);
      goto exitErrorBuildSmbiosType29Table;
    }

    StringTableFree (&StrTable);

    TableList[Index]    = (SMBIOS_STRUCTURE *)SmbiosRecord;
    CmObjectList[Index] = ElectricalCurrentProbeInfo[Index].ElectricalCurrentProbeToken;
    SmbiosRecord        = NULL;
  }

  *Table         = TableList;
  *CmObjectToken = CmObjectList;
  *TableCount    = ElectricalCurrentProbeCount;

  return EFI_SUCCESS;
exitErrorBuildSmbiosType29Table:
  if (TableList != NULL) {
    for (Index = 0; Index < ElectricalCurrentProbeCount; Index++) {
      if (TableList[Index] != NULL) {
        FreePool (TableList[Index]);
      }
    }

    FreePool (TableList);
  }

  if (CmObjectList != NULL) {
    FreePool (CmObjectList);
  }

  if (SmbiosRecord != NULL) {
    FreePool (SmbiosRecord);
  }

  return Status;
}

/** The interface for the SMBIOS Type29 Table Generator.
*/
STATIC
CONST
SMBIOS_TABLE_GENERATOR  SmbiosType29Generator = {
  // Generator ID
  CREATE_STD_SMBIOS_TABLE_GEN_ID (EStdSmbiosTableIdType29),
  // Generator Description
  L"SMBIOS.TYPE29.GENERATOR",
  // SMBIOS Table Type
  EFI_SMBIOS_TYPE_ELECTRICAL_CURRENT_PROBE,
  NULL,
  NULL,
  // Build table function.
  BuildSmbiosType29TableEx,
  // Free function.
  FreeSmbiosType29TableEx,
};

/** Register the Generator with the SMBIOS Table Factory.

  @param [in]  ImageHandle  The handle to the image.
  @param [in]  SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS           The Generator is registered.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_ALREADY_STARTED   The Generator for the Table ID
                                is already registered.
**/
EFI_STATUS
EFIAPI
SmbiosType29LibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterSmbiosTableGenerator (&SmbiosType29Generator);
  DEBUG ((
    DEBUG_INFO,
    "SMBIOS Type 29: Register Generator. Status = %r\n",
    Status
    ));
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/** Deregister the Generator from the SMBIOS Table Factory.

  @param [in]  ImageHandle  The handle to the image.
  @param [in]  SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS           The Generator is deregistered.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The Generator is not registered.
**/
EFI_STATUS
EFIAPI
SmbiosType29LibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterSmbiosTableGenerator (&SmbiosType29Generator);
  DEBUG ((
    DEBUG_INFO,
    "SMBIOS Type 29: Deregister Generator. Status = %r\n",
    Status
    ));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
