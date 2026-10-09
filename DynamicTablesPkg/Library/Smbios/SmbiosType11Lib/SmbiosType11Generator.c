/** @file
  SMBIOS Type 11 OEM Strings Table Generator.

  Copyright (c) 2024 - 2026, NVIDIA CORPORATION & AFFILIATES. All rights reserved.<BR>
  Copyright (c) 2020 - 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SmbiosStringTableLib.h>

// Module specific include files.
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <Protocol/DynamicTableFactoryProtocol.h>
#include <IndustryStandard/SmBios.h>

/** SMBIOS Type 11 OEM Strings Generator

Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - EArchCommonObjOemStringsInfo
  - EArchCommonObjOemString
*/

/** This macro expands to a function that retrieves the OEM Strings
    Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjOemStringsInfo,
  CM_ARCH_COMMON_OEM_STRINGS_INFO
  );

/** This macro expands to a function that retrieves the OEM-defined string
    array from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjOemString,
  CM_ARCH_COMMON_OEM_STRING
  );

/** Validate SMBIOS Type 11 OEM Strings Information.

  Validation follows SMBIOS Specification v3.9.0, Sections 6.1.3 and 7.12.

  @param [in]  CfgMgrProtocol  Pointer to the Configuration Manager Protocol
                              interface.
  @param [in]  OemStringsInfo  OEM Strings Information CM object.
  @param [out] OemString       Pointer to the OEM-defined string array.
  @param [out] OemStringCount  Number of OEM-defined strings.

  @retval EFI_SUCCESS            The OEM Strings Information is valid.
  @retval EFI_INVALID_PARAMETER  The OEM Strings Information is invalid.
  @retval Others                 Error returned by the Configuration Manager.
**/
STATIC
EFI_STATUS
ValidateOemStringsInfo (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CfgMgrProtocol,
  IN  CONST CM_ARCH_COMMON_OEM_STRINGS_INFO       *OemStringsInfo,
  OUT       CM_ARCH_COMMON_OEM_STRING             **OemString,
  OUT       UINT32                                *OemStringCount
  )
{
  EFI_STATUS  Status;
  UINTN       Index;

  if ((OemStringsInfo->OemStringsInfoToken == CM_NULL_TOKEN) ||
      (OemStringsInfo->StringListToken == CM_NULL_TOKEN))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid OEM Strings Information\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  Status = GetEArchCommonObjOemString (
             CfgMgrProtocol,
             OemStringsInfo->StringListToken,
             OemString,
             OemStringCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get OEM String CM Objects. Status = %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  if ((*OemString == NULL) ||
      (*OemStringCount == 0) ||
      (*OemStringCount > MAX_UINT8))
  {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Invalid OEM string count %u\n",
      __func__,
      *OemStringCount
      ));
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 0; Index < *OemStringCount; Index++) {
    if ((*OemString)[Index].String[0] == '\0') {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Empty OEM string at index %u\n",
        __func__,
        Index
        ));
      return EFI_INVALID_PARAMETER;
    }
  }

  return EFI_SUCCESS;
}

/** Free any resources allocated when installing SMBIOS Type 11 table.

  @param [in]  This                 Pointer to the SMBIOS table generator.
  @param [in]  TableFactoryProtocol Pointer to the SMBIOS Table Factory
                                    Protocol interface.
  @param [in]  SmbiosTableInfo      Pointer to the SMBIOS table information.
  @param [in]  CfgMgrProtocol       Pointer to the Configuration Manager
                                    Protocol interface.
  @param [in]  Table                Pointer to the generated SMBIOS table.

  @retval EFI_SUCCESS  Table freed successfully.
**/
STATIC
EFI_STATUS
FreeSmbiosType11Table (
  IN      CONST SMBIOS_TABLE_GENERATOR                    *CONST  This,
  IN      CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL      *CONST  TableFactoryProtocol,
  IN      CONST CM_STD_OBJ_SMBIOS_TABLE_INFO              *CONST  SmbiosTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL      *CONST  CfgMgrProtocol,
  IN      SMBIOS_STRUCTURE                               **CONST  Table
  )
{
  if (*Table != NULL) {
    FreePool (*Table);
    *Table = NULL;
  }

  return EFI_SUCCESS;
}

/** Construct SMBIOS Type 11 OEM Strings table.

  If this function allocates any resources then they must be freed
  in the FreeSmbiosType11Table function.

  @param [in]  This                 Pointer to the SMBIOS table generator.
  @param [in]  TableFactoryProtocol Pointer to the SMBIOS Table Factory
                                    Protocol interface.
  @param [in]  SmbiosTableInfo      Pointer to the SMBIOS table information.
  @param [in]  CfgMgrProtocol       Pointer to the Configuration Manager
                                    Protocol interface.
  @param [out] Table                Pointer to the generated SMBIOS table.
  @param [out] CmObjectToken        Pointer to the CM Object Token for the
                                    generated SMBIOS table.

  @retval EFI_SUCCESS            Table generated successfully.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_NOT_FOUND          Required information is not found.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.
  @retval Others                 Error returned by the Configuration Manager.
**/
STATIC
EFI_STATUS
BuildSmbiosType11Table (
  IN  CONST SMBIOS_TABLE_GENERATOR                         *This,
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL   *CONST  TableFactoryProtocol,
  IN        CM_STD_OBJ_SMBIOS_TABLE_INFO           *CONST  SmbiosTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  OUT       SMBIOS_STRUCTURE                               **Table,
  OUT       CM_OBJECT_TOKEN                        *CONST  CmObjectToken
  )
{
  EFI_STATUS                       Status;
  CM_ARCH_COMMON_OEM_STRINGS_INFO  *OemStringsInfo;
  CM_ARCH_COMMON_OEM_STRING        *OemString;
  UINT32                           OemStringsInfoCount;
  UINT32                           OemStringCount;
  UINTN                            Index;
  STRING_TABLE                     StrTable;
  SMBIOS_TABLE_TYPE11              *SmbiosRecord;

  OemStringsInfo      = NULL;
  OemString           = NULL;
  OemStringsInfoCount = 0;
  OemStringCount      = 0;
  SmbiosRecord        = NULL;

  ASSERT (This != NULL);
  ASSERT (TableFactoryProtocol != NULL);
  ASSERT (SmbiosTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Table != NULL);
  ASSERT (CmObjectToken != NULL);
  ASSERT (SmbiosTableInfo->TableGeneratorId == This->GeneratorID);

  if ((This == NULL) || (TableFactoryProtocol == NULL) ||
      (SmbiosTableInfo == NULL) || (CfgMgrProtocol == NULL) ||
      (Table == NULL) || (CmObjectToken == NULL) ||
      (SmbiosTableInfo->TableGeneratorId != This->GeneratorID))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Parameter\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  *Table         = NULL;
  *CmObjectToken = CM_NULL_TOKEN;

  Status = GetEArchCommonObjOemStringsInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &OemStringsInfo,
             &OemStringsInfoCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get OEM Strings Information CM Object. Status = %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  if ((OemStringsInfo == NULL) || (OemStringsInfoCount != 1)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Expected one OEM Strings Information object, got %u\n",
      __func__,
      OemStringsInfoCount
      ));
    return EFI_INVALID_PARAMETER;
  }

  Status = ValidateOemStringsInfo (
             CfgMgrProtocol,
             OemStringsInfo,
             &OemString,
             &OemStringCount
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = StringTableInitialize (&StrTable, OemStringCount);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to initialize the string table. Status = %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  for (Index = 0; Index < OemStringCount; Index++) {
    Status = StringTableAddString (
               &StrTable,
               OemString[Index].String,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to add OEM string %u. Status = %r\n",
        __func__,
        Index,
        Status
        ));
      goto exitBuildSmbiosType11Table;
    }
  }

  SmbiosRecord = (SMBIOS_TABLE_TYPE11 *)AllocateSmbiosRecord (
                                          sizeof (SMBIOS_TABLE_TYPE11),
                                          &StrTable
                                          );
  if (SmbiosRecord == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto exitBuildSmbiosType11Table;
  }

  SmbiosRecord->Hdr.Type    = SMBIOS_TYPE_OEM_STRINGS;
  SmbiosRecord->Hdr.Length  = sizeof (SMBIOS_TABLE_TYPE11);
  SmbiosRecord->StringCount = (UINT8)OemStringCount;

  Status = StringTablePublishStringSet (
             &StrTable,
             (CHAR8 *)(SmbiosRecord + 1),
             StringTableGetStringSetSize (&StrTable)
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to publish the string table. Status = %r\n",
      __func__,
      Status
      ));
    goto exitBuildSmbiosType11Table;
  }

  *Table         = (SMBIOS_STRUCTURE *)SmbiosRecord;
  *CmObjectToken = OemStringsInfo->OemStringsInfoToken;
  Status         = EFI_SUCCESS;

exitBuildSmbiosType11Table:
  if (EFI_ERROR (Status) && (SmbiosRecord != NULL)) {
    FreePool (SmbiosRecord);
  }

  StringTableFree (&StrTable);
  return Status;
}

/** The SMBIOS Type 11 Table Generator.
*/
STATIC CONST SMBIOS_TABLE_GENERATOR  SmbiosType11Generator = {
  // Generator ID
  CREATE_STD_SMBIOS_TABLE_GEN_ID (EStdSmbiosTableIdType11),
  // Generator Description
  L"SMBIOS.TYPE11.GENERATOR",
  // SMBIOS structure type
  SMBIOS_TYPE_OEM_STRINGS,
  // Build table function
  BuildSmbiosType11Table,
  // Free function
  FreeSmbiosType11Table,
  NULL,
  NULL
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
SmbiosType11LibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterSmbiosTableGenerator (&SmbiosType11Generator);
  DEBUG ((DEBUG_INFO, "SMBIOS Type 11: Register Generator. Status = %r\n", Status));
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
SmbiosType11LibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterSmbiosTableGenerator (&SmbiosType11Generator);
  DEBUG ((DEBUG_INFO, "SMBIOS Type 11: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);

  return Status;
}
