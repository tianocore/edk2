/** @file
  SMBIOS Type 39 System Power Supply Table Generator.

  Copyright (c) 2024 - 2026, NVIDIA CORPORATION & AFFILIATES. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SmbiosStringTableLib.h>

#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <Protocol/DynamicTableFactoryProtocol.h>
#include <IndustryStandard/SmBios.h>

/** SMBIOS Type 39 System Power Supply Generator

Requirements:
  The following Configuration Manager Object is required by this Generator:
  - EArchCommonObjSystemPowerSupplyInfo

  The following Configuration Manager Objects are required when their
  corresponding reference tokens are not CM_NULL_TOKEN:
  - EArchCommonObjVoltageProbeInfo
  - EArchCommonObjCoolingDeviceInfo
  - EArchCommonObjElectricalCurrentProbeInfo
*/

#define SMBIOS_TYPE39_MAX_STRINGS                7
#define SMBIOS_TYPE39_RESERVED_MASK              (BIT14 | BIT15)
#define SMBIOS_TYPE39_POWER_SUPPLY_TYPE_MASK     0x3C00
#define SMBIOS_TYPE39_POWER_SUPPLY_TYPE_SHIFT    10
#define SMBIOS_TYPE39_POWER_SUPPLY_TYPE_MIN      1
#define SMBIOS_TYPE39_POWER_SUPPLY_TYPE_MAX      8
#define SMBIOS_TYPE39_POWER_SUPPLY_STATUS_MASK   0x0380
#define SMBIOS_TYPE39_POWER_SUPPLY_STATUS_SHIFT  7
#define SMBIOS_TYPE39_POWER_SUPPLY_STATUS_MIN    1
#define SMBIOS_TYPE39_POWER_SUPPLY_STATUS_MAX    5
#define SMBIOS_TYPE39_VOLTAGE_SWITCH_MASK        0x0078
#define SMBIOS_TYPE39_VOLTAGE_SWITCH_SHIFT       3
#define SMBIOS_TYPE39_VOLTAGE_SWITCH_MIN         1
#define SMBIOS_TYPE39_VOLTAGE_SWITCH_MAX         6

GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjSystemPowerSupplyInfo,
  CM_ARCH_COMMON_SYSTEM_POWER_SUPPLY_INFO
  );

/** Validate a System Power Supply CM object.

  Validation follows SMBIOS Specification v3.9.0, Section 7.40.1.

  @param [in]  PowerSupplyInfo  System Power Supply Information to validate.

  @retval EFI_SUCCESS            The System Power Supply Information is valid.
  @retval EFI_INVALID_PARAMETER  The System Power Supply Information is invalid.
**/
STATIC
EFI_STATUS
ValidateSystemPowerSupplyInfo (
  IN CONST CM_ARCH_COMMON_SYSTEM_POWER_SUPPLY_INFO  *PowerSupplyInfo
  )
{
  UINT16  Characteristics;
  UINT16  PowerSupplyType;
  UINT16  PowerSupplyStatus;
  UINT16  VoltageSwitch;

  Characteristics = PowerSupplyInfo->PowerSupplyCharacteristics;
  PowerSupplyType = (Characteristics & SMBIOS_TYPE39_POWER_SUPPLY_TYPE_MASK) >>
                    SMBIOS_TYPE39_POWER_SUPPLY_TYPE_SHIFT;
  PowerSupplyStatus = (Characteristics & SMBIOS_TYPE39_POWER_SUPPLY_STATUS_MASK) >>
                      SMBIOS_TYPE39_POWER_SUPPLY_STATUS_SHIFT;
  VoltageSwitch = (Characteristics & SMBIOS_TYPE39_VOLTAGE_SWITCH_MASK) >>
                  SMBIOS_TYPE39_VOLTAGE_SWITCH_SHIFT;

  if ((PowerSupplyInfo->PowerSupplyInfoToken == CM_NULL_TOKEN) ||
      ((Characteristics & SMBIOS_TYPE39_RESERVED_MASK) != 0) ||
      (PowerSupplyType < SMBIOS_TYPE39_POWER_SUPPLY_TYPE_MIN) ||
      (PowerSupplyType > SMBIOS_TYPE39_POWER_SUPPLY_TYPE_MAX) ||
      (PowerSupplyStatus < SMBIOS_TYPE39_POWER_SUPPLY_STATUS_MIN) ||
      (PowerSupplyStatus > SMBIOS_TYPE39_POWER_SUPPLY_STATUS_MAX) ||
      (VoltageSwitch < SMBIOS_TYPE39_VOLTAGE_SWITCH_MIN) ||
      (VoltageSwitch > SMBIOS_TYPE39_VOLTAGE_SWITCH_MAX))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid System Power Supply Information\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/** Add an optional string to a string table.

  @param [in,out] StrTable   String table receiving the string.
  @param [in]     String     Null-terminated string to add.
  @param [out]    StringRef  SMBIOS string reference.

  @retval EFI_SUCCESS  The string was added or was empty.
  @return              Error status returned by StringTableAddString().
**/
STATIC
EFI_STATUS
AddOptionalString (
  IN OUT STRING_TABLE         *StrTable,
  IN     CONST CHAR8          *String,
  OUT    SMBIOS_TABLE_STRING  *StringRef
  )
{
  *StringRef = 0;
  if (String[0] == '\0') {
    return EFI_SUCCESS;
  }

  return StringTableAddString (StrTable, String, StringRef);
}

/** Resolve an optional CM object token to an SMBIOS handle.

  @param [in]  TableFactoryProtocol  Pointer to the SMBIOS table factory.
  @param [in]  GeneratorId           Generator ID for the referenced table.
  @param [in]  Token                 Referenced CM object token.
  @param [out] Handle                Resolved SMBIOS handle, or 0xFFFF when
                                     Token is CM_NULL_TOKEN.

  @retval EFI_SUCCESS    The token is null or was resolved.
  @retval EFI_NOT_FOUND  The non-null token could not be resolved.
**/
STATIC
EFI_STATUS
ResolveReferenceHandle (
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL  *TableFactoryProtocol,
  IN  SMBIOS_TABLE_GENERATOR_ID                   GeneratorId,
  IN  CM_OBJECT_TOKEN                             Token,
  OUT SMBIOS_HANDLE                               *Handle
  )
{
  *Handle = SMBIOS_HANDLE_INVALID;
  if (Token == CM_NULL_TOKEN) {
    return EFI_SUCCESS;
  }

  *Handle = TableFactoryProtocol->GetSmbiosHandleEx (GeneratorId, Token);
  if (*Handle == SMBIOS_HANDLE_INVALID) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/** Construct SMBIOS Type 39 tables describing system power supplies.

  @param [in]  This                  Pointer to the SMBIOS table generator.
  @param [in]  TableFactoryProtocol  Pointer to the SMBIOS table factory.
  @param [in]  SmbiosTableInfo       Pointer to the SMBIOS table information.
  @param [in]  CfgMgrProtocol        Pointer to the Configuration Manager.
  @param [out] Table                 Pointer to the generated SMBIOS tables.
  @param [out] CmObjectToken         Pointer to the CM object tokens.
  @param [out] TableCount            Number of generated SMBIOS tables.

  @retval EFI_SUCCESS            Tables generated successfully.
  @retval EFI_INVALID_PARAMETER  A parameter or CM object is invalid.
  @retval EFI_NOT_FOUND          A required object or handle was not found.
  @retval EFI_OUT_OF_RESOURCES   Could not allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
BuildSmbiosType39TableEx (
  IN  CONST SMBIOS_TABLE_GENERATOR                         *This,
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL   *CONST  TableFactoryProtocol,
  IN        CM_STD_OBJ_SMBIOS_TABLE_INFO           *CONST  SmbiosTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  OUT       SMBIOS_STRUCTURE                               ***Table,
  OUT       CM_OBJECT_TOKEN                                **CmObjectToken,
  OUT       UINTN                                  *CONST  TableCount
  )
{
  EFI_STATUS                               Status;
  CM_ARCH_COMMON_SYSTEM_POWER_SUPPLY_INFO  *PowerSupplyInfo;
  UINT32                                   PowerSupplyCount;
  SMBIOS_STRUCTURE                         **TableList;
  CM_OBJECT_TOKEN                          *CmObjectList;
  SMBIOS_TABLE_TYPE39                      *SmbiosRecord;
  STRING_TABLE                             StrTable;
  BOOLEAN                                  StringTableInitialized;
  SMBIOS_TABLE_STRING                      LocationRef;
  SMBIOS_TABLE_STRING                      DeviceNameRef;
  SMBIOS_TABLE_STRING                      ManufacturerRef;
  SMBIOS_TABLE_STRING                      SerialNumberRef;
  SMBIOS_TABLE_STRING                      AssetTagNumberRef;
  SMBIOS_TABLE_STRING                      ModelPartNumberRef;
  SMBIOS_TABLE_STRING                      RevisionLevelRef;
  SMBIOS_HANDLE                            ReferenceHandle;
  UINTN                                    Index;
  UINTN                                    CompareIndex;

  ASSERT (This != NULL);
  ASSERT (TableFactoryProtocol != NULL);
  ASSERT (SmbiosTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Table != NULL);
  ASSERT (CmObjectToken != NULL);
  ASSERT (TableCount != NULL);
  ASSERT (SmbiosTableInfo->TableGeneratorId == This->GeneratorID);

  if ((This == NULL) || (TableFactoryProtocol == NULL) ||
      (SmbiosTableInfo == NULL) || (CfgMgrProtocol == NULL) ||
      (Table == NULL) || (CmObjectToken == NULL) || (TableCount == NULL) ||
      (SmbiosTableInfo->TableGeneratorId != This->GeneratorID))
  {
    return EFI_INVALID_PARAMETER;
  }

  *Table                 = NULL;
  *CmObjectToken         = NULL;
  *TableCount            = 0;
  TableList              = NULL;
  CmObjectList           = NULL;
  SmbiosRecord           = NULL;
  StringTableInitialized = FALSE;

  Status = GetEArchCommonObjSystemPowerSupplyInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &PowerSupplyInfo,
             &PowerSupplyCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get System Power Supply objects: %r\n", __func__, Status));
    return Status;
  }

  if (PowerSupplyCount == 0) {
    return EFI_NOT_FOUND;
  }

  if ((PowerSupplyInfo == NULL) ||
      (PowerSupplyCount > (MAX_UINT32 / sizeof (*TableList))) ||
      (PowerSupplyCount > (MAX_UINT32 / sizeof (*CmObjectList))))
  {
    return EFI_INVALID_PARAMETER;
  }

  TableList = AllocateZeroPool (sizeof (*TableList) * PowerSupplyCount);
  if (TableList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CmObjectList = AllocateZeroPool (sizeof (*CmObjectList) * PowerSupplyCount);
  if (CmObjectList == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }

  for (Index = 0; Index < PowerSupplyCount; Index++) {
    Status = ValidateSystemPowerSupplyInfo (&PowerSupplyInfo[Index]);
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    if (PowerSupplyInfo[Index].PowerUnitGroup != 0) {
      for (CompareIndex = 0; CompareIndex < PowerSupplyCount; CompareIndex++) {
        if ((CompareIndex != Index) &&
            (PowerSupplyInfo[CompareIndex].PowerUnitGroup ==
             PowerSupplyInfo[Index].PowerUnitGroup))
        {
          break;
        }
      }

      if (CompareIndex == PowerSupplyCount) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Power Unit Group %u has no redundant peer\n",
          __func__,
          PowerSupplyInfo[Index].PowerUnitGroup
          ));
        Status = EFI_INVALID_PARAMETER;
        goto ErrorExit;
      }
    }

    Status = StringTableInitialize (&StrTable, SMBIOS_TYPE39_MAX_STRINGS);
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    StringTableInitialized = TRUE;
    Status                 = AddOptionalString (&StrTable, PowerSupplyInfo[Index].Location, &LocationRef);
    if (!EFI_ERROR (Status)) {
      Status = AddOptionalString (&StrTable, PowerSupplyInfo[Index].DeviceName, &DeviceNameRef);
    }

    if (!EFI_ERROR (Status)) {
      Status = AddOptionalString (&StrTable, PowerSupplyInfo[Index].Manufacturer, &ManufacturerRef);
    }

    if (!EFI_ERROR (Status)) {
      Status = AddOptionalString (&StrTable, PowerSupplyInfo[Index].SerialNumber, &SerialNumberRef);
    }

    if (!EFI_ERROR (Status)) {
      Status = AddOptionalString (&StrTable, PowerSupplyInfo[Index].AssetTagNumber, &AssetTagNumberRef);
    }

    if (!EFI_ERROR (Status)) {
      Status = AddOptionalString (&StrTable, PowerSupplyInfo[Index].ModelPartNumber, &ModelPartNumberRef);
    }

    if (!EFI_ERROR (Status)) {
      Status = AddOptionalString (&StrTable, PowerSupplyInfo[Index].RevisionLevel, &RevisionLevelRef);
    }

    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    SmbiosRecord = AllocateSmbiosRecord (sizeof (*SmbiosRecord), &StrTable);
    if (SmbiosRecord == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ErrorExit;
    }

    SmbiosRecord->Hdr.Type        = EFI_SMBIOS_TYPE_SYSTEM_POWER_SUPPLY;
    SmbiosRecord->Hdr.Length      = sizeof (*SmbiosRecord);
    SmbiosRecord->PowerUnitGroup  = PowerSupplyInfo[Index].PowerUnitGroup;
    SmbiosRecord->Location        = LocationRef;
    SmbiosRecord->DeviceName      = DeviceNameRef;
    SmbiosRecord->Manufacturer    = ManufacturerRef;
    SmbiosRecord->SerialNumber    = SerialNumberRef;
    SmbiosRecord->AssetTagNumber  = AssetTagNumberRef;
    SmbiosRecord->ModelPartNumber = ModelPartNumberRef;
    SmbiosRecord->RevisionLevel   = RevisionLevelRef;
    WriteUnaligned16 (&SmbiosRecord->MaxPowerCapacity, PowerSupplyInfo[Index].MaxPowerCapacity);
    CopyMem (
      &SmbiosRecord->PowerSupplyCharacteristics,
      &PowerSupplyInfo[Index].PowerSupplyCharacteristics,
      sizeof (SmbiosRecord->PowerSupplyCharacteristics)
      );

    Status = ResolveReferenceHandle (
               TableFactoryProtocol,
               CREATE_STD_SMBIOS_TABLE_GEN_ID (EStdSmbiosTableIdType26),
               PowerSupplyInfo[Index].InputVoltageProbeToken,
               &ReferenceHandle
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to resolve input voltage probe for power supply %u\n", __func__, Index));
      goto ErrorExit;
    }

    WriteUnaligned16 (&SmbiosRecord->InputVoltageProbeHandle, ReferenceHandle);

    Status = ResolveReferenceHandle (
               TableFactoryProtocol,
               CREATE_STD_SMBIOS_TABLE_GEN_ID (EStdSmbiosTableIdType27),
               PowerSupplyInfo[Index].CoolingDeviceToken,
               &ReferenceHandle
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to resolve cooling device for power supply %u\n", __func__, Index));
      goto ErrorExit;
    }

    WriteUnaligned16 (&SmbiosRecord->CoolingDeviceHandle, ReferenceHandle);

    Status = ResolveReferenceHandle (
               TableFactoryProtocol,
               CREATE_STD_SMBIOS_TABLE_GEN_ID (EStdSmbiosTableIdType29),
               PowerSupplyInfo[Index].InputCurrentProbeToken,
               &ReferenceHandle
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to resolve input current probe for power supply %u\n", __func__, Index));
      goto ErrorExit;
    }

    WriteUnaligned16 (&SmbiosRecord->InputCurrentProbeHandle, ReferenceHandle);

    Status = StringTablePublishStringSet (
               &StrTable,
               (CHAR8 *)(SmbiosRecord + 1),
               StringTableGetStringSetSize (&StrTable)
               );
    StringTableFree (&StrTable);
    StringTableInitialized = FALSE;
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    TableList[Index]    = (SMBIOS_STRUCTURE *)SmbiosRecord;
    CmObjectList[Index] = PowerSupplyInfo[Index].PowerSupplyInfoToken;
    SmbiosRecord        = NULL;
  }

  *Table         = TableList;
  *CmObjectToken = CmObjectList;
  *TableCount    = PowerSupplyCount;
  return EFI_SUCCESS;

ErrorExit:
  if (StringTableInitialized) {
    StringTableFree (&StrTable);
  }

  if (SmbiosRecord != NULL) {
    FreePool (SmbiosRecord);
  }

  if (TableList != NULL) {
    for (Index = 0; Index < PowerSupplyCount; Index++) {
      if (TableList[Index] != NULL) {
        FreePool (TableList[Index]);
      }
    }

    FreePool (TableList);
  }

  if (CmObjectList != NULL) {
    FreePool (CmObjectList);
  }

  return Status;
}

/** Free resources allocated when installing SMBIOS Type 39 tables.

  @param [in] This                  Pointer to the SMBIOS table generator.
  @param [in] TableFactoryProtocol  Pointer to the SMBIOS Table Factory.
  @param [in] SmbiosTableInfo       Pointer to the SMBIOS table information.
  @param [in] CfgMgrProtocol        Pointer to the Configuration Manager.
  @param [in] Table                 Pointer to the SMBIOS tables.
  @param [in] CmObjectToken         Pointer to the CM object token array.
  @param [in] TableCount            Number of SMBIOS tables.

  @retval EFI_SUCCESS            Resources freed successfully.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
FreeSmbiosType39TableEx (
  IN      CONST SMBIOS_TABLE_GENERATOR                   *CONST  This,
  IN      CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL     *CONST  TableFactoryProtocol,
  IN      CONST CM_STD_OBJ_SMBIOS_TABLE_INFO             *CONST  SmbiosTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL     *CONST  CfgMgrProtocol,
  IN      SMBIOS_STRUCTURE                             ***CONST  Table,
  IN      CM_OBJECT_TOKEN                                        **CmObjectToken,
  IN      CONST UINTN                                            TableCount
  )
{
  UINTN  Index;

  if ((This == NULL) || (TableFactoryProtocol == NULL) ||
      (SmbiosTableInfo == NULL) || (CfgMgrProtocol == NULL) ||
      (Table == NULL) || (CmObjectToken == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (*Table != NULL) {
    for (Index = 0; Index < TableCount; Index++) {
      if ((*Table)[Index] != NULL) {
        FreePool ((*Table)[Index]);
      }
    }

    FreePool (*Table);
    *Table = NULL;
  }

  if (*CmObjectToken != NULL) {
    FreePool (*CmObjectToken);
    *CmObjectToken = NULL;
  }

  return EFI_SUCCESS;
}

STATIC CONST SMBIOS_TABLE_GENERATOR  SmbiosType39Generator = {
  CREATE_STD_SMBIOS_TABLE_GEN_ID (EStdSmbiosTableIdType39),
  L"SMBIOS.TYPE39.GENERATOR",
  EFI_SMBIOS_TYPE_SYSTEM_POWER_SUPPLY,
  NULL,
  NULL,
  BuildSmbiosType39TableEx,
  FreeSmbiosType39TableEx
};

/** Register the Generator with the SMBIOS Table Factory.

  @param [in] ImageHandle  The handle to the image.
  @param [in] SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS            The Generator is registered.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_ALREADY_STARTED    The Generator is already registered.
**/
EFI_STATUS
EFIAPI
SmbiosType39LibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterSmbiosTableGenerator (&SmbiosType39Generator);
  DEBUG ((DEBUG_INFO, "SMBIOS Type 39: Register Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/** Deregister the Generator from the SMBIOS Table Factory.

  @param [in] ImageHandle  The handle to the image.
  @param [in] SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS            The Generator is deregistered.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_NOT_FOUND          The Generator is not registered.
**/
EFI_STATUS
EFIAPI
SmbiosType39LibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterSmbiosTableGenerator (&SmbiosType39Generator);
  DEBUG ((DEBUG_INFO, "SMBIOS Type 39: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
