/** @file
  SMBIOS Type 41 Onboard Devices Extended Information Table Generator.

  Copyright (c) 2024 - 2026, NVIDIA CORPORATION & AFFILIATES. All rights reserved.<BR>

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

/** SMBIOS Type 41 Onboard Devices Extended Information Generator

Requirements:
  The following Configuration Manager Object is required by this Generator:
  - EArchCommonObjOnboardDeviceInfo
*/

#define SMBIOS_TYPE41_MAX_STRINGS  1
#define SMBIOS_TYPE41_TYPE_MASK    0x7F
#define SMBIOS_TYPE41_TYPE_MIN     0x01
#define SMBIOS_TYPE41_TYPE_MAX     0x10

// SMBIOS Specification v3.9.0, Section 7.42.4 defines these values for
// devices without PCI bus/device/function information.
#define SMBIOS_TYPE41_NO_PCI_SEGMENT_GROUP    0x00FF
#define SMBIOS_TYPE41_NO_PCI_BUS              0xFF
#define SMBIOS_TYPE41_NO_PCI_DEVICE_FUNCTION  0xFF

GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjOnboardDeviceInfo,
  CM_ARCH_COMMON_ONBOARD_DEVICE_INFO
  );

/** Return whether an Onboard Device Information CM object uses the SMBIOS
    no-PCI-address values defined in SMBIOS Specification v3.9.0, Section 7.42.4.

  @param [in] OnboardDeviceInfo  Onboard Device Information to inspect.

  @retval TRUE   The object uses the SMBIOS no-PCI-address values.
  @retval FALSE  The object contains a PCI address.
**/
STATIC
BOOLEAN
HasNoPciAddress (
  IN CONST CM_ARCH_COMMON_ONBOARD_DEVICE_INFO  *OnboardDeviceInfo
  )
{
  return (BOOLEAN)(
                   (OnboardDeviceInfo->SegmentGroupNum == SMBIOS_TYPE41_NO_PCI_SEGMENT_GROUP) &&
                   (OnboardDeviceInfo->BusNum == SMBIOS_TYPE41_NO_PCI_BUS) &&
                   (OnboardDeviceInfo->DevFuncNum == SMBIOS_TYPE41_NO_PCI_DEVICE_FUNCTION)
                   );
}

/** Validate an Onboard Device Information CM object.

  @param [in] OnboardDeviceInfo  Onboard Device Information to validate.

  @retval EFI_SUCCESS            The Onboard Device Information is valid.
  @retval EFI_INVALID_PARAMETER  The Onboard Device Information is invalid.
**/
STATIC
EFI_STATUS
ValidateOnboardDeviceInfo (
  IN CONST CM_ARCH_COMMON_ONBOARD_DEVICE_INFO  *OnboardDeviceInfo
  )
{
  UINT8  DeviceType;

  DeviceType = OnboardDeviceInfo->DeviceType & SMBIOS_TYPE41_TYPE_MASK;
  if ((OnboardDeviceInfo->OnboardDeviceInfoToken == CM_NULL_TOKEN) ||
      (OnboardDeviceInfo->ReferenceDesignation[0] == '\0') ||
      (DeviceType < SMBIOS_TYPE41_TYPE_MIN) ||
      (DeviceType > SMBIOS_TYPE41_TYPE_MAX))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Onboard Device Information\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/** Construct SMBIOS Type 41 tables describing onboard devices.

  @param [in]  This                  Pointer to the SMBIOS table generator.
  @param [in]  TableFactoryProtocol  Pointer to the SMBIOS table factory.
  @param [in]  SmbiosTableInfo       Pointer to the SMBIOS table information.
  @param [in]  CfgMgrProtocol        Pointer to the Configuration Manager.
  @param [out] Table                 Pointer to the generated SMBIOS tables.
  @param [out] CmObjectToken         Pointer to the CM object tokens.
  @param [out] TableCount            Number of generated SMBIOS tables.

  @retval EFI_SUCCESS            Tables generated successfully.
  @retval EFI_INVALID_PARAMETER  A parameter or CM object is invalid.
  @retval EFI_NOT_FOUND          No Onboard Device Information objects were found.
  @retval EFI_OUT_OF_RESOURCES   Could not allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
BuildSmbiosType41TableEx (
  IN  CONST SMBIOS_TABLE_GENERATOR                         *This,
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL   *CONST  TableFactoryProtocol,
  IN        CM_STD_OBJ_SMBIOS_TABLE_INFO           *CONST  SmbiosTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  OUT       SMBIOS_STRUCTURE                               ***Table,
  OUT       CM_OBJECT_TOKEN                                **CmObjectToken,
  OUT       UINTN                                  *CONST  TableCount
  )
{
  EFI_STATUS                          Status;
  CM_ARCH_COMMON_ONBOARD_DEVICE_INFO  *OnboardDeviceInfo;
  UINT32                              OnboardDeviceCount;
  SMBIOS_STRUCTURE                    **TableList;
  CM_OBJECT_TOKEN                     *CmObjectList;
  SMBIOS_TABLE_TYPE41                 *SmbiosRecord;
  STRING_TABLE                        StrTable;
  BOOLEAN                             StringTableInitialized;
  SMBIOS_TABLE_STRING                 ReferenceDesignationRef;
  UINTN                               Index;
  UINTN                               CompareIndex;

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

  Status = GetEArchCommonObjOnboardDeviceInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &OnboardDeviceInfo,
             &OnboardDeviceCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get Onboard Device objects: %r\n", __func__, Status));
    return Status;
  }

  if (OnboardDeviceCount == 0) {
    return EFI_NOT_FOUND;
  }

  if ((OnboardDeviceInfo == NULL) ||
      (OnboardDeviceCount > (MAX_UINT32 / sizeof (*TableList))) ||
      (OnboardDeviceCount > (MAX_UINT32 / sizeof (*CmObjectList))))
  {
    return EFI_INVALID_PARAMETER;
  }

  TableList = AllocateZeroPool (sizeof (*TableList) * OnboardDeviceCount);
  if (TableList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CmObjectList = AllocateZeroPool (sizeof (*CmObjectList) * OnboardDeviceCount);
  if (CmObjectList == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }

  for (Index = 0; Index < OnboardDeviceCount; Index++) {
    Status = ValidateOnboardDeviceInfo (&OnboardDeviceInfo[Index]);
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    for (CompareIndex = 0; CompareIndex < Index; CompareIndex++) {
      if (((OnboardDeviceInfo[CompareIndex].DeviceType & SMBIOS_TYPE41_TYPE_MASK) ==
           (OnboardDeviceInfo[Index].DeviceType & SMBIOS_TYPE41_TYPE_MASK)) &&
          (OnboardDeviceInfo[CompareIndex].DeviceTypeInstance ==
           OnboardDeviceInfo[Index].DeviceTypeInstance))
      {
        DEBUG ((DEBUG_ERROR, "%a: Duplicate device type and instance at index %u\n", __func__, Index));
        Status = EFI_INVALID_PARAMETER;
        goto ErrorExit;
      }

      if (!HasNoPciAddress (&OnboardDeviceInfo[Index]) &&
          (OnboardDeviceInfo[CompareIndex].SegmentGroupNum ==
           OnboardDeviceInfo[Index].SegmentGroupNum) &&
          (OnboardDeviceInfo[CompareIndex].BusNum == OnboardDeviceInfo[Index].BusNum) &&
          (OnboardDeviceInfo[CompareIndex].DevFuncNum == OnboardDeviceInfo[Index].DevFuncNum))
      {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Onboard device objects at indices %u and %u have duplicate PCI address "
          "%04x:%02x:%02x.%x; Type 41 table generation aborted\n",
          __func__,
          CompareIndex,
          Index,
          OnboardDeviceInfo[Index].SegmentGroupNum,
          OnboardDeviceInfo[Index].BusNum,
          OnboardDeviceInfo[Index].DevFuncNum >> 3,
          OnboardDeviceInfo[Index].DevFuncNum & 0x7
          ));
        Status = EFI_INVALID_PARAMETER;
        goto ErrorExit;
      }
    }

    Status = StringTableInitialize (&StrTable, SMBIOS_TYPE41_MAX_STRINGS);
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    StringTableInitialized = TRUE;
    Status                 = StringTableAddString (
                               &StrTable,
                               OnboardDeviceInfo[Index].ReferenceDesignation,
                               &ReferenceDesignationRef
                               );
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    SmbiosRecord = AllocateSmbiosRecord (sizeof (*SmbiosRecord), &StrTable);
    if (SmbiosRecord == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ErrorExit;
    }

    SmbiosRecord->Hdr.Type             = EFI_SMBIOS_TYPE_ONBOARD_DEVICES_EXTENDED_INFORMATION;
    SmbiosRecord->Hdr.Length           = sizeof (*SmbiosRecord);
    SmbiosRecord->ReferenceDesignation = ReferenceDesignationRef;
    SmbiosRecord->DeviceType           = OnboardDeviceInfo[Index].DeviceType;
    SmbiosRecord->DeviceTypeInstance   = OnboardDeviceInfo[Index].DeviceTypeInstance;
    WriteUnaligned16 (&SmbiosRecord->SegmentGroupNum, OnboardDeviceInfo[Index].SegmentGroupNum);
    SmbiosRecord->BusNum     = OnboardDeviceInfo[Index].BusNum;
    SmbiosRecord->DevFuncNum = OnboardDeviceInfo[Index].DevFuncNum;

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
    CmObjectList[Index] = OnboardDeviceInfo[Index].OnboardDeviceInfoToken;
    SmbiosRecord        = NULL;
  }

  *Table         = TableList;
  *CmObjectToken = CmObjectList;
  *TableCount    = OnboardDeviceCount;
  return EFI_SUCCESS;

ErrorExit:
  if (StringTableInitialized) {
    StringTableFree (&StrTable);
  }

  if (SmbiosRecord != NULL) {
    FreePool (SmbiosRecord);
  }

  if (TableList != NULL) {
    for (Index = 0; Index < OnboardDeviceCount; Index++) {
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

/** Free resources allocated when installing SMBIOS Type 41 tables.

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
FreeSmbiosType41TableEx (
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

STATIC CONST SMBIOS_TABLE_GENERATOR  SmbiosType41Generator = {
  CREATE_STD_SMBIOS_TABLE_GEN_ID (EStdSmbiosTableIdType41),
  L"SMBIOS.TYPE41.GENERATOR",
  EFI_SMBIOS_TYPE_ONBOARD_DEVICES_EXTENDED_INFORMATION,
  NULL,
  NULL,
  BuildSmbiosType41TableEx,
  FreeSmbiosType41TableEx
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
SmbiosType41LibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterSmbiosTableGenerator (&SmbiosType41Generator);
  DEBUG ((DEBUG_INFO, "SMBIOS Type 41: Register Generator. Status = %r\n", Status));
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
SmbiosType41LibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterSmbiosTableGenerator (&SmbiosType41Generator);
  DEBUG ((DEBUG_INFO, "SMBIOS Type 41: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
