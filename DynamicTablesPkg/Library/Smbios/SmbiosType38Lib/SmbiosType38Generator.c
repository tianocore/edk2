/** @file
  SMBIOS Type 38 IPMI Device Information Table Generator.

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

/** SMBIOS Type 38 IPMI Device Information Generator

Requirements:
  The following Configuration Manager Object is required by this Generator:
  - EArchCommonObjIpmiDeviceInfo
*/

#define SMBIOS_TYPE38_MODIFIER_RESERVED_MASK  (BIT5 | BIT2)
#define SMBIOS_TYPE38_REGISTER_SPACING_MASK   (BIT7 | BIT6)

GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjIpmiDeviceInfo,
  CM_ARCH_COMMON_IPMI_DEVICE_INFO
  );

/** Validate an IPMI Device Information CM object.

  Validation follows SMBIOS Specification v3.9.0, Section 7.39.

  @param [in] IpmiDeviceInfo  IPMI Device Information to validate.

  @retval EFI_SUCCESS            The IPMI Device Information is valid.
  @retval EFI_INVALID_PARAMETER  The IPMI Device Information is invalid.
**/
STATIC
EFI_STATUS
ValidateIpmiDeviceInfo (
  IN CONST CM_ARCH_COMMON_IPMI_DEVICE_INFO  *IpmiDeviceInfo
  )
{
  if ((IpmiDeviceInfo->IpmiDeviceInfoToken == CM_NULL_TOKEN) ||
      (IpmiDeviceInfo->InterfaceType > IPMIDeviceInfoInterfaceTypeSSIF) ||
      ((IpmiDeviceInfo->IpmiSpecificationRevision & 0x0F) > 9) ||
      (((IpmiDeviceInfo->IpmiSpecificationRevision >> 4) & 0x0F) > 9) ||
      ((IpmiDeviceInfo->BaseAddressModifierInterruptInfo &
        SMBIOS_TYPE38_MODIFIER_RESERVED_MASK) != 0) ||
      ((IpmiDeviceInfo->BaseAddressModifierInterruptInfo &
        SMBIOS_TYPE38_REGISTER_SPACING_MASK) == SMBIOS_TYPE38_REGISTER_SPACING_MASK))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid IPMI Device Information\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if ((IpmiDeviceInfo->InterfaceType == IPMIDeviceInfoInterfaceTypeSSIF) &&
      (IpmiDeviceInfo->BaseAddressModifierInterruptInfo != 0))
  {
    DEBUG ((DEBUG_ERROR, "%a: Base address modifier must be zero for SSIF\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/** Construct SMBIOS Type 38 tables describing IPMI devices.

  @param [in]  This                  Pointer to the SMBIOS table generator.
  @param [in]  TableFactoryProtocol  Pointer to the SMBIOS table factory.
  @param [in]  SmbiosTableInfo       Pointer to the SMBIOS table information.
  @param [in]  CfgMgrProtocol        Pointer to the Configuration Manager.
  @param [out] Table                 Pointer to the generated SMBIOS tables.
  @param [out] CmObjectToken         Pointer to the CM object tokens.
  @param [out] TableCount            Number of generated SMBIOS tables.

  @retval EFI_SUCCESS            Tables generated successfully.
  @retval EFI_INVALID_PARAMETER  A parameter or CM object is invalid.
  @retval EFI_NOT_FOUND          No IPMI Device Information objects were found.
  @retval EFI_OUT_OF_RESOURCES   Could not allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
BuildSmbiosType38TableEx (
  IN  CONST SMBIOS_TABLE_GENERATOR                         *This,
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL   *CONST  TableFactoryProtocol,
  IN        CM_STD_OBJ_SMBIOS_TABLE_INFO           *CONST  SmbiosTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  OUT       SMBIOS_STRUCTURE                               ***Table,
  OUT       CM_OBJECT_TOKEN                                **CmObjectToken,
  OUT       UINTN                                  *CONST  TableCount
  )
{
  EFI_STATUS                       Status;
  CM_ARCH_COMMON_IPMI_DEVICE_INFO  *IpmiDeviceInfo;
  UINT32                           IpmiDeviceCount;
  SMBIOS_STRUCTURE                 **TableList;
  CM_OBJECT_TOKEN                  *CmObjectList;
  SMBIOS_TABLE_TYPE38              *SmbiosRecord;
  UINTN                            Index;

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

  Status = GetEArchCommonObjIpmiDeviceInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &IpmiDeviceInfo,
             &IpmiDeviceCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get IPMI Device objects: %r\n", __func__, Status));
    return Status;
  }

  if (IpmiDeviceCount == 0) {
    return EFI_NOT_FOUND;
  }

  if ((IpmiDeviceInfo == NULL) ||
      (IpmiDeviceCount > (MAX_UINT32 / sizeof (*TableList))) ||
      (IpmiDeviceCount > (MAX_UINT32 / sizeof (*CmObjectList))))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid IPMI Device object list\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  TableList = AllocateZeroPool (sizeof (*TableList) * IpmiDeviceCount);
  if (TableList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CmObjectList = AllocateZeroPool (sizeof (*CmObjectList) * IpmiDeviceCount);
  if (CmObjectList == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }

  for (Index = 0; Index < IpmiDeviceCount; Index++) {
    Status = ValidateIpmiDeviceInfo (&IpmiDeviceInfo[Index]);
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    SmbiosRecord = AllocateSmbiosRecord (sizeof (*SmbiosRecord), NULL);
    if (SmbiosRecord == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ErrorExit;
    }

    SmbiosRecord->Hdr.Type                  = SMBIOS_TYPE_IPMI_DEVICE_INFORMATION;
    SmbiosRecord->Hdr.Length                = sizeof (*SmbiosRecord);
    SmbiosRecord->InterfaceType             = IpmiDeviceInfo[Index].InterfaceType;
    SmbiosRecord->IPMISpecificationRevision = IpmiDeviceInfo[Index].IpmiSpecificationRevision;
    SmbiosRecord->I2CSlaveAddress           = IpmiDeviceInfo[Index].I2cTargetAddress;
    SmbiosRecord->NVStorageDeviceAddress    = IpmiDeviceInfo[Index].NvStorageDeviceAddress;
    WriteUnaligned64 (&SmbiosRecord->BaseAddress, IpmiDeviceInfo[Index].BaseAddress);
    SmbiosRecord->BaseAddressModifier_InterruptInfo =
      IpmiDeviceInfo[Index].BaseAddressModifierInterruptInfo;
    SmbiosRecord->InterruptNumber = IpmiDeviceInfo[Index].InterruptNumber;

    TableList[Index]    = (SMBIOS_STRUCTURE *)SmbiosRecord;
    CmObjectList[Index] = IpmiDeviceInfo[Index].IpmiDeviceInfoToken;
    SmbiosRecord        = NULL;
  }

  *Table         = TableList;
  *CmObjectToken = CmObjectList;
  *TableCount    = IpmiDeviceCount;
  return EFI_SUCCESS;

ErrorExit:
  if (SmbiosRecord != NULL) {
    FreePool (SmbiosRecord);
  }

  if (TableList != NULL) {
    for (Index = 0; Index < IpmiDeviceCount; Index++) {
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

/** Free resources allocated when installing SMBIOS Type 38 tables.

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
FreeSmbiosType38TableEx (
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

STATIC CONST SMBIOS_TABLE_GENERATOR  SmbiosType38Generator = {
  CREATE_STD_SMBIOS_TABLE_GEN_ID (EStdSmbiosTableIdType38),
  L"SMBIOS.TYPE38.GENERATOR",
  EFI_SMBIOS_TYPE_IPMI_DEVICE_INFORMATION,
  NULL,
  NULL,
  BuildSmbiosType38TableEx,
  FreeSmbiosType38TableEx
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
SmbiosType38LibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterSmbiosTableGenerator (&SmbiosType38Generator);
  DEBUG ((DEBUG_INFO, "SMBIOS Type 38: Register Generator. Status = %r\n", Status));
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
SmbiosType38LibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterSmbiosTableGenerator (&SmbiosType38Generator);
  DEBUG ((DEBUG_INFO, "SMBIOS Type 38: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
