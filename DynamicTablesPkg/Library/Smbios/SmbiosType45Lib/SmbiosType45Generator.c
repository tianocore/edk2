/** @file
  SMBIOS Type 45 Firmware Inventory Information Table Generator.

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

/** SMBIOS Type 45 Firmware Inventory Information Generator

Requirements:
  The following Configuration Manager Objects are required by this Generator:
  - EArchCommonObjFirmwareInventoryInfo
  - EArchCommonObjFirmwareInventoryComponent
    Required only when firmware information provides a non-null
    AssociatedComponentListToken.
*/

#define SMBIOS_TYPE45_MAX_STRINGS  6
#define SMBIOS_TYPE45_BASE_LENGTH  sizeof (SMBIOS_TABLE_TYPE45)
#define SMBIOS_TYPE45_MAX_ASSOCIATED_COMPONENTS  \
  ((MAX_UINT8 - SMBIOS_TYPE45_BASE_LENGTH) / sizeof (SMBIOS_HANDLE))

GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjFirmwareInventoryInfo,
  CM_ARCH_COMMON_FIRMWARE_INVENTORY_INFO
  );

GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjFirmwareInventoryComponent,
  CM_ARCH_COMMON_FIRMWARE_INVENTORY_COMPONENT
  );

/** Return whether a fixed-size CM string is null terminated.

  @param [in] String  String to validate.

  @retval TRUE   The string is null terminated.
  @retval FALSE  The string is not null terminated.
**/
STATIC
BOOLEAN
IsValidString (
  IN CONST CHAR8  *String
  )
{
  return (AsciiStrnLenS (String, SMBIOS_MAX_STRING_SIZE) < SMBIOS_MAX_STRING_SIZE);
}

/** Return whether a character is an ASCII decimal digit.

  @param [in] Character  Character to validate.

  @retval TRUE   Character is an ASCII decimal digit.
  @retval FALSE  Character is not an ASCII decimal digit.
**/
STATIC
BOOLEAN
IsDecimalDigit (
  IN CHAR8  Character
  )
{
  return ((Character >= '0') && (Character <= '9'));
}

/** Return whether a character is an ASCII hexadecimal digit.

  @param [in] Character  Character to validate.

  @retval TRUE   Character is an ASCII hexadecimal digit.
  @retval FALSE  Character is not an ASCII hexadecimal digit.
**/
STATIC
BOOLEAN
IsHexadecimalDigit (
  IN CHAR8  Character
  )
{
  return IsDecimalDigit (Character) ||
         ((Character >= 'a') && (Character <= 'f')) ||
         ((Character >= 'A') && (Character <= 'F'));
}

/** Return whether a two-digit decimal field is within a range.

  @param [in] String   Two-digit decimal field to validate.
  @param [in] Minimum  Minimum accepted value.
  @param [in] Maximum  Maximum accepted value.

  @retval TRUE   The field is within the range.
  @retval FALSE  The field is not a two-digit decimal value in the range.
**/
STATIC
BOOLEAN
IsDecimalPairInRange (
  IN CONST CHAR8  *String,
  IN UINTN        Minimum,
  IN UINTN        Maximum
  )
{
  UINTN  Value;

  if (!IsDecimalDigit (String[0]) || !IsDecimalDigit (String[1])) {
    return FALSE;
  }

  Value = ((String[0] - '0') * 10) + (String[1] - '0');
  return ((Value >= Minimum) && (Value <= Maximum));
}

/** Validate a firmware version string for the selected SMBIOS format.

  The standardized formats are defined by SMBIOS Specification v3.9.0,
  Table 134. Empty strings and implementation-specific formats are accepted.

  @param [in] String  Firmware version string to validate.
  @param [in] Format  SMBIOS firmware version format selector.

  @retval TRUE   The string matches the selected format.
  @retval FALSE  The string does not match the selected format.
**/
STATIC
BOOLEAN
IsValidVersionString (
  IN CONST CHAR8  *String,
  IN UINT8        Format
  )
{
  UINTN    Index;
  UINTN    Length;
  BOOLEAN  SeparatorFound;

  if (String[0] == '\0') {
    return TRUE;
  }

  if ((Format == VersionFormatTypeFreeForm) || (Format >= VersionFormatTypeOem)) {
    return TRUE;
  }

  Length = AsciiStrLen (String);
  if (Format == VersionFormatTypeMajorMinor) {
    SeparatorFound = FALSE;
    for (Index = 0; Index < Length; Index++) {
      if (String[Index] == '.') {
        if (SeparatorFound || (Index == 0) || (Index == (Length - 1))) {
          return FALSE;
        }

        SeparatorFound = TRUE;
      } else if (!IsDecimalDigit (String[Index])) {
        return FALSE;
      }
    }

    return SeparatorFound;
  }

  if (Format == VersionFormatType32BitHex) {
    Length = 8;
  } else if (Format == VersionFormatType64BitHex) {
    Length = 16;
  } else {
    return FALSE;
  }

  if ((AsciiStrLen (String) != (Length + 2)) ||
      (String[0] != '0') || (String[1] != 'x'))
  {
    return FALSE;
  }

  for (Index = 2; Index < (Length + 2); Index++) {
    if (!IsHexadecimalDigit (String[Index])) {
      return FALSE;
    }
  }

  return TRUE;
}

/** Validate an SMBIOS UUID-format firmware ID string.

  The format is defined by SMBIOS Specification v3.9.0, Table 135.
  Empty strings are accepted.

  @param [in] String  Firmware ID string to validate.

  @retval TRUE   The string is empty or uses the UUID format.
  @retval FALSE  The string does not use the UUID format.
**/
STATIC
BOOLEAN
IsValidUuidString (
  IN CONST CHAR8  *String
  )
{
  UINTN  Index;

  if (String[0] == '\0') {
    return TRUE;
  }

  if (AsciiStrLen (String) != 36) {
    return FALSE;
  }

  for (Index = 0; Index < 36; Index++) {
    if ((Index == 8) || (Index == 13) || (Index == 18) || (Index == 23)) {
      if (String[Index] != '-') {
        return FALSE;
      }
    } else if (!IsHexadecimalDigit (String[Index])) {
      return FALSE;
    }
  }

  return TRUE;
}

/** Validate a DSP0266 Date-Time string.

  @param [in] String  Date-Time string to validate.

  @retval TRUE   The string is empty or uses the DSP0266 Date-Time format.
  @retval FALSE  The string does not use the DSP0266 Date-Time format.
**/
STATIC
BOOLEAN
IsValidDateTimeString (
  IN CONST CHAR8  *String
  )
{
  UINTN  Index;
  UINTN  Length;

  if (String[0] == '\0') {
    return TRUE;
  }

  Length = AsciiStrLen (String);
  if ((Length < 20) ||
      !IsDecimalDigit (String[0]) ||
      !IsDecimalDigit (String[1]) ||
      !IsDecimalDigit (String[2]) ||
      !IsDecimalDigit (String[3]) ||
      (String[4] != '-') ||
      !IsDecimalPairInRange (&String[5], 1, 12) ||
      (String[7] != '-') ||
      !IsDecimalPairInRange (&String[8], 1, 31) ||
      (String[10] != 'T') ||
      !IsDecimalPairInRange (&String[11], 0, 23) ||
      (String[13] != ':') ||
      !IsDecimalPairInRange (&String[14], 0, 59) ||
      (String[16] != ':') ||
      !IsDecimalPairInRange (&String[17], 0, 59))
  {
    return FALSE;
  }

  Index = 19;
  if (String[Index] == '.') {
    Index++;
    if (!IsDecimalDigit (String[Index])) {
      return FALSE;
    }

    while (IsDecimalDigit (String[Index])) {
      Index++;
    }
  }

  if (String[Index] == 'Z') {
    return (String[Index + 1] == '\0');
  }

  if (((String[Index] != '+') && (String[Index] != '-')) ||
      ((Index + 6) != Length) ||
      !IsDecimalPairInRange (&String[Index + 1], 0, 23) ||
      (String[Index + 3] != ':') ||
      !IsDecimalPairInRange (&String[Index + 4], 0, 59))
  {
    return FALSE;
  }

  return TRUE;
}

/** Validate Firmware Inventory Information.

  Validation follows SMBIOS Specification v3.9.0, Sections 6.1.3 and 7.46.
  Annex A does not define additional Type 45-specific conformance checks.

  @param [in] FirmwareInfo  Firmware Inventory Information to validate.

  @retval EFI_SUCCESS            The information is valid.
  @retval EFI_INVALID_PARAMETER  The information is invalid.
**/
STATIC
EFI_STATUS
ValidateFirmwareInventoryInfo (
  IN CONST CM_ARCH_COMMON_FIRMWARE_INVENTORY_INFO  *FirmwareInfo
  )
{
  if ((FirmwareInfo->FirmwareInventoryInfoToken == CM_NULL_TOKEN) ||
      !IsValidString (FirmwareInfo->FirmwareComponentName) ||
      !IsValidString (FirmwareInfo->FirmwareVersion) ||
      !IsValidString (FirmwareInfo->FirmwareId) ||
      !IsValidString (FirmwareInfo->ReleaseDate) ||
      !IsValidString (FirmwareInfo->Manufacturer) ||
      !IsValidString (FirmwareInfo->LowestSupportedVersion))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Firmware Inventory Information\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if ((FirmwareInfo->FirmwareVersionFormat >= VersionFormatTypeReserved) &&
      (FirmwareInfo->FirmwareVersionFormat < VersionFormatTypeOem))
  {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Reserved firmware version format: 0x%x\n",
      __func__,
      FirmwareInfo->FirmwareVersionFormat
      ));
    return EFI_INVALID_PARAMETER;
  }

  if (!IsValidVersionString (
         FirmwareInfo->FirmwareVersion,
         FirmwareInfo->FirmwareVersionFormat
         ) ||
      !IsValidVersionString (
         FirmwareInfo->LowestSupportedVersion,
         FirmwareInfo->FirmwareVersionFormat
         ))
  {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Firmware version does not match format 0x%x\n",
      __func__,
      FirmwareInfo->FirmwareVersionFormat
      ));
    return EFI_INVALID_PARAMETER;
  }

  // SMBIOS 3.9.0, Table 135 reserves Firmware ID Format values 02h-7Fh.
  if ((FirmwareInfo->FirmwareIdFormat >= 0x02) &&
      (FirmwareInfo->FirmwareIdFormat < InventoryFirmwareIdFormatTypeOem))
  {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Reserved firmware ID format: 0x%x\n",
      __func__,
      FirmwareInfo->FirmwareIdFormat
      ));
    return EFI_INVALID_PARAMETER;
  }

  if ((FirmwareInfo->FirmwareIdFormat == FirmwareIdFormatTypeUuid) &&
      !IsValidUuidString (FirmwareInfo->FirmwareId))
  {
    DEBUG ((DEBUG_ERROR, "%a: Firmware ID is not a valid UUID\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if (!IsValidDateTimeString (FirmwareInfo->ReleaseDate)) {
    DEBUG ((DEBUG_ERROR, "%a: Firmware release date is not a valid DSP0266 Date-Time value\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if (FirmwareInfo->Characteristics.Reserved != 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Reserved firmware characteristic bits are set: 0x%x\n",
      __func__,
      FirmwareInfo->Characteristics.Reserved
      ));
    return EFI_INVALID_PARAMETER;
  }

  if ((FirmwareInfo->State < FirmwareInventoryStateOther) ||
      (FirmwareInfo->State > FirmwareInventoryStateUnavailableOffline))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid firmware state: 0x%x\n", __func__, FirmwareInfo->State));
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

/** Construct SMBIOS Type 45 tables describing firmware inventory.

  Referenced tables must be installed before the Type 45 generator runs so
  that associated component handles can be resolved.

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
**/
STATIC
EFI_STATUS
EFIAPI
BuildSmbiosType45TableEx (
  IN  CONST SMBIOS_TABLE_GENERATOR                         *This,
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL   *CONST  TableFactoryProtocol,
  IN        CM_STD_OBJ_SMBIOS_TABLE_INFO           *CONST  SmbiosTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  OUT       SMBIOS_STRUCTURE                               ***Table,
  OUT       CM_OBJECT_TOKEN                                **CmObjectToken,
  OUT       UINTN                                  *CONST  TableCount
  )
{
  EFI_STATUS                                   Status;
  CM_ARCH_COMMON_FIRMWARE_INVENTORY_INFO       *FirmwareInfo;
  CM_ARCH_COMMON_FIRMWARE_INVENTORY_COMPONENT  *AssociatedComponent;
  UINT32                                       FirmwareInfoCount;
  UINT32                                       AssociatedComponentCount;
  SMBIOS_STRUCTURE                             **TableList;
  CM_OBJECT_TOKEN                              *CmObjectList;
  SMBIOS_TABLE_TYPE45                          *SmbiosRecord;
  CONST SMBIOS_TABLE_GENERATOR                 *ReferencedGenerator;
  SMBIOS_HANDLE                                ReferencedHandle;
  STRING_TABLE                                 StrTable;
  BOOLEAN                                      StringTableInitialized;
  SMBIOS_TABLE_STRING                          FirmwareComponentNameRef;
  SMBIOS_TABLE_STRING                          FirmwareVersionRef;
  SMBIOS_TABLE_STRING                          FirmwareIdRef;
  SMBIOS_TABLE_STRING                          ReleaseDateRef;
  SMBIOS_TABLE_STRING                          ManufacturerRef;
  SMBIOS_TABLE_STRING                          LowestSupportedVersionRef;
  UINTN                                        FormattedLength;
  UINTN                                        Index;
  UINTN                                        ComponentIndex;

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

  Status = GetEArchCommonObjFirmwareInventoryInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &FirmwareInfo,
             &FirmwareInfoCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get Firmware Inventory objects: %r\n", __func__, Status));
    return Status;
  }

  if (FirmwareInfoCount == 0) {
    return EFI_NOT_FOUND;
  }

  if ((FirmwareInfo == NULL) ||
      (FirmwareInfoCount > (MAX_UINT32 / sizeof (*TableList))) ||
      (FirmwareInfoCount > (MAX_UINT32 / sizeof (*CmObjectList))))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Firmware Inventory object list\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  TableList = AllocateZeroPool (sizeof (*TableList) * FirmwareInfoCount);
  if (TableList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CmObjectList = AllocateZeroPool (sizeof (*CmObjectList) * FirmwareInfoCount);
  if (CmObjectList == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }

  for (Index = 0; Index < FirmwareInfoCount; Index++) {
    Status = ValidateFirmwareInventoryInfo (&FirmwareInfo[Index]);
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    AssociatedComponent      = NULL;
    AssociatedComponentCount = 0;
    if (FirmwareInfo[Index].AssociatedComponentListToken != CM_NULL_TOKEN) {
      Status = GetEArchCommonObjFirmwareInventoryComponent (
                 CfgMgrProtocol,
                 FirmwareInfo[Index].AssociatedComponentListToken,
                 &AssociatedComponent,
                 &AssociatedComponentCount
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to get associated components for firmware %u: %r\n",
          __func__,
          Index,
          Status
          ));
        goto ErrorExit;
      }

      if (AssociatedComponent == NULL) {
        DEBUG ((DEBUG_ERROR, "%a: Missing associated component list for firmware %u\n", __func__, Index));
        Status = EFI_INVALID_PARAMETER;
        goto ErrorExit;
      }
    }

    if (AssociatedComponentCount > SMBIOS_TYPE45_MAX_ASSOCIATED_COMPONENTS) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Too many associated components for firmware %u: %u\n",
        __func__,
        Index,
        AssociatedComponentCount
        ));
      Status = EFI_INVALID_PARAMETER;
      goto ErrorExit;
    }

    FormattedLength = SMBIOS_TYPE45_BASE_LENGTH +
                      (AssociatedComponentCount * sizeof (SMBIOS_HANDLE));

    Status = StringTableInitialize (&StrTable, SMBIOS_TYPE45_MAX_STRINGS);
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    StringTableInitialized = TRUE;
    Status                 = AddOptionalString (
                               &StrTable,
                               FirmwareInfo[Index].FirmwareComponentName,
                               &FirmwareComponentNameRef
                               );
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    Status = AddOptionalString (&StrTable, FirmwareInfo[Index].FirmwareVersion, &FirmwareVersionRef);
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    Status = AddOptionalString (&StrTable, FirmwareInfo[Index].FirmwareId, &FirmwareIdRef);
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    Status = AddOptionalString (&StrTable, FirmwareInfo[Index].ReleaseDate, &ReleaseDateRef);
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    Status = AddOptionalString (&StrTable, FirmwareInfo[Index].Manufacturer, &ManufacturerRef);
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    Status = AddOptionalString (
               &StrTable,
               FirmwareInfo[Index].LowestSupportedVersion,
               &LowestSupportedVersionRef
               );
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    SmbiosRecord = AllocateSmbiosRecord (FormattedLength, &StrTable);
    if (SmbiosRecord == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ErrorExit;
    }

    SmbiosRecord->Hdr.Type                 = SMBIOS_TYPE_FIRMWARE_INVENTORY_INFORMATION;
    SmbiosRecord->Hdr.Length               = (UINT8)FormattedLength;
    SmbiosRecord->FirmwareComponentName    = FirmwareComponentNameRef;
    SmbiosRecord->FirmwareVersion          = FirmwareVersionRef;
    SmbiosRecord->FirmwareVersionFormat    = FirmwareInfo[Index].FirmwareVersionFormat;
    SmbiosRecord->FirmwareId               = FirmwareIdRef;
    SmbiosRecord->FirmwareIdFormat         = FirmwareInfo[Index].FirmwareIdFormat;
    SmbiosRecord->ReleaseDate              = ReleaseDateRef;
    SmbiosRecord->Manufacturer             = ManufacturerRef;
    SmbiosRecord->LowestSupportedVersion   = LowestSupportedVersionRef;
    SmbiosRecord->State                    = FirmwareInfo[Index].State;
    SmbiosRecord->AssociatedComponentCount = (UINT8)AssociatedComponentCount;
    WriteUnaligned64 (&SmbiosRecord->ImageSize, FirmwareInfo[Index].ImageSize);
    SmbiosRecord->Characteristics = FirmwareInfo[Index].Characteristics;

    for (ComponentIndex = 0;
         (AssociatedComponent != NULL) && (ComponentIndex < AssociatedComponentCount);
         ComponentIndex++)
    {
      if (AssociatedComponent[ComponentIndex].ReferencedObjectToken == CM_NULL_TOKEN) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Firmware %u component %u has a null object token\n",
          __func__,
          Index,
          ComponentIndex
          ));
        Status = EFI_INVALID_PARAMETER;
        goto ErrorExit;
      }

      ReferencedGenerator = NULL;
      Status              = TableFactoryProtocol->GetSmbiosTableGenerator (
                                                    TableFactoryProtocol,
                                                    AssociatedComponent[ComponentIndex].ReferencedTableGeneratorId,
                                                    &ReferencedGenerator
                                                    );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to find generator for firmware %u component %u: %r\n",
          __func__,
          Index,
          ComponentIndex,
          Status
          ));
        goto ErrorExit;
      }

      if (ReferencedGenerator == NULL) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Generator is missing for firmware %u component %u\n",
          __func__,
          Index,
          ComponentIndex
          ));
        Status = EFI_NOT_FOUND;
        goto ErrorExit;
      }

      if (ReferencedGenerator->Type == SMBIOS_TYPE_FIRMWARE_INVENTORY_INFORMATION) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Firmware %u component %u references another Type 45 table\n",
          __func__,
          Index,
          ComponentIndex
          ));
        Status = EFI_INVALID_PARAMETER;
        goto ErrorExit;
      }

      ReferencedHandle = TableFactoryProtocol->GetSmbiosHandleEx (
                                                 AssociatedComponent[ComponentIndex].ReferencedTableGeneratorId,
                                                 AssociatedComponent[ComponentIndex].ReferencedObjectToken
                                                 );
      if (ReferencedHandle == SMBIOS_HANDLE_INVALID) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to resolve firmware %u component %u\n",
          __func__,
          Index,
          ComponentIndex
          ));
        Status = EFI_NOT_FOUND;
        goto ErrorExit;
      }

      WriteUnaligned16 (
        (UINT16 *)((UINT8 *)SmbiosRecord + SMBIOS_TYPE45_BASE_LENGTH +
                   (ComponentIndex * sizeof (SMBIOS_HANDLE))),
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
    CmObjectList[Index] = FirmwareInfo[Index].FirmwareInventoryInfoToken;
    SmbiosRecord        = NULL;
  }

  *Table         = TableList;
  *CmObjectToken = CmObjectList;
  *TableCount    = FirmwareInfoCount;
  return EFI_SUCCESS;

ErrorExit:
  if (StringTableInitialized) {
    StringTableFree (&StrTable);
  }

  if (SmbiosRecord != NULL) {
    FreePool (SmbiosRecord);
  }

  if (TableList != NULL) {
    for (Index = 0; Index < FirmwareInfoCount; Index++) {
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

/** Free resources allocated when installing SMBIOS Type 45 tables.

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
FreeSmbiosType45TableEx (
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

/** The SMBIOS Type 45 Table Generator.
*/
STATIC CONST SMBIOS_TABLE_GENERATOR  SmbiosType45Generator = {
  CREATE_STD_SMBIOS_TABLE_GEN_ID (EStdSmbiosTableIdType45),
  L"SMBIOS.TYPE45.GENERATOR",
  SMBIOS_TYPE_FIRMWARE_INVENTORY_INFORMATION,
  NULL,
  NULL,
  BuildSmbiosType45TableEx,
  FreeSmbiosType45TableEx
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
SmbiosType45LibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterSmbiosTableGenerator (&SmbiosType45Generator);
  DEBUG ((DEBUG_INFO, "SMBIOS Type 45: Register Generator. Status = %r\n", Status));
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
SmbiosType45LibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterSmbiosTableGenerator (&SmbiosType45Generator);
  DEBUG ((DEBUG_INFO, "SMBIOS Type 45: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
