/** @file
  SMBIOS Type 43 TPM Device Table Generator.

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

/** SMBIOS Type 43 TPM Device Generator

Requirements:
  The following Configuration Manager Object is required by this Generator:
  - EArchCommonObjTpmDeviceInfo
*/

#define SMBIOS_TYPE43_MAX_STRINGS           1
#define SMBIOS_TYPE43_CHARACTERISTICS_MASK  (BIT2 | BIT3 | BIT4 | BIT5)

GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjTpmDeviceInfo,
  CM_ARCH_COMMON_TPM_DEVICE_INFO
  );

/** Return whether a TPM Vendor ID uses the encoding defined by SMBIOS.

  @param [in] VendorId  Four-byte TCG Vendor ID to validate.

  @retval TRUE   The Vendor ID is valid.
  @retval FALSE  The Vendor ID is invalid.
**/
STATIC
BOOLEAN
IsVendorIdValid (
  IN CONST UINT8  VendorId[4]
  )
{
  UINTN  Index;

  for (Index = 0; Index < 3; Index++) {
    if ((VendorId[Index] < 0x20) || (VendorId[Index] > 0x7E)) {
      return FALSE;
    }
  }

  return (VendorId[3] == 0) ||
         ((VendorId[3] >= 0x20) && (VendorId[3] <= 0x7E));
}

/** Validate a TPM Device Information CM object.

  Validation follows SMBIOS Specification v3.9.0, Section 7.44.

  @param [in] TpmDeviceInfo  TPM Device Information to validate.

  @retval EFI_SUCCESS            The TPM Device Information is valid.
  @retval EFI_INVALID_PARAMETER  The TPM Device Information is invalid.
**/
STATIC
EFI_STATUS
ValidateTpmDeviceInfo (
  IN CONST CM_ARCH_COMMON_TPM_DEVICE_INFO  *TpmDeviceInfo
  )
{
  if ((TpmDeviceInfo->TpmDeviceInfoToken == CM_NULL_TOKEN) ||
      !IsVendorIdValid (TpmDeviceInfo->VendorId) ||
      ((TpmDeviceInfo->Characteristics & ~SMBIOS_TYPE43_CHARACTERISTICS_MASK) != 0) ||
      ((TpmDeviceInfo->MajorSpecVersion == 1) &&
       (TpmDeviceInfo->FirmwareVersion2 != 0)))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid TPM Device Information\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/** Construct SMBIOS Type 43 tables describing TPM devices.

  @param [in]  This                  Pointer to the SMBIOS table generator.
  @param [in]  TableFactoryProtocol  Pointer to the SMBIOS table factory.
  @param [in]  SmbiosTableInfo       Pointer to the SMBIOS table information.
  @param [in]  CfgMgrProtocol        Pointer to the Configuration Manager.
  @param [out] Table                 Pointer to the generated SMBIOS tables.
  @param [out] CmObjectToken         Pointer to the CM object tokens.
  @param [out] TableCount            Number of generated SMBIOS tables.

  @retval EFI_SUCCESS            Tables generated successfully.
  @retval EFI_INVALID_PARAMETER  A parameter or CM object is invalid.
  @retval EFI_NOT_FOUND          No TPM Device Information objects were found.
  @retval EFI_OUT_OF_RESOURCES   Could not allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
BuildSmbiosType43TableEx (
  IN  CONST SMBIOS_TABLE_GENERATOR                         *This,
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL   *CONST  TableFactoryProtocol,
  IN        CM_STD_OBJ_SMBIOS_TABLE_INFO           *CONST  SmbiosTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  OUT       SMBIOS_STRUCTURE                               ***Table,
  OUT       CM_OBJECT_TOKEN                                **CmObjectToken,
  OUT       UINTN                                  *CONST  TableCount
  )
{
  EFI_STATUS                      Status;
  CM_ARCH_COMMON_TPM_DEVICE_INFO  *TpmDeviceInfo;
  UINT32                          TpmDeviceCount;
  SMBIOS_STRUCTURE                **TableList;
  CM_OBJECT_TOKEN                 *CmObjectList;
  SMBIOS_TABLE_TYPE43             *SmbiosRecord;
  STRING_TABLE                    StrTable;
  BOOLEAN                         StringTableInitialized;
  SMBIOS_TABLE_STRING             DescriptionRef;
  UINTN                           Index;

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

  Status = GetEArchCommonObjTpmDeviceInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &TpmDeviceInfo,
             &TpmDeviceCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get TPM Device objects: %r\n", __func__, Status));
    return Status;
  }

  if (TpmDeviceCount == 0) {
    return EFI_NOT_FOUND;
  }

  if ((TpmDeviceInfo == NULL) ||
      (TpmDeviceCount > (MAX_UINT32 / sizeof (*TableList))) ||
      (TpmDeviceCount > (MAX_UINT32 / sizeof (*CmObjectList))))
  {
    return EFI_INVALID_PARAMETER;
  }

  TableList = AllocateZeroPool (sizeof (*TableList) * TpmDeviceCount);
  if (TableList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CmObjectList = AllocateZeroPool (sizeof (*CmObjectList) * TpmDeviceCount);
  if (CmObjectList == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }

  for (Index = 0; Index < TpmDeviceCount; Index++) {
    Status = ValidateTpmDeviceInfo (&TpmDeviceInfo[Index]);
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    Status = StringTableInitialize (&StrTable, SMBIOS_TYPE43_MAX_STRINGS);
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    StringTableInitialized = TRUE;
    DescriptionRef         = 0;
    if (TpmDeviceInfo[Index].Description[0] != '\0') {
      Status = StringTableAddString (
                 &StrTable,
                 TpmDeviceInfo[Index].Description,
                 &DescriptionRef
                 );
      if (EFI_ERROR (Status)) {
        goto ErrorExit;
      }
    }

    SmbiosRecord = AllocateSmbiosRecord (sizeof (*SmbiosRecord), &StrTable);
    if (SmbiosRecord == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ErrorExit;
    }

    SmbiosRecord->Hdr.Type   = SMBIOS_TYPE_TPM_DEVICE;
    SmbiosRecord->Hdr.Length = sizeof (*SmbiosRecord);
    CopyMem (SmbiosRecord->VendorID, TpmDeviceInfo[Index].VendorId, sizeof (SmbiosRecord->VendorID));
    SmbiosRecord->MajorSpecVersion = TpmDeviceInfo[Index].MajorSpecVersion;
    SmbiosRecord->MinorSpecVersion = TpmDeviceInfo[Index].MinorSpecVersion;
    WriteUnaligned32 (&SmbiosRecord->FirmwareVersion1, TpmDeviceInfo[Index].FirmwareVersion1);
    WriteUnaligned32 (&SmbiosRecord->FirmwareVersion2, TpmDeviceInfo[Index].FirmwareVersion2);
    SmbiosRecord->Description = DescriptionRef;
    WriteUnaligned64 (&SmbiosRecord->Characteristics, TpmDeviceInfo[Index].Characteristics);
    WriteUnaligned32 (&SmbiosRecord->OemDefined, TpmDeviceInfo[Index].OemDefined);

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
    CmObjectList[Index] = TpmDeviceInfo[Index].TpmDeviceInfoToken;
    SmbiosRecord        = NULL;
  }

  *Table         = TableList;
  *CmObjectToken = CmObjectList;
  *TableCount    = TpmDeviceCount;
  return EFI_SUCCESS;

ErrorExit:
  if (StringTableInitialized) {
    StringTableFree (&StrTable);
  }

  if (SmbiosRecord != NULL) {
    FreePool (SmbiosRecord);
  }

  if (TableList != NULL) {
    for (Index = 0; Index < TpmDeviceCount; Index++) {
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

/** Free resources allocated when installing SMBIOS Type 43 tables.

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
FreeSmbiosType43TableEx (
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

STATIC CONST SMBIOS_TABLE_GENERATOR  SmbiosType43Generator = {
  CREATE_STD_SMBIOS_TABLE_GEN_ID (EStdSmbiosTableIdType43),
  L"SMBIOS.TYPE43.GENERATOR",
  SMBIOS_TYPE_TPM_DEVICE,
  NULL,
  NULL,
  BuildSmbiosType43TableEx,
  FreeSmbiosType43TableEx
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
SmbiosType43LibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterSmbiosTableGenerator (&SmbiosType43Generator);
  DEBUG ((DEBUG_INFO, "SMBIOS Type 43: Register Generator. Status = %r\n", Status));
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
SmbiosType43LibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterSmbiosTableGenerator (&SmbiosType43Generator);
  DEBUG ((DEBUG_INFO, "SMBIOS Type 43: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
