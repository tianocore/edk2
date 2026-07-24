/** @file
  SMBIOS Type 2 Baseboard (or Module) Information Table Generator.

  Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SmbiosStringTableLib.h>

#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <Protocol/DynamicTableFactoryProtocol.h>
#include <IndustryStandard/SmBios.h>

/** SMBIOS Type 2 Baseboard (or Module) Information Generator

Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - EArchCommonObjBaseboardInfo
  - EArchCommonObjBaseboardContainedObject
    Required only when a baseboard provides a non-null
    ContainedObjectListToken.
*/

#define SMBIOS_TYPE2_MAX_STRINGS                 6
#define SMBIOS_TYPE2_FEATURE_FLAG_RESERVED_MASK  0xE0
#define SMBIOS_TYPE2_BASE_LENGTH                 OFFSET_OF (SMBIOS_TABLE_TYPE2, ContainedObjectHandles)

GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjBaseboardInfo,
  CM_ARCH_COMMON_BASEBOARD_INFO
  );

GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjBaseboardContainedObject,
  CM_ARCH_COMMON_BASEBOARD_CONTAINED_OBJECT
  );

/** Validate a Baseboard Information CM object.

  Validation follows SMBIOS Specification v3.9.0, Sections 7.3.1 and 7.3.2.

  @param [in]  BaseboardInfo  Baseboard Information to validate.

  @retval EFI_SUCCESS            The Baseboard Information is valid.
  @retval EFI_INVALID_PARAMETER  The Baseboard Information is invalid.
**/
STATIC
EFI_STATUS
ValidateBaseboardInfo (
  IN CONST CM_ARCH_COMMON_BASEBOARD_INFO  *BaseboardInfo
  )
{
  if ((BaseboardInfo->FeatureFlag & SMBIOS_TYPE2_FEATURE_FLAG_RESERVED_MASK) != 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: FeatureFlag reserved bits must be zero: 0x%x\n",
      __func__,
      BaseboardInfo->FeatureFlag
      ));
    return EFI_INVALID_PARAMETER;
  }

  if ((BaseboardInfo->BoardType < BaseBoardTypeUnknown) ||
      (BaseboardInfo->BoardType > BaseBoardTypeInterconnectBoard))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid BoardType 0x%x\n", __func__, BaseboardInfo->BoardType));
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/** Add an optional string to a string table.

  @param [in,out]  StrTable   String table receiving the string.
  @param [in]      String     Null-terminated string to add.
  @param [out]     StringRef  SMBIOS string reference.

  @retval EFI_SUCCESS  The string was added or was empty.
  @return             Error status returned by StringTableAddString().
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

/** Construct SMBIOS Type 2 tables describing Baseboards.

  @param [in]  This                  Pointer to the SMBIOS table generator.
  @param [in]  TableFactoryProtocol  Pointer to the SMBIOS table factory.
  @param [in]  SmbiosTableInfo       Pointer to the SMBIOS table information.
  @param [in]  CfgMgrProtocol        Pointer to the Configuration Manager.
  @param [out] Table                 Pointer to the generated SMBIOS tables.
  @param [out] CmObjectToken         Pointer to the CM object tokens.
  @param [out] TableCount            Number of generated SMBIOS tables.

  @retval EFI_SUCCESS            Tables generated successfully.
  @retval EFI_INVALID_PARAMETER  A parameter or CM object is invalid.
  @retval EFI_NOT_FOUND          No Baseboard Information objects were found.
  @retval EFI_OUT_OF_RESOURCES   Could not allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
BuildSmbiosType2TableEx (
  IN  CONST SMBIOS_TABLE_GENERATOR                         *This,
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL   *CONST  TableFactoryProtocol,
  IN        CM_STD_OBJ_SMBIOS_TABLE_INFO           *CONST  SmbiosTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  OUT       SMBIOS_STRUCTURE                               ***Table,
  OUT       CM_OBJECT_TOKEN                                **CmObjectToken,
  OUT       UINTN                                  *CONST  TableCount
  )
{
  EFI_STATUS                                 Status;
  CM_ARCH_COMMON_BASEBOARD_INFO              *BaseboardInfo;
  CM_ARCH_COMMON_BASEBOARD_CONTAINED_OBJECT  *ContainedObject;
  UINT32                                     BaseboardCount;
  UINT32                                     ContainedObjectCount;
  SMBIOS_STRUCTURE                           **TableList;
  CM_OBJECT_TOKEN                            *CmObjectList;
  SMBIOS_TABLE_TYPE2                         *SmbiosRecord;
  STRING_TABLE                               StrTable;
  BOOLEAN                                    StringTableInitialized;
  SMBIOS_TABLE_STRING                        ManufacturerRef;
  SMBIOS_TABLE_STRING                        ProductNameRef;
  SMBIOS_TABLE_STRING                        VersionRef;
  SMBIOS_TABLE_STRING                        SerialNumRef;
  SMBIOS_TABLE_STRING                        AssetTagRef;
  SMBIOS_TABLE_STRING                        LocationInChassisRef;
  SMBIOS_HANDLE                              ReferenceHandle;
  UINTN                                      FormattedLength;
  UINTN                                      Index;
  UINTN                                      ContainedObjectIndex;

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
    DEBUG ((DEBUG_ERROR, "%a: Invalid Parameter\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  *Table                 = NULL;
  *CmObjectToken         = NULL;
  *TableCount            = 0;
  TableList              = NULL;
  CmObjectList           = NULL;
  SmbiosRecord           = NULL;
  StringTableInitialized = FALSE;

  Status = GetEArchCommonObjBaseboardInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &BaseboardInfo,
             &BaseboardCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get Baseboard objects: %r\n", __func__, Status));
    return Status;
  }

  if (BaseboardCount == 0) {
    return EFI_NOT_FOUND;
  }

  if ((BaseboardInfo == NULL) ||
      (BaseboardCount > (MAX_UINTN / sizeof (*TableList))) ||
      (BaseboardCount > (MAX_UINTN / sizeof (*CmObjectList))))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Baseboard object list\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  TableList = AllocateZeroPool (sizeof (*TableList) * BaseboardCount);
  if (TableList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CmObjectList = AllocateZeroPool (sizeof (*CmObjectList) * BaseboardCount);
  if (CmObjectList == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }

  for (Index = 0; Index < BaseboardCount; Index++) {
    Status = ValidateBaseboardInfo (&BaseboardInfo[Index]);
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    ContainedObject      = NULL;
    ContainedObjectCount = 0;
    if (BaseboardInfo[Index].ContainedObjectListToken != CM_NULL_TOKEN) {
      Status = GetEArchCommonObjBaseboardContainedObject (
                 CfgMgrProtocol,
                 BaseboardInfo[Index].ContainedObjectListToken,
                 &ContainedObject,
                 &ContainedObjectCount
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to get contained objects for Baseboard %u: %r\n",
          __func__,
          Index,
          Status
          ));
        goto ErrorExit;
      }

      if ((ContainedObjectCount != 0) && (ContainedObject == NULL)) {
        DEBUG ((DEBUG_ERROR, "%a: Contained object list is NULL for Baseboard %u\n", __func__, Index));
        Status = EFI_INVALID_PARAMETER;
        goto ErrorExit;
      }
    }

    if (ContainedObjectCount > MAX_UINT8) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Too many contained objects for Baseboard %u: %u\n",
        __func__,
        Index,
        ContainedObjectCount
        ));
      Status = EFI_INVALID_PARAMETER;
      goto ErrorExit;
    }

    FormattedLength = SMBIOS_TYPE2_BASE_LENGTH +
                      (ContainedObjectCount * sizeof (SMBIOS_HANDLE));
    if (FormattedLength > MAX_UINT8) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Formatted length %Lu exceeds Hdr.Length for Baseboard %u\n",
        __func__,
        (UINT64)FormattedLength,
        Index
        ));
      Status = EFI_INVALID_PARAMETER;
      goto ErrorExit;
    }

    Status = StringTableInitialize (&StrTable, SMBIOS_TYPE2_MAX_STRINGS);
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    StringTableInitialized = TRUE;
    Status                 = AddOptionalString (&StrTable, BaseboardInfo[Index].Manufacturer, &ManufacturerRef);
    if (!EFI_ERROR (Status)) {
      Status = AddOptionalString (&StrTable, BaseboardInfo[Index].ProductName, &ProductNameRef);
    }

    if (!EFI_ERROR (Status)) {
      Status = AddOptionalString (&StrTable, BaseboardInfo[Index].Version, &VersionRef);
    }

    if (!EFI_ERROR (Status)) {
      Status = AddOptionalString (&StrTable, BaseboardInfo[Index].SerialNum, &SerialNumRef);
    }

    if (!EFI_ERROR (Status)) {
      Status = AddOptionalString (&StrTable, BaseboardInfo[Index].AssetTag, &AssetTagRef);
    }

    if (!EFI_ERROR (Status)) {
      Status = AddOptionalString (
                 &StrTable,
                 BaseboardInfo[Index].LocationInChassis,
                 &LocationInChassisRef
                 );
    }

    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    SmbiosRecord = AllocateSmbiosRecord (FormattedLength, &StrTable);
    if (SmbiosRecord == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ErrorExit;
    }

    SmbiosRecord->Hdr.Type                       = EFI_SMBIOS_TYPE_BASEBOARD_INFORMATION;
    SmbiosRecord->Hdr.Length                     = (UINT8)FormattedLength;
    SmbiosRecord->Manufacturer                   = ManufacturerRef;
    SmbiosRecord->ProductName                    = ProductNameRef;
    SmbiosRecord->Version                        = VersionRef;
    SmbiosRecord->SerialNumber                   = SerialNumRef;
    SmbiosRecord->AssetTag                       = AssetTagRef;
    *(UINT8 *) &SmbiosRecord->FeatureFlag        = BaseboardInfo[Index].FeatureFlag;
    SmbiosRecord->LocationInChassis              = LocationInChassisRef;
    SmbiosRecord->BoardType                      = BaseboardInfo[Index].BoardType;
    SmbiosRecord->NumberOfContainedObjectHandles = (UINT8)ContainedObjectCount;

    ReferenceHandle = SMBIOS_HANDLE_INVALID;
    if (BaseboardInfo[Index].ChassisToken != CM_NULL_TOKEN) {
      ReferenceHandle = TableFactoryProtocol->GetSmbiosHandleEx (
                                                CREATE_STD_SMBIOS_TABLE_GEN_ID (EStdSmbiosTableIdType03),
                                                BaseboardInfo[Index].ChassisToken
                                                );
      if (ReferenceHandle == SMBIOS_HANDLE_INVALID) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to resolve ChassisToken 0x%p for Baseboard %u\n",
          __func__,
          BaseboardInfo[Index].ChassisToken,
          Index
          ));
        Status = EFI_NOT_FOUND;
        goto ErrorExit;
      }
    }

    WriteUnaligned16 (&SmbiosRecord->ChassisHandle, ReferenceHandle);

    if (ContainedObject != NULL) {
      for (ContainedObjectIndex = 0;
           ContainedObjectIndex < ContainedObjectCount;
           ContainedObjectIndex++)
      {
        ReferenceHandle = TableFactoryProtocol->GetSmbiosHandleEx (
                                                  ContainedObject[ContainedObjectIndex].GeneratorId,
                                                  ContainedObject[ContainedObjectIndex].ContainedObjectToken
                                                  );
        if (ReferenceHandle == SMBIOS_HANDLE_INVALID) {
          DEBUG ((
            DEBUG_ERROR,
            "%a: Failed to resolve contained object %u for Baseboard %u\n",
            __func__,
            ContainedObjectIndex,
            Index
            ));
          Status = EFI_NOT_FOUND;
          goto ErrorExit;
        }

        WriteUnaligned16 (
          &SmbiosRecord->ContainedObjectHandles[ContainedObjectIndex],
          ReferenceHandle
          );
      }
    }

    Status = StringTablePublishStringSet (
               &StrTable,
               (CHAR8 *)SmbiosRecord + FormattedLength,
               StringTableGetStringSetSize (&StrTable)
               );
    StringTableFree (&StrTable);
    StringTableInitialized = FALSE;
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    TableList[Index]    = (SMBIOS_STRUCTURE *)SmbiosRecord;
    CmObjectList[Index] = BaseboardInfo[Index].BaseboardInfoToken;
    SmbiosRecord        = NULL;
  }

  *Table         = TableList;
  *CmObjectToken = CmObjectList;
  *TableCount    = BaseboardCount;
  return EFI_SUCCESS;

ErrorExit:
  if (StringTableInitialized) {
    StringTableFree (&StrTable);
  }

  if (SmbiosRecord != NULL) {
    FreePool (SmbiosRecord);
  }

  if (TableList != NULL) {
    for (Index = 0; Index < BaseboardCount; Index++) {
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

/** Free resources allocated when installing SMBIOS Type 2 tables.

  @param [in]  This                  Pointer to the SMBIOS table generator.
  @param [in]  TableFactoryProtocol  Pointer to the SMBIOS Table Factory.
  @param [in]  SmbiosTableInfo       Pointer to the SMBIOS table information.
  @param [in]  CfgMgrProtocol        Pointer to the Configuration Manager.
  @param [in]  Table                 Pointer to the SMBIOS tables.
  @param [in]  CmObjectToken         Pointer to the CM ObjectToken array.
  @param [in]  TableCount            Number of SMBIOS tables.

  @retval EFI_SUCCESS            Resources freed successfully.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
FreeSmbiosType2TableEx (
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
    DEBUG ((DEBUG_ERROR, "%a: Invalid Parameter\n", __func__));
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

STATIC CONST SMBIOS_TABLE_GENERATOR  SmbiosType2Generator = {
  // Generator ID
  CREATE_STD_SMBIOS_TABLE_GEN_ID (EStdSmbiosTableIdType02),
  // Generator Description
  L"SMBIOS.TYPE2.GENERATOR",
  // SMBIOS Table Type
  EFI_SMBIOS_TYPE_BASEBOARD_INFORMATION,
  NULL,
  NULL,
  // Build table function Extended.
  BuildSmbiosType2TableEx,
  // Free function Extended.
  FreeSmbiosType2TableEx
};

/** Register the Generator with the SMBIOS Table Factory.

  @param [in]  ImageHandle  The handle to the image.
  @param [in]  SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS           The Generator is registered.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_ALREADY_STARTED   The Generator is already registered.
**/
EFI_STATUS
EFIAPI
SmbiosType2LibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterSmbiosTableGenerator (&SmbiosType2Generator);
  DEBUG ((DEBUG_INFO, "SMBIOS Type 2: Register Generator. Status = %r\n", Status));
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
SmbiosType2LibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterSmbiosTableGenerator (&SmbiosType2Generator);
  DEBUG ((DEBUG_INFO, "SMBIOS Type 2: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
