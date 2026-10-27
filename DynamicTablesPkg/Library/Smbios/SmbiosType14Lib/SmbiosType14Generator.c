/** @file
  SMBIOS Type 14 Group Associations Table Generator.

  Copyright (c) 2024 - 2026, NVIDIA CORPORATION & AFFILIATES. All rights reserved.<BR>
  Copyright (c) 2020 - 2021, Arm Limited. All rights reserved.<BR>

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

/** SMBIOS Type 14 Group Associations Generator

Requirements:
  The following Configuration Manager Objects are required by this Generator:
  - EArchCommonObjGroupAssociationInfo
  - EArchCommonObjGroupAssociationItem
    Required only when a group provides a non-null GroupItemListToken.
*/

#define SMBIOS_TYPE14_MAX_STRINGS  1
#define SMBIOS_TYPE14_BASE_LENGTH  OFFSET_OF (SMBIOS_TABLE_TYPE14, Group)
#define SMBIOS_TYPE14_MAX_ITEMS    ((MAX_UINT8 - SMBIOS_TYPE14_BASE_LENGTH) / sizeof (GROUP_STRUCT))

GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjGroupAssociationInfo,
  CM_ARCH_COMMON_GROUP_ASSOCIATION_INFO
  );

GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjGroupAssociationItem,
  CM_ARCH_COMMON_GROUP_ASSOCIATION_ITEM
  );

/** Validate Group Association Information.

  Validation follows SMBIOS Specification v3.9.0, Sections 6.1.3 and 7.15.

  @param [in] GroupAssociationInfo  Group Association Information to validate.

  @retval EFI_SUCCESS            The Group Association Information is valid.
  @retval EFI_INVALID_PARAMETER  The Group Association Information is invalid.
**/
STATIC
EFI_STATUS
ValidateGroupAssociationInfo (
  IN CONST CM_ARCH_COMMON_GROUP_ASSOCIATION_INFO  *GroupAssociationInfo
  )
{
  if ((GroupAssociationInfo->GroupAssociationInfoToken == CM_NULL_TOKEN) ||
      (AsciiStrnLenS (
         GroupAssociationInfo->GroupName,
         SMBIOS_MAX_STRING_SIZE
         ) == SMBIOS_MAX_STRING_SIZE))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Group Association Information\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/** Add an optional group name to a string table.

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

/** Construct SMBIOS Type 14 tables describing group associations.

  Referenced tables must be installed before the Type 14 generator runs so
  that their SMBIOS handles can be resolved. References to other Type 14
  tables are not supported.

  @param [in]  This                  Pointer to the SMBIOS table generator.
  @param [in]  TableFactoryProtocol  Pointer to the SMBIOS table factory.
  @param [in]  SmbiosTableInfo       Pointer to the SMBIOS table information.
  @param [in]  CfgMgrProtocol        Pointer to the Configuration Manager.
  @param [out] Table                 Pointer to the generated SMBIOS tables.
  @param [out] CmObjectToken         Pointer to the CM object tokens.
  @param [out] TableCount            Number of generated SMBIOS tables.

  @retval EFI_SUCCESS            Tables generated successfully.
  @retval EFI_INVALID_PARAMETER  A parameter or CM object is invalid.
  @retval EFI_NOT_FOUND          A required object, generator, or handle was not found.
  @retval EFI_OUT_OF_RESOURCES   Could not allocate memory.
  @retval EFI_UNSUPPORTED        A group item references another Type 14 table.
**/
STATIC
EFI_STATUS
EFIAPI
BuildSmbiosType14TableEx (
  IN  CONST SMBIOS_TABLE_GENERATOR                         *This,
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL   *CONST  TableFactoryProtocol,
  IN        CM_STD_OBJ_SMBIOS_TABLE_INFO           *CONST  SmbiosTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  OUT       SMBIOS_STRUCTURE                               ***Table,
  OUT       CM_OBJECT_TOKEN                                **CmObjectToken,
  OUT       UINTN                                  *CONST  TableCount
  )
{
  EFI_STATUS                             Status;
  CM_ARCH_COMMON_GROUP_ASSOCIATION_INFO  *GroupAssociationInfo;
  CM_ARCH_COMMON_GROUP_ASSOCIATION_ITEM  *GroupAssociationItem;
  UINT32                                 GroupAssociationCount;
  UINT32                                 GroupItemCount;
  SMBIOS_STRUCTURE                       **TableList;
  CM_OBJECT_TOKEN                        *CmObjectList;
  SMBIOS_TABLE_TYPE14                    *SmbiosRecord;
  CONST SMBIOS_TABLE_GENERATOR           *ReferencedGenerator;
  SMBIOS_HANDLE                          ReferencedHandle;
  STRING_TABLE                           StrTable;
  BOOLEAN                                StringTableInitialized;
  SMBIOS_TABLE_STRING                    GroupNameRef;
  UINTN                                  FormattedLength;
  UINTN                                  Index;
  UINTN                                  ItemIndex;

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

  Status = GetEArchCommonObjGroupAssociationInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &GroupAssociationInfo,
             &GroupAssociationCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get Group Association objects: %r\n", __func__, Status));
    return Status;
  }

  if (GroupAssociationCount == 0) {
    return EFI_NOT_FOUND;
  }

  if ((GroupAssociationInfo == NULL) ||
      (GroupAssociationCount > (MAX_UINT32 / sizeof (*TableList))) ||
      (GroupAssociationCount > (MAX_UINT32 / sizeof (*CmObjectList))))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Group Association object list\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  TableList = AllocateZeroPool (sizeof (*TableList) * GroupAssociationCount);
  if (TableList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CmObjectList = AllocateZeroPool (sizeof (*CmObjectList) * GroupAssociationCount);
  if (CmObjectList == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }

  for (Index = 0; Index < GroupAssociationCount; Index++) {
    Status = ValidateGroupAssociationInfo (&GroupAssociationInfo[Index]);
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    GroupAssociationItem = NULL;
    GroupItemCount       = 0;
    if (GroupAssociationInfo[Index].GroupItemListToken == CM_NULL_TOKEN) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Missing item list token for Group Association %u\n",
        __func__,
        Index
        ));
      Status = EFI_INVALID_PARAMETER;
      goto ErrorExit;
    }

    Status = GetEArchCommonObjGroupAssociationItem (
               CfgMgrProtocol,
               GroupAssociationInfo[Index].GroupItemListToken,
               &GroupAssociationItem,
               &GroupItemCount
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to get items for Group Association %u: %r\n",
        __func__,
        Index,
        Status
        ));
      goto ErrorExit;
    }

    if (GroupAssociationItem == NULL) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Missing item list for Group Association %u\n",
        __func__,
        Index
        ));
      Status = EFI_INVALID_PARAMETER;
      goto ErrorExit;
    }

    if (GroupItemCount == 0) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Empty item list for Group Association %u\n",
        __func__,
        Index
        ));
      Status = EFI_INVALID_PARAMETER;
      goto ErrorExit;
    }

    if (GroupItemCount > SMBIOS_TYPE14_MAX_ITEMS) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Too many items for Group Association %u: %u items\n",
        __func__,
        Index,
        GroupItemCount
        ));
      Status = EFI_INVALID_PARAMETER;
      goto ErrorExit;
    }

    FormattedLength = SMBIOS_TYPE14_BASE_LENGTH +
                      (GroupItemCount * sizeof (GROUP_STRUCT));

    Status = StringTableInitialize (&StrTable, SMBIOS_TYPE14_MAX_STRINGS);
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    StringTableInitialized = TRUE;
    Status                 = AddOptionalString (
                               &StrTable,
                               GroupAssociationInfo[Index].GroupName,
                               &GroupNameRef
                               );
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    SmbiosRecord = AllocateSmbiosRecord (FormattedLength, &StrTable);
    if (SmbiosRecord == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ErrorExit;
    }

    SmbiosRecord->Hdr.Type   = EFI_SMBIOS_TYPE_GROUP_ASSOCIATIONS;
    SmbiosRecord->Hdr.Length = (UINT8)FormattedLength;
    SmbiosRecord->GroupName  = GroupNameRef;

    for (ItemIndex = 0; ItemIndex < GroupItemCount; ItemIndex++) {
      if (GroupAssociationItem[ItemIndex].ReferencedObjectToken == CM_NULL_TOKEN) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Group Association %u item %u has a null object token\n",
          __func__,
          Index,
          ItemIndex
          ));
        Status = EFI_INVALID_PARAMETER;
        goto ErrorExit;
      }

      Status = TableFactoryProtocol->GetSmbiosTableGenerator (
                                       TableFactoryProtocol,
                                       GroupAssociationItem[ItemIndex].ReferencedTableGeneratorId,
                                       &ReferencedGenerator
                                       );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to find generator for Group Association %u item %u: %r\n",
          __func__,
          Index,
          ItemIndex,
          Status
          ));
        goto ErrorExit;
      }

      if (ReferencedGenerator->Type == EFI_SMBIOS_TYPE_GROUP_ASSOCIATIONS) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Group Association %u item %u references another Type 14 table, which is unsupported\n",
          __func__,
          Index,
          ItemIndex
          ));
        Status = EFI_UNSUPPORTED;
        goto ErrorExit;
      }

      ReferencedHandle = TableFactoryProtocol->GetSmbiosHandleEx (
                                                 GroupAssociationItem[ItemIndex].ReferencedTableGeneratorId,
                                                 GroupAssociationItem[ItemIndex].ReferencedObjectToken
                                                 );
      if (ReferencedHandle == SMBIOS_HANDLE_INVALID) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to resolve Group Association %u item %u\n",
          __func__,
          Index,
          ItemIndex
          ));
        Status = EFI_NOT_FOUND;
        goto ErrorExit;
      }

      SmbiosRecord->Group[ItemIndex].ItemType = ReferencedGenerator->Type;
      WriteUnaligned16 (
        &SmbiosRecord->Group[ItemIndex].ItemHandle,
        ReferencedHandle
        );
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
    CmObjectList[Index] = GroupAssociationInfo[Index].GroupAssociationInfoToken;
    SmbiosRecord        = NULL;
  }

  *Table         = TableList;
  *CmObjectToken = CmObjectList;
  *TableCount    = GroupAssociationCount;
  return EFI_SUCCESS;

ErrorExit:
  if (StringTableInitialized) {
    StringTableFree (&StrTable);
  }

  if (SmbiosRecord != NULL) {
    FreePool (SmbiosRecord);
  }

  if (TableList != NULL) {
    for (Index = 0; Index < GroupAssociationCount; Index++) {
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

/** Free resources allocated when installing SMBIOS Type 14 tables.

  @param [in] This                  Pointer to the SMBIOS table generator.
  @param [in] TableFactoryProtocol  Pointer to the SMBIOS Table Factory.
  @param [in] SmbiosTableInfo       Pointer to the SMBIOS table information.
  @param [in] CfgMgrProtocol        Pointer to the Configuration Manager.
  @param [in] Table                 Pointer to the SMBIOS tables.
  @param [in] CmObjectToken         Pointer to the CM ObjectToken array.
  @param [in] TableCount            Number of SMBIOS tables.

  @retval EFI_SUCCESS            Resources freed successfully.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
FreeSmbiosType14TableEx (
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

/** The SMBIOS Type 14 Table Generator.
*/
STATIC CONST SMBIOS_TABLE_GENERATOR  SmbiosType14Generator = {
  CREATE_STD_SMBIOS_TABLE_GEN_ID (EStdSmbiosTableIdType14),
  L"SMBIOS.TYPE14.GENERATOR",
  EFI_SMBIOS_TYPE_GROUP_ASSOCIATIONS,
  NULL,
  NULL,
  BuildSmbiosType14TableEx,
  FreeSmbiosType14TableEx
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
SmbiosType14LibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterSmbiosTableGenerator (&SmbiosType14Generator);
  DEBUG ((DEBUG_INFO, "SMBIOS Type 14: Register Generator. Status = %r\n", Status));
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
SmbiosType14LibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterSmbiosTableGenerator (&SmbiosType14Generator);
  DEBUG ((DEBUG_INFO, "SMBIOS Type 14: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
