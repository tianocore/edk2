/** @file
  SMBIOS Type40 Table Generator.

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SmbiosStringTableLib.h>

// Module specific include files.
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <Protocol/DynamicTableFactoryProtocol.h>
#include <IndustryStandard/SmBios.h>

/** SMBIOS Type 40 Additional Information Generator

Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - EArchCommonObjAdditionalInformation
  - EArchCommonObjAdditionalInformationEntry
  - EArchCommonObjAdditionalInformationValue

  The Additional Information object provides the list of Type 40 entries.
  Each Additional Information Entry references the SMBIOS structure field
  being described and a value object containing the raw entry Value bytes.
*/

/**
  This macro expands to a function that retrieves the Additional Information
  object from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjAdditionalInformation,
  CM_ARCH_COMMON_ADDITIONAL_INFORMATION
  );

/**
  This macro expands to a function that retrieves the Additional Information
  Entry array from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjAdditionalInformationEntry,
  CM_ARCH_COMMON_ADDITIONAL_INFORMATION_ENTRY
  );

/**
  This macro expands to a function that retrieves the Additional Information
  Value bytes from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjAdditionalInformationValue,
  CM_ARCH_COMMON_ADDITIONAL_INFORMATION_VALUE
  );

/**
  Ensure the platform-configured maximum value size is non-zero and does not
  exceed the capacity of the Additional Information Value CM object.
*/
STATIC_ASSERT (
  (FixedPcdGet8 (PcdMaxAdditionalInformationValue) > 0) &&
  (FixedPcdGet8 (PcdMaxAdditionalInformationValue) <=
   SMBIOS_MAX_ADDITIONAL_INFORMATION_VALUE_SIZE),
  "PcdMaxAdditionalInformationValue is invalid"
  );

/** Validate an SMBIOS Type 40 CM object and calculate its record size.

  @param [in]  TableFactoryProtocol             Pointer to the SMBIOS table
                                                   factory protocol.
  @param [in]  CfgMgrProtocol                    Pointer to the Configuration
                                                   Manager Protocol interface.
  @param [in]  AdditionalInformation             Pointer to the Additional
                                                   Information CM object.
  @param [in]  AdditionalInformationIndex        Index of the Additional
                                                   Information CM object.
  @param [out] AdditionalInformationEntry        Pointer to the Additional
                                                   Information Entry CM objects.
  @param [out] AdditionalInformationEntryCount   Number of Additional
                                                   Information Entry CM objects.
  @param [out] RecordSize                        Size of the formatted SMBIOS
                                                   Type 40 record.

  @retval EFI_SUCCESS            The CM object is valid.
  @retval EFI_INVALID_PARAMETER  The CM object is invalid.
  @retval EFI_NOT_FOUND          A referenced SMBIOS handle is not found.
  @retval Others                 Error returned by the Configuration Manager.
**/
STATIC
EFI_STATUS
ValidateSmbiosType40Table (
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL         *TableFactoryProtocol,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL         *CfgMgrProtocol,
  IN  CONST CM_ARCH_COMMON_ADDITIONAL_INFORMATION        *AdditionalInformation,
  IN        UINTN                                        AdditionalInformationIndex,
  OUT       CM_ARCH_COMMON_ADDITIONAL_INFORMATION_ENTRY  **AdditionalInformationEntry,
  OUT       UINT32                                       *AdditionalInformationEntryCount,
  OUT       UINTN                                        *RecordSize
  )
{
  EFI_STATUS                                   Status;
  CM_ARCH_COMMON_ADDITIONAL_INFORMATION_VALUE  *AdditionalInformationValue;
  SMBIOS_HANDLE                                ReferencedHandle;
  UINT32                                       AdditionalInformationValueCount;
  UINTN                                        EntryIndex;
  UINTN                                        EntryLength;

  if ((AdditionalInformation->AdditionalInformationToken == CM_NULL_TOKEN) ||
      (AdditionalInformation->AdditionalInformationEntryListToken == CM_NULL_TOKEN))
  {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Invalid token for object %u\n",
      __func__,
      AdditionalInformationIndex
      ));
    return EFI_INVALID_PARAMETER;
  }

  Status = GetEArchCommonObjAdditionalInformationEntry (
             CfgMgrProtocol,
             AdditionalInformation->AdditionalInformationEntryListToken,
             AdditionalInformationEntry,
             AdditionalInformationEntryCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get Additional Information Entry CM Object "
      "for object %u. Status = %r\n",
      __func__,
      AdditionalInformationIndex,
      Status
      ));
    return Status;
  }

  if ((*AdditionalInformationEntryCount == 0) ||
      (*AdditionalInformationEntryCount > MAX_UINT8))
  {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Invalid Additional Information Entry count %u for object %u\n",
      __func__,
      *AdditionalInformationEntryCount,
      AdditionalInformationIndex
      ));
    return EFI_INVALID_PARAMETER;
  }

  *RecordSize = OFFSET_OF (
                  SMBIOS_TABLE_TYPE40,
                  AdditionalInfoEntries
                  );

  for (EntryIndex = 0;
       EntryIndex < *AdditionalInformationEntryCount;
       EntryIndex++)
  {
    if (((*AdditionalInformationEntry)[EntryIndex].ReferencedObjectToken ==
         CM_NULL_TOKEN) ||
        ((*AdditionalInformationEntry)[EntryIndex].ValueToken == CM_NULL_TOKEN))
    {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Invalid token for Additional Information Entry %u\n",
        __func__,
        EntryIndex
        ));
      return EFI_INVALID_PARAMETER;
    }

    ReferencedHandle = TableFactoryProtocol->GetSmbiosHandleEx (
                                               (*AdditionalInformationEntry)[EntryIndex].ReferencedTableGeneratorId,
                                               (*AdditionalInformationEntry)[EntryIndex].ReferencedObjectToken
                                               );
    if (ReferencedHandle == SMBIOS_HANDLE_INVALID) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to resolve referenced SMBIOS handle for Additional Information Entry %u\n",
        __func__,
        EntryIndex
        ));
      return EFI_NOT_FOUND;
    }

    Status = GetEArchCommonObjAdditionalInformationValue (
               CfgMgrProtocol,
               (*AdditionalInformationEntry)[EntryIndex].ValueToken,
               &AdditionalInformationValue,
               &AdditionalInformationValueCount
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to get Additional Information Value CM Object for Entry %u. Status = %r\n",
        __func__,
        EntryIndex,
        Status
        ));
      return Status;
    }

    if ((AdditionalInformationValue == NULL) ||
        (AdditionalInformationValueCount != 1) ||
        (AdditionalInformationValue->Len == 0) ||
        (AdditionalInformationValue->Len >
         FixedPcdGet8 (PcdMaxAdditionalInformationValue)))
    {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Invalid Additional Information Value for Entry %u\n",
        __func__,
        EntryIndex
        ));
      return EFI_INVALID_PARAMETER;
    }

    EntryLength = OFFSET_OF (ADDITIONAL_INFORMATION_ENTRY, Value) +
                  AdditionalInformationValue->Len;

    if (EntryLength > MAX_UINT8) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Additional Information Entry %u length %u exceeds SMBIOS Type 40 limit\n",
        __func__,
        EntryIndex,
        EntryLength
        ));
      return EFI_INVALID_PARAMETER;
    }

    *RecordSize += EntryLength;
  }

  if (*RecordSize > MAX_UINT8) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Type 40 record size %u exceeds SMBIOS header length limit\n",
      __func__,
      *RecordSize
      ));
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/** Construct SMBIOS Type 40 table describing Additional Information.

  If this function allocates any resources then they must be freed in
  FreeSmbiosType40TableEx().

  @param [in]  This                  Pointer to the SMBIOS table generator.
  @param [in]  TableFactoryProtocol  Pointer to the SMBIOS table factory protocol.
  @param [in]  SmbiosTableInfo       Pointer to the SMBIOS table information.
  @param [in]  CfgMgrProtocol        Pointer to the Configuration Manager
                                     Protocol interface.
  @param [out] Table                 Pointer to the generated SMBIOS table.
  @param [out] CmObjectToken         Pointer to the CM object token for the
                                     generated SMBIOS table.
  @param [out] TableCount            Number of generated SMBIOS tables.

  @retval EFI_SUCCESS            Table generated successfully.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_NOT_FOUND          Required CM object is not found.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
BuildSmbiosType40TableEx (
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
  CM_ARCH_COMMON_ADDITIONAL_INFORMATION        *AdditionalInformation;
  CM_ARCH_COMMON_ADDITIONAL_INFORMATION_ENTRY  *AdditionalInformationEntry;
  CM_ARCH_COMMON_ADDITIONAL_INFORMATION_VALUE  *AdditionalInformationValue;
  UINT32                                       AdditionalInformationCount;
  UINT32                                       AdditionalInformationEntryCount;
  UINT32                                       AdditionalInformationValueCount;
  STRING_TABLE                                 StrTable;
  SMBIOS_STRUCTURE                             **TableList;
  CM_OBJECT_TOKEN                              *CmObjectList;
  SMBIOS_TABLE_TYPE40                          *SmbiosRecord;
  ADDITIONAL_INFORMATION_ENTRY                 *SmbiosEntry;
  SMBIOS_TABLE_STRING                          *EntryStringRef;
  SMBIOS_HANDLE                                ReferencedHandle;
  UINTN                                        EntryLength;
  UINTN                                        RecordSize;
  UINTN                                        StringAreaSize;
  UINTN                                        TableIndex;
  UINTN                                        EntryIndex;
  BOOLEAN                                      StrTableInitialized;

  AdditionalInformation           = NULL;
  AdditionalInformationEntry      = NULL;
  AdditionalInformationValue      = NULL;
  AdditionalInformationCount      = 0;
  AdditionalInformationEntryCount = 0;
  AdditionalInformationValueCount = 0;
  TableList                       = NULL;
  CmObjectList                    = NULL;
  SmbiosRecord                    = NULL;
  SmbiosEntry                     = NULL;
  EntryStringRef                  = NULL;
  RecordSize                      = 0;
  StringAreaSize                  = 0;
  StrTableInitialized             = FALSE;

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
      (Table == NULL) || (CmObjectToken == NULL) ||
      (TableCount == NULL))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Parameter\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if (SmbiosTableInfo->TableGeneratorId != This->GeneratorID) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Invalid Generator ID. Expected 0x%x, got 0x%x\n",
      __func__,
      This->GeneratorID,
      SmbiosTableInfo->TableGeneratorId
      ));
    return EFI_INVALID_PARAMETER;
  }

  *Table         = NULL;
  *CmObjectToken = NULL;
  *TableCount    = 0;

  Status = GetEArchCommonObjAdditionalInformation (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &AdditionalInformation,
             &AdditionalInformationCount
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get Additional Information CM Object. Status = %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  if (AdditionalInformationCount == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: No Additional Information CM Objects found\n",
      __func__
      ));
    return EFI_NOT_FOUND;
  }

  TableList = AllocateZeroPool (
                sizeof (SMBIOS_STRUCTURE *) *
                AdditionalInformationCount
                );
  if (TableList == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto exitBuildSmbiosType40TableEx;
  }

  CmObjectList = AllocateZeroPool (
                   sizeof (CM_OBJECT_TOKEN) *
                   AdditionalInformationCount
                   );
  if (CmObjectList == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto exitBuildSmbiosType40TableEx;
  }

  for (TableIndex = 0;
       TableIndex < AdditionalInformationCount;
       TableIndex++)
  {
    AdditionalInformationEntry      = NULL;
    AdditionalInformationValue      = NULL;
    AdditionalInformationEntryCount = 0;
    AdditionalInformationValueCount = 0;
    SmbiosRecord                    = NULL;
    SmbiosEntry                     = NULL;
    EntryStringRef                  = NULL;
    RecordSize                      = 0;
    StringAreaSize                  = 0;
    StrTableInitialized             = FALSE;

    Status = ValidateSmbiosType40Table (
               TableFactoryProtocol,
               CfgMgrProtocol,
               &AdditionalInformation[TableIndex],
               TableIndex,
               &AdditionalInformationEntry,
               &AdditionalInformationEntryCount,
               &RecordSize
               );
    if (EFI_ERROR (Status)) {
      goto exitBuildSmbiosType40TableEx;
    }

    Status = StringTableInitialize (
               &StrTable,
               AdditionalInformationEntryCount
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to initialise Type 40 string table for object %u. "
        "Status = %r\n",
        __func__,
        TableIndex,
        Status
        ));
      goto exitBuildSmbiosType40TableEx;
    }

    StrTableInitialized = TRUE;

    EntryStringRef = AllocateZeroPool (
                       sizeof (SMBIOS_TABLE_STRING) *
                       AdditionalInformationEntryCount
                       );
    if (EntryStringRef == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto exitBuildSmbiosType40TableEx;
    }

    for (EntryIndex = 0;
         EntryIndex < AdditionalInformationEntryCount;
         EntryIndex++)
    {
      if (AdditionalInformationEntry[EntryIndex].EntryString[0] != '\0') {
        Status = StringTableAddString (
                   &StrTable,
                   AdditionalInformationEntry[EntryIndex].EntryString,
                   &EntryStringRef[EntryIndex]
                   );
        if (EFI_ERROR (Status)) {
          DEBUG ((
            DEBUG_ERROR,
            "%a: Failed to add EntryString for Additional Information Entry %u. Status = %r\n",
            __func__,
            EntryIndex,
            Status
            ));
          goto exitBuildSmbiosType40TableEx;
        }
      }
    }

    SmbiosRecord = (SMBIOS_TABLE_TYPE40 *)AllocateSmbiosRecord (
                                            RecordSize,
                                            &StrTable
                                            );
    if (SmbiosRecord == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto exitBuildSmbiosType40TableEx;
    }

    SmbiosRecord->Hdr.Type                             = EFI_SMBIOS_TYPE_ADDITIONAL_INFORMATION;
    SmbiosRecord->Hdr.Length                           = (UINT8)RecordSize;
    SmbiosRecord->Hdr.Handle                           = SMBIOS_HANDLE_PI_RESERVED;
    SmbiosRecord->NumberOfAdditionalInformationEntries =
      (UINT8)AdditionalInformationEntryCount;

    SmbiosEntry = SmbiosRecord->AdditionalInfoEntries;

    for (EntryIndex = 0;
         EntryIndex < AdditionalInformationEntryCount;
         EntryIndex++)
    {
      AdditionalInformationValue      = NULL;
      AdditionalInformationValueCount = 0;

      Status = GetEArchCommonObjAdditionalInformationValue (
                 CfgMgrProtocol,
                 AdditionalInformationEntry[EntryIndex].ValueToken,
                 &AdditionalInformationValue,
                 &AdditionalInformationValueCount
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to get Additional Information Value CM Object for Entry %u. Status = %r\n",
          __func__,
          EntryIndex,
          Status
          ));
        goto exitBuildSmbiosType40TableEx;
      }

      if ((AdditionalInformationValue == NULL) ||
          (AdditionalInformationValueCount != 1))
      {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Invalid Additional Information Value CM Object for Entry %u\n",
          __func__,
          EntryIndex
          ));
        Status = EFI_INVALID_PARAMETER;
        goto exitBuildSmbiosType40TableEx;
      }

      ReferencedHandle = TableFactoryProtocol->GetSmbiosHandleEx (
                                                 AdditionalInformationEntry[EntryIndex].ReferencedTableGeneratorId,
                                                 AdditionalInformationEntry[EntryIndex].ReferencedObjectToken
                                                 );

      EntryLength = OFFSET_OF (ADDITIONAL_INFORMATION_ENTRY, Value) +
                    AdditionalInformationValue->Len;

      SmbiosEntry->EntryLength      = (UINT8)EntryLength;
      SmbiosEntry->ReferencedHandle = ReferencedHandle;
      SmbiosEntry->ReferencedOffset =
        AdditionalInformationEntry[EntryIndex].ReferencedOffset;
      SmbiosEntry->EntryString = EntryStringRef[EntryIndex];

      CopyMem (
        SmbiosEntry->Value,
        AdditionalInformationValue->Value,
        AdditionalInformationValue->Len
        );

      SmbiosEntry = (ADDITIONAL_INFORMATION_ENTRY *)(
                                                     (UINT8 *)SmbiosEntry + EntryLength
                                                     );
    }

    StringAreaSize = StringTableGetStringSetSize (&StrTable);

    Status = StringTablePublishStringSet (
               &StrTable,
               (CHAR8 *)SmbiosRecord + RecordSize,
               StringAreaSize
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to publish Type 40 string set. Status = %r\n",
        __func__,
        Status
        ));
      goto exitBuildSmbiosType40TableEx;
    }

    TableList[TableIndex]    = (SMBIOS_STRUCTURE *)SmbiosRecord;
    CmObjectList[TableIndex] =
      AdditionalInformation[TableIndex].AdditionalInformationToken;

    SmbiosRecord = NULL;

    if (StrTableInitialized) {
      StringTableFree (&StrTable);
      StrTableInitialized = FALSE;
    }

    if (EntryStringRef != NULL) {
      FreePool (EntryStringRef);
      EntryStringRef = NULL;
    }
  }

  *Table         = TableList;
  *CmObjectToken = CmObjectList;
  *TableCount    = AdditionalInformationCount;
  Status         = EFI_SUCCESS;

exitBuildSmbiosType40TableEx:
  if (StrTableInitialized) {
    StringTableFree (&StrTable);
  }

  if (EntryStringRef != NULL) {
    FreePool (EntryStringRef);
  }

  if (EFI_ERROR (Status)) {
    if (TableList != NULL) {
      for (TableIndex = 0;
           TableIndex < AdditionalInformationCount;
           TableIndex++)
      {
        if (TableList[TableIndex] != NULL) {
          FreePool (TableList[TableIndex]);
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
  }

  return Status;
}

/** Free any resources allocated for constructing SMBIOS Type 40 table.

  @param [in]      This                 Pointer to the SMBIOS table generator.
  @param [in]      TableFactoryProtocol Pointer to the SMBIOS table factory
                                           protocol.
  @param [in]      SmbiosTableInfo      Pointer to the SMBIOS table information.
  @param [in]      CfgMgrProtocol       Pointer to the Configuration Manager
                                           Protocol interface.
  @param [in]      Table                Pointer to the SMBIOS table.
  @param [in]      CmObjectToken        Pointer to the CM object token.
  @param [in]      TableCount           Number of generated SMBIOS tables.

  @retval EFI_SUCCESS            Resources freed successfully.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
FreeSmbiosType40TableEx (
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

/** The SMBIOS Type 40 Table Generator.
*/
STATIC CONST SMBIOS_TABLE_GENERATOR  SmbiosType40Generator = {
  // Generator ID
  CREATE_STD_SMBIOS_TABLE_GEN_ID (EStdSmbiosTableIdType40),
  // Generator Description
  L"SMBIOS.TYPE40.GENERATOR",
  // SMBIOS structure type
  SMBIOS_TYPE_ADDITIONAL_INFORMATION,
  NULL,
  NULL,
  // Build table function.
  BuildSmbiosType40TableEx,
  // Free function.
  FreeSmbiosType40TableEx
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
SmbiosType40LibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterSmbiosTableGenerator (&SmbiosType40Generator);
  DEBUG ((DEBUG_INFO, "SMBIOS Type 40: Register Generator. Status = %r\n", Status));
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
SmbiosType40LibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterSmbiosTableGenerator (&SmbiosType40Generator);
  DEBUG ((DEBUG_INFO, "SMBIOS Type 40: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);

  return Status;
}
