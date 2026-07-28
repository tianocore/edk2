/** @file
  SMBIOS Type 3 System Enclosure Table Generator.

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

#define SMBIOS_TYPE3_MAX_STRINGS                      5
#define SMBIOS_TYPE3_CHASSIS_TYPE_MASK                0x7F
#define SMBIOS_TYPE3_CONTAINED_ELEMENT_TYPE_SELECT    BIT7
#define SMBIOS_TYPE3_CONTAINED_ELEMENT_TYPE_MASK      0x7F
#define SMBIOS_TYPE3_CONTAINED_ELEMENT_RECORD_LENGTH  3
#define SMBIOS_TYPE3_TRAILING_FIELD_LENGTH            3
#define SMBIOS_TYPE3_BASE_LENGTH                      \
  (OFFSET_OF (SMBIOS_TABLE_TYPE3, ContainedElements) + SMBIOS_TYPE3_TRAILING_FIELD_LENGTH)

GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjSystemEnclosureInfo,
  CM_ARCH_COMMON_SYSTEM_ENCLOSURE_INFO
  );

GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjEnclosureElement,
  CM_ARCH_COMMON_ENCLOSURE_ELEMENT
  );

/** Validate System Enclosure Contained Element CM objects.

  @param [in]  ContainedElement       Contained Element objects to validate.
  @param [in]  ContainedElementCount  Number of Contained Element objects.

  @retval EFI_SUCCESS            The Contained Element objects are valid.
  @retval EFI_INVALID_PARAMETER  A Contained Element object is invalid.
**/
STATIC
EFI_STATUS
ValidateSystemEnclosureContainedElements (
  IN CONST CM_ARCH_COMMON_ENCLOSURE_ELEMENT  *ContainedElement,
  IN       UINT32                            ContainedElementCount
  )
{
  UINT32  Index;
  UINT8   ElementType;

  for (Index = 0; Index < ContainedElementCount; Index++) {
    ElementType = ContainedElement[Index].ContainedElementType &
                  SMBIOS_TYPE3_CONTAINED_ELEMENT_TYPE_MASK;

    if (((ContainedElement[Index].ContainedElementType &
          SMBIOS_TYPE3_CONTAINED_ELEMENT_TYPE_SELECT) == 0) &&
        ((ElementType < BaseBoardTypeUnknown) ||
         (ElementType > BaseBoardTypeInterconnectBoard)))
    {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Invalid Baseboard Type 0x%x for Contained Element %u\n",
        __func__,
        ElementType,
        Index
        ));
      return EFI_INVALID_PARAMETER;
    }

    // When bit 7 is set, bits 6:0 identify an SMBIOS structure type.
    // Type 127 is the end-of-table marker and cannot represent a device.
    if (((ContainedElement[Index].ContainedElementType &
          SMBIOS_TYPE3_CONTAINED_ELEMENT_TYPE_SELECT) != 0) &&
        (ElementType > EFI_SMBIOS_TYPE_INACTIVE))
    {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Invalid SMBIOS Type 0x%x for Contained Element %u\n",
        __func__,
        ElementType,
        Index
        ));
      return EFI_INVALID_PARAMETER;
    }

    if ((ContainedElement[Index].ContainedElementMinimum == MAX_UINT8) ||
        (ContainedElement[Index].ContainedElementMaximum == 0) ||
        (ContainedElement[Index].ContainedElementMinimum >
         ContainedElement[Index].ContainedElementMaximum))
    {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Invalid quantity range %u-%u for Contained Element %u\n",
        __func__,
        ContainedElement[Index].ContainedElementMinimum,
        ContainedElement[Index].ContainedElementMaximum,
        Index
        ));
      return EFI_INVALID_PARAMETER;
    }
  }

  return EFI_SUCCESS;
}

/** Validate a System Enclosure CM object.

  @param [in]  EnclosureInfo  System Enclosure information to validate.

  @retval EFI_SUCCESS            The System Enclosure information is valid.
  @retval EFI_INVALID_PARAMETER  The System Enclosure information is invalid.
**/
STATIC
EFI_STATUS
ValidateSystemEnclosureInfo (
  IN CONST CM_ARCH_COMMON_SYSTEM_ENCLOSURE_INFO  *EnclosureInfo
  )
{
  UINT8  ChassisType;

  if (EnclosureInfo->Manufacturer[0] == '\0') {
    DEBUG ((DEBUG_ERROR, "%a: Manufacturer must be non-empty\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  ChassisType = EnclosureInfo->Type & SMBIOS_TYPE3_CHASSIS_TYPE_MASK;
  if ((ChassisType < MiscChassisTypeOther) ||
      (ChassisType == MiscChassisTypeUnknown) ||
      (ChassisType > MiscChassisStickPc))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid chassis Type 0x%x\n", __func__, EnclosureInfo->Type));
    return EFI_INVALID_PARAMETER;
  }

  if ((EnclosureInfo->BootUpState < ChassisStateOther) ||
      (EnclosureInfo->BootUpState > ChassisStateNonRecoverable) ||
      (EnclosureInfo->PowerSupplyState < ChassisStateOther) ||
      (EnclosureInfo->PowerSupplyState > ChassisStateNonRecoverable) ||
      (EnclosureInfo->ThermalState < ChassisStateOther) ||
      (EnclosureInfo->ThermalState > ChassisStateNonRecoverable))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid chassis state\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if ((EnclosureInfo->SecurityStatus < ChassisSecurityStatusOther) ||
      (EnclosureInfo->SecurityStatus > ChassisSecurityStatusExternalInterfaceLockedEnabled))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid SecurityStatus 0x%x\n", __func__, EnclosureInfo->SecurityStatus));
    return EFI_INVALID_PARAMETER;
  }

  if (EnclosureInfo->RackType > ChassisRackTypeOU) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid RackType 0x%x\n", __func__, EnclosureInfo->RackType));
    return EFI_INVALID_PARAMETER;
  }

  if ((EnclosureInfo->Height == ChassisHeightUseRackHeight) &&
      (EnclosureInfo->RackHeight == 0))
  {
    DEBUG ((DEBUG_ERROR, "%a: RackHeight must be non-zero when Height is 0xFF\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/** Construct SMBIOS Type 3 tables describing system enclosures.

  @param [in]  This                  Pointer to the SMBIOS table generator.
  @param [in]  TableFactoryProtocol  Pointer to the SMBIOS table factory.
  @param [in]  SmbiosTableInfo       Pointer to the SMBIOS table information.
  @param [in]  CfgMgrProtocol        Pointer to the Configuration Manager.
  @param [out] Table                 Pointer to the generated SMBIOS tables.
  @param [out] CmObjectToken         Pointer to the CM object tokens.
  @param [out] TableCount            Number of generated SMBIOS tables.

  @retval EFI_SUCCESS            Tables generated successfully.
  @retval EFI_INVALID_PARAMETER  A parameter or CM object is invalid.
  @retval EFI_NOT_FOUND          No System Enclosure objects were found.
  @retval EFI_OUT_OF_RESOURCES   Could not allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
BuildSmbiosType3TableEx (
  IN  CONST SMBIOS_TABLE_GENERATOR                         *This,
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL   *CONST  TableFactoryProtocol,
  IN        CM_STD_OBJ_SMBIOS_TABLE_INFO           *CONST  SmbiosTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  OUT       SMBIOS_STRUCTURE                               ***Table,
  OUT       CM_OBJECT_TOKEN                                **CmObjectToken,
  OUT       UINTN                                  *CONST  TableCount
  )
{
  EFI_STATUS                            Status;
  CM_ARCH_COMMON_SYSTEM_ENCLOSURE_INFO  *EnclosureInfo;
  CM_ARCH_COMMON_ENCLOSURE_ELEMENT      *ContainedElement;
  UINT32                                EnclosureCount;
  UINT32                                ContainedElementCount;
  SMBIOS_STRUCTURE                      **TableList;
  CM_OBJECT_TOKEN                       *CmObjectList;
  SMBIOS_TABLE_TYPE3                    *SmbiosRecord;
  CONTAINED_ELEMENT                     *ContainedElementField;
  STRING_TABLE                          StrTable;
  SMBIOS_TABLE_STRING                   ManufacturerRef;
  SMBIOS_TABLE_STRING                   VersionRef;
  SMBIOS_TABLE_STRING                   SerialNumRef;
  SMBIOS_TABLE_STRING                   AssetTagRef;
  SMBIOS_TABLE_STRING                   SkuNumRef;
  SMBIOS_TABLE_STRING                   *SkuNumField;
  UINT8                                 *RackTypeField;
  UINT8                                 *RackHeightField;
  UINTN                                 FormattedLength;
  UINTN                                 RecordSize;
  UINTN                                 Index;
  UINTN                                 ContainedElementIndex;

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

  *Table         = NULL;
  *CmObjectToken = NULL;
  *TableCount    = 0;
  TableList      = NULL;
  CmObjectList   = NULL;
  SmbiosRecord   = NULL;

  Status = GetEArchCommonObjSystemEnclosureInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &EnclosureInfo,
             &EnclosureCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get System Enclosure objects: %r\n", __func__, Status));
    return Status;
  }

  if (EnclosureCount == 0) {
    return EFI_NOT_FOUND;
  }

  TableList = AllocateZeroPool (sizeof (*TableList) * EnclosureCount);
  if (TableList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CmObjectList = AllocateZeroPool (sizeof (*CmObjectList) * EnclosureCount);
  if (CmObjectList == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }

  for (Index = 0; Index < EnclosureCount; Index++) {
    Status = ValidateSystemEnclosureInfo (&EnclosureInfo[Index]);
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    ContainedElement      = NULL;
    ContainedElementCount = 0;
    if (EnclosureInfo[Index].ContainedElementListToken != CM_NULL_TOKEN) {
      Status = GetEArchCommonObjEnclosureElement (
                 CfgMgrProtocol,
                 EnclosureInfo[Index].ContainedElementListToken,
                 &ContainedElement,
                 &ContainedElementCount
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to get Contained Elements for enclosure %u: %r\n",
          __func__,
          Index,
          Status
          ));
        goto ErrorExit;
      }

      if ((ContainedElementCount != 0) && (ContainedElement == NULL)) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Contained Element list is NULL for enclosure %u\n",
          __func__,
          Index
          ));
        Status = EFI_INVALID_PARAMETER;
        goto ErrorExit;
      }

      if (ContainedElementCount > MAX_UINT8) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Too many Contained Elements for enclosure %u: %u\n",
          __func__,
          Index,
          ContainedElementCount
          ));
        Status = EFI_INVALID_PARAMETER;
        goto ErrorExit;
      }

      Status = ValidateSystemEnclosureContainedElements (
                 ContainedElement,
                 ContainedElementCount
                 );
      if (EFI_ERROR (Status)) {
        goto ErrorExit;
      }
    }

    FormattedLength = SMBIOS_TYPE3_BASE_LENGTH +
                      (ContainedElementCount * SMBIOS_TYPE3_CONTAINED_ELEMENT_RECORD_LENGTH);
    if (FormattedLength > MAX_UINT8) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Formatted length %Lu exceeds Hdr.Length for enclosure %u\n",
        __func__,
        (UINT64)FormattedLength,
        Index
        ));
      Status = EFI_INVALID_PARAMETER;
      goto ErrorExit;
    }

    Status = StringTableInitialize (&StrTable, SMBIOS_TYPE3_MAX_STRINGS);
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    ManufacturerRef = 0;
    VersionRef      = 0;
    SerialNumRef    = 0;
    AssetTagRef     = 0;
    SkuNumRef       = 0;

    Status = StringTableAddString (&StrTable, EnclosureInfo[Index].Manufacturer, &ManufacturerRef);
    if (EFI_ERROR (Status)) {
      StringTableFree (&StrTable);
      goto ErrorExit;
    }

    if (EnclosureInfo[Index].Version[0] != '\0') {
      Status = StringTableAddString (&StrTable, EnclosureInfo[Index].Version, &VersionRef);
    }

    if (!EFI_ERROR (Status) && (EnclosureInfo[Index].SerialNum[0] != '\0')) {
      Status = StringTableAddString (&StrTable, EnclosureInfo[Index].SerialNum, &SerialNumRef);
    }

    if (!EFI_ERROR (Status) && (EnclosureInfo[Index].AssetTag[0] != '\0')) {
      Status = StringTableAddString (&StrTable, EnclosureInfo[Index].AssetTag, &AssetTagRef);
    }

    if (!EFI_ERROR (Status) && (EnclosureInfo[Index].SkuNum[0] != '\0')) {
      Status = StringTableAddString (&StrTable, EnclosureInfo[Index].SkuNum, &SkuNumRef);
    }

    if (EFI_ERROR (Status)) {
      StringTableFree (&StrTable);
      goto ErrorExit;
    }

    RecordSize   = FormattedLength + StringTableGetStringSetSize (&StrTable);
    SmbiosRecord = AllocateZeroPool (RecordSize);
    if (SmbiosRecord == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      StringTableFree (&StrTable);
      goto ErrorExit;
    }

    SmbiosRecord->Hdr.Type                     = EFI_SMBIOS_TYPE_SYSTEM_ENCLOSURE;
    SmbiosRecord->Hdr.Length                   = (UINT8)FormattedLength;
    SmbiosRecord->Manufacturer                 = ManufacturerRef;
    SmbiosRecord->Type                         = EnclosureInfo[Index].Type;
    SmbiosRecord->Version                      = VersionRef;
    SmbiosRecord->SerialNumber                 = SerialNumRef;
    SmbiosRecord->AssetTag                     = AssetTagRef;
    SmbiosRecord->BootupState                  = EnclosureInfo[Index].BootUpState;
    SmbiosRecord->PowerSupplyState             = EnclosureInfo[Index].PowerSupplyState;
    SmbiosRecord->ThermalState                 = EnclosureInfo[Index].ThermalState;
    SmbiosRecord->SecurityStatus               = EnclosureInfo[Index].SecurityStatus;
    SmbiosRecord->Height                       = EnclosureInfo[Index].Height;
    SmbiosRecord->NumberofPowerCords           = EnclosureInfo[Index].NumberOfPowerCords;
    SmbiosRecord->ContainedElementCount        = (UINT8)ContainedElementCount;
    SmbiosRecord->ContainedElementRecordLength =
      (ContainedElementCount == 0) ? 0 : SMBIOS_TYPE3_CONTAINED_ELEMENT_RECORD_LENGTH;
    WriteUnaligned32 ((UINT32 *)SmbiosRecord->OemDefined, EnclosureInfo[Index].OemDefined);

    ContainedElementField = (CONTAINED_ELEMENT *)((UINT8 *)SmbiosRecord +
                                                  OFFSET_OF (SMBIOS_TABLE_TYPE3, ContainedElements));
    if (ContainedElement != NULL) {
      for (ContainedElementIndex = 0;
           ContainedElementIndex < ContainedElementCount;
           ContainedElementIndex++)
      {
        ContainedElementField[ContainedElementIndex].ContainedElementType =
          ContainedElement[ContainedElementIndex].ContainedElementType;
        ContainedElementField[ContainedElementIndex].ContainedElementMinimum =
          ContainedElement[ContainedElementIndex].ContainedElementMinimum;
        ContainedElementField[ContainedElementIndex].ContainedElementMaximum =
          ContainedElement[ContainedElementIndex].ContainedElementMaximum;
      }
    }

    SkuNumField = (SMBIOS_TABLE_STRING *)((UINT8 *)ContainedElementField +
                                          (ContainedElementCount *
                                           SMBIOS_TYPE3_CONTAINED_ELEMENT_RECORD_LENGTH));
    RackTypeField    = (UINT8 *)(SkuNumField + 1);
    RackHeightField  = RackTypeField + 1;
    *SkuNumField     = SkuNumRef;
    *RackTypeField   = EnclosureInfo[Index].RackType;
    *RackHeightField = EnclosureInfo[Index].RackHeight;

    Status = StringTablePublishStringSet (
               &StrTable,
               (CHAR8 *)SmbiosRecord + FormattedLength,
               RecordSize - FormattedLength
               );
    StringTableFree (&StrTable);
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    TableList[Index]    = (SMBIOS_STRUCTURE *)SmbiosRecord;
    CmObjectList[Index] = EnclosureInfo[Index].SystemEnclosureToken;
    SmbiosRecord        = NULL;
  }

  *Table         = TableList;
  *CmObjectToken = CmObjectList;
  *TableCount    = EnclosureCount;
  return EFI_SUCCESS;

ErrorExit:
  if (SmbiosRecord != NULL) {
    FreePool (SmbiosRecord);
  }

  if (TableList != NULL) {
    for (Index = 0; Index < EnclosureCount; Index++) {
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

/**
  Free any resources allocated when installing SMBIOS Type 3 tables.

  @param [in]  This                 Pointer to the SMBIOS table generator.
  @param [in]  TableFactoryProtocol Pointer to the SMBIOS Table Factory
                                    Protocol interface.
  @param [in]  SmbiosTableInfo      Pointer to the SMBIOS table information.
  @param [in]  CfgMgrProtocol       Pointer to the Configuration Manager
                                    Protocol interface.
  @param [in]  Table                Pointer to the SMBIOS tables.
  @param [in]  CmObjectToken        Pointer to the CM ObjectToken array.
  @param [in]  TableCount           Number of SMBIOS tables.

  @retval EFI_SUCCESS           Resources freed successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
FreeSmbiosType3TableEx (
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

STATIC CONST SMBIOS_TABLE_GENERATOR  SmbiosType3Generator = {
  // Generator ID
  CREATE_STD_SMBIOS_TABLE_GEN_ID (EStdSmbiosTableIdType03),
  // Generator Description
  L"SMBIOS.TYPE3.GENERATOR",
  // SMBIOS Table Type
  EFI_SMBIOS_TYPE_SYSTEM_ENCLOSURE,
  NULL,
  NULL,
  // Build table function Extended.
  BuildSmbiosType3TableEx,
  // Free function Extended.
  FreeSmbiosType3TableEx
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
SmbiosType3LibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterSmbiosTableGenerator (&SmbiosType3Generator);
  DEBUG ((DEBUG_INFO, "SMBIOS Type 3: Register Generator. Status = %r\n", Status));
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
SmbiosType3LibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterSmbiosTableGenerator (&SmbiosType3Generator);
  DEBUG ((DEBUG_INFO, "SMBIOS Type 3: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
