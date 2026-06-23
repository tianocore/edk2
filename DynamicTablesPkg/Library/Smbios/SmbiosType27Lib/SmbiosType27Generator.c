/** @file
  SMBIOS Type27 Table Generator.

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

/** SMBIOS Type 27 Cooling Device Generator

Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - EArchCommonObjCoolingDeviceInfo
*/

/**
  This macro expands to a function that retrieves the Cooling Device
  information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjCoolingDeviceInfo,
  CM_ARCH_COMMON_COOLING_DEVICE_INFO
  );

/**
  Type 27 records currently expose one string field: Description.
*/
#define SMBIOS_TYPE27_MAX_STRINGS  (1)

/**
  Valid SMBIOS Type 27 Status values are 0x01-0x06.
  Valid Device Type values are 0x01-0x09 and 0x10-0x11;
  values 0x0A-0x0F are reserved by the SMBIOS specification.
*/
#define SMBIOS_TYPE27_COOLING_STATUS_MIN           1
#define SMBIOS_TYPE27_COOLING_STATUS_MAX           6
#define SMBIOS_TYPE27_COOLING_DEVICE_TYPE_MIN      1
#define SMBIOS_TYPE27_COOLING_DEVICE_TYPE_MAX      17
#define SMBIOS_TYPE27_COOLING_DEVICE_TYPE_GAP_MIN  10
#define SMBIOS_TYPE27_COOLING_DEVICE_TYPE_GAP_MAX  15

/** Check whether a Type 27 cooling device status value is valid.

  @param [in] Status  Cooling device status value.

  @retval TRUE   The status value is valid.
  @retval FALSE  The status value is invalid.
**/
STATIC
BOOLEAN
IsValidCoolingDeviceStatus (
  IN UINT8  Status
  )
{
  return (Status >= SMBIOS_TYPE27_COOLING_STATUS_MIN) &&
         (Status <= SMBIOS_TYPE27_COOLING_STATUS_MAX);
}

/** Check whether a Type 27 cooling device type value is valid.

  @param [in] Type  Cooling device type value.

  @retval TRUE   The type value is valid.
  @retval FALSE  The type value is invalid.
**/
STATIC
BOOLEAN
IsValidCoolingDeviceType (
  IN UINT8  Type
  )
{
  return ((Type >= SMBIOS_TYPE27_COOLING_DEVICE_TYPE_MIN) &&
          (Type < SMBIOS_TYPE27_COOLING_DEVICE_TYPE_GAP_MIN)) ||
         ((Type > SMBIOS_TYPE27_COOLING_DEVICE_TYPE_GAP_MAX) &&
          (Type <= SMBIOS_TYPE27_COOLING_DEVICE_TYPE_MAX));
}

/**
  Free any resources allocated when installing SMBIOS Type 27 table.

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
FreeSmbiosType27TableEx (
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

/** Construct SMBIOS Type 27 Table describing cooling devices.

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
BuildSmbiosType27TableEx (
  IN  CONST SMBIOS_TABLE_GENERATOR                         *This,
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL   *CONST  TableFactoryProtocol,
  IN        CM_STD_OBJ_SMBIOS_TABLE_INFO           *CONST  SmbiosTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  OUT       SMBIOS_STRUCTURE                               ***Table,
  OUT       CM_OBJECT_TOKEN                                **CmObjectToken,
  OUT       UINTN                                  *CONST  TableCount
  )
{
  EFI_STATUS                          Status;
  UINTN                               Index;
  UINTN                               SmbiosRecordSize;
  UINT32                              CoolingDevCount;
  UINT8                               Description;
  STRING_TABLE                        StrTable;
  SMBIOS_STRUCTURE                    **TableList;
  SMBIOS_TABLE_TYPE27                 *SmbiosRecord;
  CM_OBJECT_TOKEN                     *CmObjectList;
  CM_ARCH_COMMON_COOLING_DEVICE_INFO  *CoolingDevInfo;

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
    DEBUG ((DEBUG_ERROR, "%a:Invalid Parameter\n ", __func__));
    return EFI_INVALID_PARAMETER;
  }

  *Table         = NULL;
  *CmObjectToken = NULL;
  *TableCount    = 0;
  TableList      = NULL;
  SmbiosRecord   = NULL;
  CmObjectList   = NULL;

  Status = GetEArchCommonObjCoolingDeviceInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &CoolingDevInfo,
             &CoolingDevCount
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get cooling device info. Status = %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  if (CoolingDevCount == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: No Cooling Device CM Objects found\n",
      __func__
      ));
    return EFI_NOT_FOUND;
  }

  TableList = (SMBIOS_STRUCTURE **)AllocateZeroPool (sizeof (SMBIOS_STRUCTURE *) * CoolingDevCount);
  if (TableList == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to alloc memory for %u cooling devices table\n",
      __func__,
      CoolingDevCount
      ));
    Status = EFI_OUT_OF_RESOURCES;
    goto exitErrorBuildSmbiosType27Table;
  }

  CmObjectList = AllocateZeroPool (
                   sizeof (CM_OBJECT_TOKEN) * CoolingDevCount
                   );
  if (CmObjectList == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to alloc memory for %u cooling devices table\n",
      __func__,
      CoolingDevCount
      ));
    Status = EFI_OUT_OF_RESOURCES;
    goto exitErrorBuildSmbiosType27Table;
  }

  Index = 0;
  for (Index = 0; Index < CoolingDevCount; Index++) {
    Status = StringTableInitialize (&StrTable, SMBIOS_TYPE27_MAX_STRINGS);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to initialize string table for CoolingDevInfo[%u]. Status = %r\n",
        __func__,
        Index,
        Status
        ));
      goto exitErrorBuildSmbiosType27Table;
    }

    Description = 0;
    if (CoolingDevInfo[Index].Description[0]) {
      Status = StringTableAddString (&StrTable, CoolingDevInfo[Index].Description, &Description);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Failed to add Description String %r\n", Status));
        ASSERT (!EFI_ERROR (Status));
        StringTableFree (&StrTable);
        goto exitErrorBuildSmbiosType27Table;
      }
    }

    SmbiosRecordSize = sizeof (SMBIOS_TABLE_TYPE27) + StringTableGetStringSetSize (&StrTable);
    SmbiosRecord     = (SMBIOS_TABLE_TYPE27 *)AllocateZeroPool (SmbiosRecordSize);
    if (SmbiosRecord == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      StringTableFree (&StrTable);
      goto exitErrorBuildSmbiosType27Table;
    }

    // Set up the header
    SmbiosRecord->Hdr.Type   = EFI_SMBIOS_TYPE_COOLING_DEVICE;
    SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE27);

    // Type 28 is not supported yet. Per the SMBIOS general handle rule,
    // use 0xFFFF when the referenced handle is not applicable or does not exist.
    // Todo: Once Type 28 generator is implemented this has to be changed to handle it
    if (CoolingDevInfo[Index].TemperatureProbeToken != CM_NULL_TOKEN) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: TemperatureProbeToken is unsupported until SMBIOS Type 28 generation is available for CoolingDevInfo[%u]\n",
        __func__,
        Index
        ));
      Status = EFI_UNSUPPORTED;
      StringTableFree (&StrTable);
      goto exitErrorBuildSmbiosType27Table;
    }

    SmbiosRecord->TemperatureProbeHandle = 0xFFFF;

    if (!IsValidCoolingDeviceStatus (
           CoolingDevInfo[Index].DeviceTypeAndStatus.CoolingDeviceStatus
           ))
    {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Invalid Cooling Device Status 0x%x for CoolingDevInfo[%u]\n",
        __func__,
        CoolingDevInfo[Index].DeviceTypeAndStatus.CoolingDeviceStatus,
        Index
        ));
      Status = EFI_INVALID_PARAMETER;
      StringTableFree (&StrTable);
      goto exitErrorBuildSmbiosType27Table;
    }

    if (!IsValidCoolingDeviceType (
           CoolingDevInfo[Index].DeviceTypeAndStatus.CoolingDevice
           ))
    {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Invalid Cooling Device Type 0x%x for CoolingDevInfo[%u]\n",
        __func__,
        CoolingDevInfo[Index].DeviceTypeAndStatus.CoolingDevice,
        Index
        ));
      Status = EFI_INVALID_PARAMETER;
      StringTableFree (&StrTable);
      goto exitErrorBuildSmbiosType27Table;
    }

    SmbiosRecord->DeviceTypeAndStatus = CoolingDevInfo[Index].DeviceTypeAndStatus;
    SmbiosRecord->CoolingUnitGroup    = CoolingDevInfo[Index].CoolingUnitGroup;
    SmbiosRecord->OEMDefined          = CoolingDevInfo[Index].OEMDefined;
    SmbiosRecord->NominalSpeed        = CoolingDevInfo[Index].NominalSpeed;
    SmbiosRecord->Description         = Description;

    Status = StringTablePublishStringSet (
               &StrTable,
               (CHAR8 *)(SmbiosRecord + 1),
               SmbiosRecordSize - sizeof (SMBIOS_TABLE_TYPE27)
               );

    if (EFI_ERROR (Status)) {
      StringTableFree (&StrTable);
      goto exitErrorBuildSmbiosType27Table;
    }

    StringTableFree (&StrTable);

    TableList[Index]    = (SMBIOS_STRUCTURE *)SmbiosRecord;
    CmObjectList[Index] = CoolingDevInfo[Index].Token;
    SmbiosRecord        = NULL;
  }

  *Table         = TableList;
  *CmObjectToken = CmObjectList;
  *TableCount    = CoolingDevCount;

  return EFI_SUCCESS;

exitErrorBuildSmbiosType27Table:
  if (TableList != NULL) {
    for (Index = 0; Index < CoolingDevCount; Index++) {
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

/** The interface for the SMBIOS Type27 Table Generator.
*/
STATIC
CONST
SMBIOS_TABLE_GENERATOR  SmbiosType27Generator = {
  // Generator ID
  CREATE_STD_SMBIOS_TABLE_GEN_ID (EStdSmbiosTableIdType27),
  // Generator Description
  L"SMBIOS.TYPE27.GENERATOR",
  // SMBIOS Table Type
  EFI_SMBIOS_TYPE_COOLING_DEVICE,
  NULL,
  NULL,
  // Build table function.
  BuildSmbiosType27TableEx,
  // Free function.
  FreeSmbiosType27TableEx,
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
SmbiosType27LibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterSmbiosTableGenerator (&SmbiosType27Generator);
  DEBUG ((
    DEBUG_INFO,
    "SMBIOS Type 27: Register Generator. Status = %r\n",
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
SmbiosType27LibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterSmbiosTableGenerator (&SmbiosType27Generator);
  DEBUG ((
    DEBUG_INFO,
    "SMBIOS Type 27: Deregister Generator. Status = %r\n",
    Status
    ));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
