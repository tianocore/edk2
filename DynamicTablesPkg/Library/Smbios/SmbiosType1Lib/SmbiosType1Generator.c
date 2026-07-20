/** @file
  SMBIOS Type 1 Table Generator.

  Copyright (c) 2024 - 2026, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
  Copyright (c) 2020 - 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SmbiosStringTableLib.h>

// Module specific include files.
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <Protocol/DynamicTableFactoryProtocol.h>
#include <Protocol/Smbios.h>
#include <IndustryStandard/SmBios.h>

#define SMBIOS_TYPE1_MAX_STRINGS  (6)

#define ALL_ONES_GUID  \
  { 0xFFFFFFFF, 0xFFFF, 0xFFFF, { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF } }

/** This macro expands to a function that retrieves the System
    information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjSystemInfo,
  CM_ARCH_COMMON_SYSTEM_INFO
  )

/**
  Validate an SMBIOS Type 1 Wake-up Type field value.

  @param [in]  WakeUpType  Wake-up Type value provided by the Configuration
                           Manager.

  @retval EFI_SUCCESS            The Wake-up Type is valid.
  @retval EFI_INVALID_PARAMETER  The Wake-up Type is invalid.
**/
STATIC
EFI_STATUS
CheckWakeUpType (
  IN  UINT8  WakeUpType
  )
{
  if ((WakeUpType == SystemWakeupTypeReserved) ||
      (WakeUpType == SystemWakeupTypeUnknown) ||
      (WakeUpType > SystemWakeupTypeAcPowerRestored))
  {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Invalid WakeUpType 0x%x\n",
      __func__,
      WakeUpType
      ));
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/** Validate SMBIOS Type 1 System Information.

  Validation follows SMBIOS Specification v3.9.0, Annex A,
  Section 4.2 (informative).

  @param [in]  SystemInfo  System Information provided by the Configuration
                           Manager.

  @retval EFI_SUCCESS            The System Information is valid.
  @retval EFI_INVALID_PARAMETER  The System Information is invalid.
**/
STATIC
EFI_STATUS
ValidateSystemInfo (
  IN CONST CM_ARCH_COMMON_SYSTEM_INFO  *SystemInfo
  )
{
  CONST GUID  AllOnesGuid = ALL_ONES_GUID;

  if ((SystemInfo->Manufacturer[0] == '\0') ||
      (SystemInfo->ProductName[0] == '\0'))
  {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Manufacturer and ProductName must be non-empty\n",
      __func__
      ));
    return EFI_INVALID_PARAMETER;
  }

  if (IsZeroGuid (&SystemInfo->Uuid) || CompareGuid (&SystemInfo->Uuid, &AllOnesGuid)) {
    DEBUG ((DEBUG_ERROR, "%a: UUID must not be all-zero or all-ones\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  return CheckWakeUpType (SystemInfo->WakeUpType);
}

/**
  Free any resources allocated when installing SMBIOS Type 1 table.

  @param [in]  This                 Pointer to the SMBIOS table generator.
  @param [in]  TableFactoryProtocol Pointer to the SMBIOS Table Factory
                                    Protocol interface.
  @param [in]  SmbiosTableInfo      Pointer to the SMBIOS table information.
  @param [in]  CfgMgrProtocol       Pointer to the Configuration Manager
                                    Protocol interface.
  @param [in]  Table                Pointer to the SMBIOS table.

  @retval EFI_SUCCESS  Table freed successfully.
**/
STATIC
EFI_STATUS
FreeSmbiosType1Table (
  IN  CONST SMBIOS_TABLE_GENERATOR                        *This,
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL  *CONST  TableFactoryProtocol,
  IN  CONST CM_STD_OBJ_SMBIOS_TABLE_INFO          *CONST  SmbiosTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN        SMBIOS_STRUCTURE                              **Table
  )
{
  if (*Table != NULL) {
    FreePool (*Table);
  }

  return EFI_SUCCESS;
}

/** Construct SMBIOS Type 1 Table describing system information.

  If this function allocates any resources then they must be freed
  in the FreeSmbiosType1Table function.

  @param [in]  This                 Pointer to the SMBIOS table generator.
  @param [in]  TableFactoryProtocol Pointer to the SMBIOS Table Factory
                                    Protocol interface.
  @param [in]  SmbiosTableInfo      Pointer to the SMBIOS table information.
  @param [in]  CfgMgrProtocol       Pointer to the Configuration Manager
                                    Protocol interface.
  @param [out] Table                Pointer to the SMBIOS table.
  @param [out] CmObjToken           Pointer to the CM Object Token.

  @retval EFI_SUCCESS            Table generated successfully.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_NOT_FOUND          Could not find information.
  @retval EFI_OUT_OF_RESOURCES   Could not allocate memory.
**/
STATIC
EFI_STATUS
BuildSmbiosType1Table (
  IN  CONST SMBIOS_TABLE_GENERATOR                        *This,
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL  *CONST  TableFactoryProtocol,
  IN        CM_STD_OBJ_SMBIOS_TABLE_INFO          *CONST  SmbiosTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       SMBIOS_STRUCTURE                              **Table,
  OUT       CM_OBJECT_TOKEN                               *CmObjToken
  )
{
  EFI_STATUS                  Status;
  CM_ARCH_COMMON_SYSTEM_INFO  *SystemInfo;
  UINT32                      SystemInfoCount;
  UINT8                       ManufacturerRef;
  UINT8                       ProductNameRef;
  UINT8                       VersionRef;
  UINT8                       SerialNumRef;
  UINT8                       SkuNumRef;
  UINT8                       FamilyRef;
  CHAR8                       *OptionalStrings;
  SMBIOS_TABLE_TYPE1          *SmbiosRecord;
  UINTN                       SmbiosRecordSize;
  STRING_TABLE                StrTable;

  SmbiosRecord = NULL;

  ASSERT (This != NULL);
  ASSERT (SmbiosTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Table != NULL);
  ASSERT (CmObjToken != NULL);
  ASSERT (SmbiosTableInfo->TableGeneratorId == This->GeneratorID);

  if ((This == NULL) ||
      (SmbiosTableInfo == NULL) ||
      (CfgMgrProtocol == NULL) ||
      (Table == NULL) ||
      (CmObjToken == NULL) ||
      (SmbiosTableInfo->TableGeneratorId != This->GeneratorID))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Parameter\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Retrieve system info from CM object
  //
  *Table = NULL;
  Status = GetEArchCommonObjSystemInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &SystemInfo,
             &SystemInfoCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get System CM Object %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  if (SystemInfoCount != 1) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Expected one System Information object, got %u\n",
      __func__,
      SystemInfoCount
      ));
    return EFI_INVALID_PARAMETER;
  }

  Status = ValidateSystemInfo (SystemInfo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Copy strings to SMBIOS table
  //
  Status = StringTableInitialize (&StrTable, SMBIOS_TYPE1_MAX_STRINGS);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to initialize string table %r\n", __func__, Status));
    return Status;
  }

  ManufacturerRef = 0;
  ProductNameRef  = 0;
  VersionRef      = 0;
  SerialNumRef    = 0;
  SkuNumRef       = 0;
  FamilyRef       = 0;

  if (SystemInfo->Manufacturer[0] != '\0') {
    Status = StringTableAddString (&StrTable, SystemInfo->Manufacturer, &ManufacturerRef);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to add Manufacturer string %r\n", __func__, Status));
      goto ErrorExit;
    }
  }

  if (SystemInfo->ProductName[0] != '\0') {
    Status = StringTableAddString (&StrTable, SystemInfo->ProductName, &ProductNameRef);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to add ProductName string %r\n", __func__, Status));
      goto ErrorExit;
    }
  }

  if (SystemInfo->Version[0] != '\0') {
    Status = StringTableAddString (&StrTable, SystemInfo->Version, &VersionRef);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to add Version string %r\n", __func__, Status));
      goto ErrorExit;
    }
  }

  if (SystemInfo->SerialNum[0] != '\0') {
    Status = StringTableAddString (&StrTable, SystemInfo->SerialNum, &SerialNumRef);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to add SerialNum string %r\n", __func__, Status));
      goto ErrorExit;
    }
  }

  if (SystemInfo->SkuNum[0] != '\0') {
    Status = StringTableAddString (&StrTable, SystemInfo->SkuNum, &SkuNumRef);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to add SkuNum string %r\n", __func__, Status));
      goto ErrorExit;
    }
  }

  if (SystemInfo->Family[0] != '\0') {
    Status = StringTableAddString (&StrTable, SystemInfo->Family, &FamilyRef);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to add Family string %r\n", __func__, Status));
      goto ErrorExit;
    }
  }

  SmbiosRecordSize = sizeof (SMBIOS_TABLE_TYPE1) +
                     StringTableGetStringSetSize (&StrTable);
  SmbiosRecord = (SMBIOS_TABLE_TYPE1 *)AllocateZeroPool (SmbiosRecordSize);
  if (SmbiosRecord == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }

  SmbiosRecord->Manufacturer = ManufacturerRef;
  SmbiosRecord->ProductName  = ProductNameRef;
  SmbiosRecord->Version      = VersionRef;
  SmbiosRecord->SerialNumber = SerialNumRef;
  SmbiosRecord->SKUNumber    = SkuNumRef;
  SmbiosRecord->Family       = FamilyRef;

  OptionalStrings = (CHAR8 *)(SmbiosRecord + 1);
  // publish the string set
  Status = StringTablePublishStringSet (
             &StrTable,
             OptionalStrings,
             (SmbiosRecordSize - sizeof (SMBIOS_TABLE_TYPE1))
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to publish string set %r\n", __func__, Status));
    goto ErrorExit;
  }

  //
  // Fill in other fields of SMBIOS table
  //
  CopyGuid (&SmbiosRecord->Uuid, &SystemInfo->Uuid);
  SmbiosRecord->WakeUpType = SystemInfo->WakeUpType;

  //
  // Setup SMBIOS header
  //
  SmbiosRecord->Hdr.Type   = EFI_SMBIOS_TYPE_SYSTEM_INFORMATION;
  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE1);

  *Table      = (SMBIOS_STRUCTURE *)SmbiosRecord;
  *CmObjToken = SystemInfo->SystemInfoToken;
  Status      = EFI_SUCCESS;

ErrorExit:
  if (EFI_ERROR (Status) && (SmbiosRecord != NULL)) {
    FreePool (SmbiosRecord);
  }

  // free string table
  StringTableFree (&StrTable);
  return Status;
}

/** The interface for the SMBIOS Type 1 Table Generator.
*/
STATIC
CONST
SMBIOS_TABLE_GENERATOR  SmbiosType1Generator = {
  // Generator ID
  CREATE_STD_SMBIOS_TABLE_GEN_ID (EStdSmbiosTableIdType01),
  // Generator Description
  L"SMBIOS.TYPE1.GENERATOR",
  // SMBIOS Table Type
  EFI_SMBIOS_TYPE_SYSTEM_INFORMATION,
  // Build table function
  BuildSmbiosType1Table,
  // Free function
  FreeSmbiosType1Table,
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
SmbiosType1LibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterSmbiosTableGenerator (&SmbiosType1Generator);
  DEBUG ((
    DEBUG_INFO,
    "SMBIOS Type 1: Register Generator. Status = %r\n",
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
SmbiosType1LibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterSmbiosTableGenerator (&SmbiosType1Generator);
  DEBUG ((
    DEBUG_INFO,
    "SMBIOS Type1: Deregister Generator. Status = %r\n",
    Status
    ));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
