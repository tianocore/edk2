/** @file
  SMBIOS Type26 Table Generator.

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

/** SMBIOS Type 26 Voltage Probe Generator

Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - EArchCommonObjVoltageProbeInfo
*/

/**
  This macro expands to a function that retrieves the Voltage Probe
  information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjVoltageProbeInfo,
  CM_ARCH_COMMON_VOLTAGE_PROBE_INFO
  );

/**
  Type 26 records expose one optional string field: Description.
*/
#define SMBIOS_TYPE26_MAX_STRINGS  (1)

/**
  Valid SMBIOS Type 26 Status values are 0x01-0x06.
  Valid Location values are 0x01-0x0B.
*/
#define SMBIOS_TYPE26_PROBE_STATUS_MIN    1
#define SMBIOS_TYPE26_PROBE_STATUS_MAX    6
#define SMBIOS_TYPE26_PROBE_LOCATION_MIN  1
#define SMBIOS_TYPE26_PROBE_LOCATION_MAX  11

/** Check whether a Type 26 voltage probe status value is valid.

  @param [in] Status  Voltage probe status value.

  @retval TRUE   The status value is valid.
  @retval FALSE  The status value is invalid.
**/
STATIC
BOOLEAN
IsValidVoltageProbeStatus (
  IN UINT8  Status
  )
{
  return (Status >= SMBIOS_TYPE26_PROBE_STATUS_MIN) &&
         (Status <= SMBIOS_TYPE26_PROBE_STATUS_MAX);
}

/** Check whether a Type 26 voltage probe location value is valid.

  @param [in] Location  Voltage probe location value.

  @retval TRUE   The location value is valid.
  @retval FALSE  The location value is invalid.
**/
STATIC
BOOLEAN
IsValidVoltageProbeLocation (
  IN UINT8  Location
  )
{
  return (Location >= SMBIOS_TYPE26_PROBE_LOCATION_MIN) &&
         (Location <= SMBIOS_TYPE26_PROBE_LOCATION_MAX);
}

/**
  Free any resources allocated when installing SMBIOS Type 26 table.

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
FreeSmbiosType26TableEx (
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

/** Construct SMBIOS Type 26 Table describing voltage probes.

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
BuildSmbiosType26TableEx (
  IN  CONST SMBIOS_TABLE_GENERATOR                         *This,
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL   *CONST  TableFactoryProtocol,
  IN        CM_STD_OBJ_SMBIOS_TABLE_INFO           *CONST  SmbiosTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  OUT       SMBIOS_STRUCTURE                               ***Table,
  OUT       CM_OBJECT_TOKEN                                **CmObjectToken,
  OUT       UINTN                                  *CONST  TableCount
  )
{
  EFI_STATUS                         Status;
  UINTN                              Index;
  UINTN                              SmbiosRecordSize;
  UINT32                             VoltageProbeCount;
  UINT8                              Description;
  STRING_TABLE                       StrTable;
  SMBIOS_STRUCTURE                   **TableList;
  SMBIOS_TABLE_TYPE26                *SmbiosRecord;
  CM_OBJECT_TOKEN                    *CmObjectList;
  CM_ARCH_COMMON_VOLTAGE_PROBE_INFO  *VoltageProbeInfo;

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

  Status = GetEArchCommonObjVoltageProbeInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &VoltageProbeInfo,
             &VoltageProbeCount
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get voltage probe info. Status = %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  if (VoltageProbeCount == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: No Voltage Probe CM Objects found\n",
      __func__
      ));
    return EFI_NOT_FOUND;
  }

  TableList = (SMBIOS_STRUCTURE **)AllocateZeroPool (sizeof (SMBIOS_STRUCTURE *) * VoltageProbeCount);
  if (TableList == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to alloc memory for %u voltage probe table\n",
      __func__,
      VoltageProbeCount
      ));
    Status = EFI_OUT_OF_RESOURCES;
    goto exitErrorBuildSmbiosType26Table;
  }

  CmObjectList = AllocateZeroPool (
                   sizeof (CM_OBJECT_TOKEN) * VoltageProbeCount
                   );
  if (CmObjectList == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to alloc memory for %u voltage probe table\n",
      __func__,
      VoltageProbeCount
      ));
    Status = EFI_OUT_OF_RESOURCES;
    goto exitErrorBuildSmbiosType26Table;
  }

  Index = 0;
  for (Index = 0; Index < VoltageProbeCount; Index++) {
    Status = StringTableInitialize (&StrTable, SMBIOS_TYPE26_MAX_STRINGS);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to initialize string table for VoltageProbeInfo[%u]. Status = %r\n",
        __func__,
        Index,
        Status
        ));
      goto exitErrorBuildSmbiosType26Table;
    }

    Description = 0;
    if (VoltageProbeInfo[Index].Description[0]) {
      Status = StringTableAddString (&StrTable, VoltageProbeInfo[Index].Description, &Description);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Failed to add Description String %r\n", Status));
        ASSERT (!EFI_ERROR (Status));
        StringTableFree (&StrTable);
        goto exitErrorBuildSmbiosType26Table;
      }
    }

    SmbiosRecordSize = sizeof (SMBIOS_TABLE_TYPE26) + StringTableGetStringSetSize (&StrTable);
    SmbiosRecord     = (SMBIOS_TABLE_TYPE26 *)AllocateZeroPool (SmbiosRecordSize);
    if (SmbiosRecord == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      StringTableFree (&StrTable);
      goto exitErrorBuildSmbiosType26Table;
    }

    // Set up the header
    SmbiosRecord->Hdr.Type   = EFI_SMBIOS_TYPE_VOLTAGE_PROBE;
    SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE26);

    if (!IsValidVoltageProbeStatus (
           VoltageProbeInfo[Index].LocationAndStatus.VoltageProbeStatus
           ))
    {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Invalid Voltage Probe Status 0x%x for VoltageProbeInfo[%u]\n",
        __func__,
        VoltageProbeInfo[Index].LocationAndStatus.VoltageProbeStatus,
        Index
        ));
      Status = EFI_INVALID_PARAMETER;
      StringTableFree (&StrTable);
      goto exitErrorBuildSmbiosType26Table;
    }

    if (!IsValidVoltageProbeLocation (
           VoltageProbeInfo[Index].LocationAndStatus.VoltageProbeSite
           ))
    {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Invalid Voltage Probe Location 0x%x for VoltageProbeInfo[%u]\n",
        __func__,
        VoltageProbeInfo[Index].LocationAndStatus.VoltageProbeSite,
        Index
        ));
      Status = EFI_INVALID_PARAMETER;
      StringTableFree (&StrTable);
      goto exitErrorBuildSmbiosType26Table;
    }

    SmbiosRecord->Description       = Description;
    SmbiosRecord->LocationAndStatus = VoltageProbeInfo[Index].LocationAndStatus;
    SmbiosRecord->MaximumValue      = VoltageProbeInfo[Index].MaximumValue;
    SmbiosRecord->MinimumValue      = VoltageProbeInfo[Index].MinimumValue;
    SmbiosRecord->Resolution        = VoltageProbeInfo[Index].Resolution;
    SmbiosRecord->Tolerance         = VoltageProbeInfo[Index].Tolerance;
    SmbiosRecord->Accuracy          = VoltageProbeInfo[Index].Accuracy;
    SmbiosRecord->OEMDefined        = VoltageProbeInfo[Index].OEMDefined;
    SmbiosRecord->NominalValue      = VoltageProbeInfo[Index].NominalValue;

    Status = StringTablePublishStringSet (
               &StrTable,
               (CHAR8 *)(SmbiosRecord + 1),
               SmbiosRecordSize - sizeof (SMBIOS_TABLE_TYPE26)
               );
    if (EFI_ERROR (Status)) {
      StringTableFree (&StrTable);
      goto exitErrorBuildSmbiosType26Table;
    }

    StringTableFree (&StrTable);

    TableList[Index]    = (SMBIOS_STRUCTURE *)SmbiosRecord;
    CmObjectList[Index] = VoltageProbeInfo[Index].VoltageProbeToken;
    SmbiosRecord        = NULL;
  }

  *Table         = TableList;
  *CmObjectToken = CmObjectList;
  *TableCount    = VoltageProbeCount;

  return EFI_SUCCESS;
exitErrorBuildSmbiosType26Table:
  if (TableList != NULL) {
    for (Index = 0; Index < VoltageProbeCount; Index++) {
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

/** The interface for the SMBIOS Type26 Table Generator.
*/
STATIC
CONST
SMBIOS_TABLE_GENERATOR  SmbiosType26Generator = {
  // Generator ID
  CREATE_STD_SMBIOS_TABLE_GEN_ID (EStdSmbiosTableIdType26),
  // Generator Description
  L"SMBIOS.TYPE26.GENERATOR",
  // SMBIOS Table Type
  EFI_SMBIOS_TYPE_VOLTAGE_PROBE,
  NULL,
  NULL,
  // Build table function.
  BuildSmbiosType26TableEx,
  // Free function.
  FreeSmbiosType26TableEx,
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
SmbiosType26LibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterSmbiosTableGenerator (&SmbiosType26Generator);
  DEBUG ((
    DEBUG_INFO,
    "SMBIOS Type 26: Register Generator. Status = %r\n",
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
SmbiosType26LibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterSmbiosTableGenerator (&SmbiosType26Generator);
  DEBUG ((
    DEBUG_INFO,
    "SMBIOS Type 26: Deregister Generator. Status = %r\n",
    Status
    ));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
