/** @file
  SMBIOS Type 13 Firmware Language Information Table Generator.

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

/** SMBIOS Type 13 Firmware Language Information Generator

Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - EArchCommonObjBiosLanguageInfo
  - EArchCommonObjBiosLanguage
*/

#define SMBIOS_TYPE13_ABBREVIATED_FORMAT_BIT  BIT0
#define SMBIOS_TYPE13_FLAGS_RESERVED_MASK     0xFE

/** This macro expands to a function that retrieves the Firmware Language
    Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjBiosLanguageInfo,
  CM_ARCH_COMMON_BIOS_LANGUAGE_INFO
  );

/** This macro expands to a function that retrieves the installable Firmware
    Language array from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjBiosLanguage,
  CM_ARCH_COMMON_BIOS_LANGUAGE
  );

/** Check whether a firmware language string has the configured SMBIOS format.

  @param [in] Language  Firmware language string.
  @param [in] Flags     SMBIOS Type 13 language format flags.

  @retval TRUE   The language string has the configured format.
  @retval FALSE  The language string does not have the configured format.
**/
STATIC
BOOLEAN
IsLanguageStringValid (
  IN CONST CHAR8  *Language,
  IN       UINT8  Flags
  )
{
  UINTN  Length;

  Length = AsciiStrLen (Language);

  if ((Flags & SMBIOS_TYPE13_ABBREVIATED_FORMAT_BIT) != 0) {
    // ISO 639-1 language code followed by ISO 3166-1 alpha-2 territory code.
    return (Length == 4);
  }

  // Long format: language code | territory code | encoding method.
  return (Length > 6) &&
         (Language[2] == '|') &&
         (Language[5] == '|');
}

/** Validate SMBIOS Type 13 Firmware Language Information.

  Validation follows SMBIOS Specification v3.9.0, Section 7.14.

  @param [in]  CfgMgrProtocol    Pointer to the Configuration Manager
                                Protocol interface.
  @param [in]  BiosLanguageInfo  Firmware Language Information CM object.
  @param [out] BiosLanguage      Pointer to the installable language array.
  @param [out] BiosLanguageCount Number of installable languages.

  @retval EFI_SUCCESS            The Firmware Language Information is valid.
  @retval EFI_INVALID_PARAMETER  The Firmware Language Information is invalid.
  @retval Others                 Error returned by the Configuration Manager.
**/
STATIC
EFI_STATUS
ValidateBiosLanguageInfo (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CfgMgrProtocol,
  IN  CONST CM_ARCH_COMMON_BIOS_LANGUAGE_INFO      *BiosLanguageInfo,
  OUT       CM_ARCH_COMMON_BIOS_LANGUAGE           **BiosLanguage,
  OUT       UINT32                                 *BiosLanguageCount
  )
{
  EFI_STATUS  Status;
  UINTN       Index;

  if ((BiosLanguageInfo->BiosLanguageInfoToken == CM_NULL_TOKEN) ||
      (BiosLanguageInfo->LanguageListToken == CM_NULL_TOKEN) ||
      ((BiosLanguageInfo->Flags & SMBIOS_TYPE13_FLAGS_RESERVED_MASK) != 0))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Firmware Language Information\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  Status = GetEArchCommonObjBiosLanguage (
             CfgMgrProtocol,
             BiosLanguageInfo->LanguageListToken,
             BiosLanguage,
             BiosLanguageCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get Firmware Language CM Objects. Status = %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  if ((*BiosLanguage == NULL) ||
      (*BiosLanguageCount == 0) ||
      (*BiosLanguageCount > MAX_UINT8) ||
      (BiosLanguageInfo->CurrentLanguage == 0) ||
      (BiosLanguageInfo->CurrentLanguage > *BiosLanguageCount))
  {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Invalid language count %u or current language %u\n",
      __func__,
      *BiosLanguageCount,
      BiosLanguageInfo->CurrentLanguage
      ));
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 0; Index < *BiosLanguageCount; Index++) {
    if (!IsLanguageStringValid (
           (*BiosLanguage)[Index].Language,
           BiosLanguageInfo->Flags
           ))
    {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Invalid firmware language at index %u\n",
        __func__,
        Index
        ));
      return EFI_INVALID_PARAMETER;
    }
  }

  return EFI_SUCCESS;
}

/** Free any resources allocated when installing SMBIOS Type 13 table.

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
FreeSmbiosType13Table (
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

/** Construct SMBIOS Type 13 Firmware Language Information table.

  If this function allocates any resources then they must be freed
  in the FreeSmbiosType13Table function.

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
BuildSmbiosType13Table (
  IN  CONST SMBIOS_TABLE_GENERATOR                         *This,
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL   *CONST  TableFactoryProtocol,
  IN        CM_STD_OBJ_SMBIOS_TABLE_INFO           *CONST  SmbiosTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  OUT       SMBIOS_STRUCTURE                               **Table,
  OUT       CM_OBJECT_TOKEN                        *CONST  CmObjectToken
  )
{
  EFI_STATUS                         Status;
  CM_ARCH_COMMON_BIOS_LANGUAGE_INFO  *BiosLanguageInfo;
  CM_ARCH_COMMON_BIOS_LANGUAGE       *BiosLanguage;
  UINT32                             BiosLanguageInfoCount;
  UINT32                             BiosLanguageCount;
  UINTN                              Index;
  STRING_TABLE                       StrTable;
  SMBIOS_TABLE_TYPE13                *SmbiosRecord;

  BiosLanguageInfo      = NULL;
  BiosLanguage          = NULL;
  BiosLanguageInfoCount = 0;
  BiosLanguageCount     = 0;
  SmbiosRecord          = NULL;

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

  Status = GetEArchCommonObjBiosLanguageInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &BiosLanguageInfo,
             &BiosLanguageInfoCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get Firmware Language Information CM Object. Status = %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  if ((BiosLanguageInfo == NULL) || (BiosLanguageInfoCount != 1)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Expected one Firmware Language Information object, got %u\n",
      __func__,
      BiosLanguageInfoCount
      ));
    return EFI_INVALID_PARAMETER;
  }

  Status = ValidateBiosLanguageInfo (
             CfgMgrProtocol,
             BiosLanguageInfo,
             &BiosLanguage,
             &BiosLanguageCount
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = StringTableInitialize (&StrTable, BiosLanguageCount);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to initialize the string table. Status = %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  for (Index = 0; Index < BiosLanguageCount; Index++) {
    Status = StringTableAddString (
               &StrTable,
               BiosLanguage[Index].Language,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to add firmware language %u. Status = %r\n",
        __func__,
        Index,
        Status
        ));
      goto exitBuildSmbiosType13Table;
    }
  }

  SmbiosRecord = (SMBIOS_TABLE_TYPE13 *)AllocateSmbiosRecord (
                                           sizeof (SMBIOS_TABLE_TYPE13),
                                           &StrTable
                                           );
  if (SmbiosRecord == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto exitBuildSmbiosType13Table;
  }

  SmbiosRecord->Hdr.Type             = SMBIOS_TYPE_BIOS_LANGUAGE_INFORMATION;
  SmbiosRecord->Hdr.Length           = sizeof (SMBIOS_TABLE_TYPE13);
  SmbiosRecord->InstallableLanguages = (UINT8)BiosLanguageCount;
  SmbiosRecord->Flags                = BiosLanguageInfo->Flags;
  SmbiosRecord->CurrentLanguages     = BiosLanguageInfo->CurrentLanguage;

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
    goto exitBuildSmbiosType13Table;
  }

  *Table         = (SMBIOS_STRUCTURE *)SmbiosRecord;
  *CmObjectToken = BiosLanguageInfo->BiosLanguageInfoToken;
  Status         = EFI_SUCCESS;

exitBuildSmbiosType13Table:
  if (EFI_ERROR (Status) && (SmbiosRecord != NULL)) {
    FreePool (SmbiosRecord);
  }

  StringTableFree (&StrTable);
  return Status;
}

/** The SMBIOS Type 13 Table Generator.
*/
STATIC CONST SMBIOS_TABLE_GENERATOR  SmbiosType13Generator = {
  // Generator ID
  CREATE_STD_SMBIOS_TABLE_GEN_ID (EStdSmbiosTableIdType13),
  // Generator Description
  L"SMBIOS.TYPE13.GENERATOR",
  // SMBIOS structure type
  SMBIOS_TYPE_BIOS_LANGUAGE_INFORMATION,
  // Build table function
  BuildSmbiosType13Table,
  // Free function
  FreeSmbiosType13Table,
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
SmbiosType13LibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterSmbiosTableGenerator (&SmbiosType13Generator);
  DEBUG ((DEBUG_INFO, "SMBIOS Type 13: Register Generator. Status = %r\n", Status));
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
SmbiosType13LibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterSmbiosTableGenerator (&SmbiosType13Generator);
  DEBUG ((DEBUG_INFO, "SMBIOS Type 13: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);

  return Status;
}
