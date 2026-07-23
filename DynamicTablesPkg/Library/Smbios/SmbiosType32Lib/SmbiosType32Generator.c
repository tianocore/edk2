/** @file
  SMBIOS Type 32 System Boot Information Table Generator.

  Copyright (c) 2024 - 2026, NVIDIA CORPORATION & AFFILIATES. All rights reserved.<BR>
  Copyright (c) 2020 - 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SmbiosStringTableLib.h>

// Module specific include files.
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <Protocol/DynamicTableFactoryProtocol.h>
#include <IndustryStandard/SmBios.h>

/** SMBIOS Type 32 System Boot Information Generator

Requirements:
  The following Configuration Manager Object is required by
  this Generator:
  - EArchCommonObjSystemBootInfo
*/

/** This macro expands to a function that retrieves the System Boot
    Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjSystemBootInfo,
  CM_ARCH_COMMON_SYSTEM_BOOT_INFO
  );

/** Validate SMBIOS Type 32 System Boot Information.

  Validation follows SMBIOS Specification v3.9.0, Section 7.33.1.

  @param [in] SystemBootInfo  System Boot Information CM object.

  @retval EFI_SUCCESS            The System Boot Information is valid.
  @retval EFI_INVALID_PARAMETER  The System Boot Information is invalid.
**/
STATIC
EFI_STATUS
ValidateSystemBootInfo (
  IN CONST CM_ARCH_COMMON_SYSTEM_BOOT_INFO  *SystemBootInfo
  )
{
  if (SystemBootInfo->SystemBootInfoToken == CM_NULL_TOKEN) {
    DEBUG ((DEBUG_ERROR, "%a: SystemBootInfoToken must not be null\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if ((SystemBootInfo->BootStatus >= BootInformationStatusStartReserved) &&
      (SystemBootInfo->BootStatus < BootInformationStatusStartOemSpecific))
  {
    DEBUG ((
      DEBUG_ERROR,
      "%a: BootStatus 0x%x is reserved by the SMBIOS specification\n",
      __func__,
      SystemBootInfo->BootStatus
      ));
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/** Free any resources allocated when installing SMBIOS Type 32 table.

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
FreeSmbiosType32Table (
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

/** Construct SMBIOS Type 32 System Boot Information table.

  The minimum formatted length defined by SMBIOS Specification v3.9.0,
  Section 7.33 is generated. It contains the six reserved bytes and one
  System Boot Status byte.

  If this function allocates any resources then they must be freed
  in the FreeSmbiosType32Table function.

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
BuildSmbiosType32Table (
  IN  CONST SMBIOS_TABLE_GENERATOR                         *This,
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL   *CONST  TableFactoryProtocol,
  IN        CM_STD_OBJ_SMBIOS_TABLE_INFO           *CONST  SmbiosTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  OUT       SMBIOS_STRUCTURE                               **Table,
  OUT       CM_OBJECT_TOKEN                        *CONST  CmObjectToken
  )
{
  EFI_STATUS                       Status;
  CM_ARCH_COMMON_SYSTEM_BOOT_INFO  *SystemBootInfo;
  UINT32                           SystemBootInfoCount;
  SMBIOS_TABLE_TYPE32              *SmbiosRecord;

  SystemBootInfo      = NULL;
  SystemBootInfoCount = 0;
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

  Status = GetEArchCommonObjSystemBootInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &SystemBootInfo,
             &SystemBootInfoCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get System Boot Information CM Object. Status = %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  // SMBIOS 3.9.0 Annex A, Section 4.11 requires exactly one Type 32 record.
  if ((SystemBootInfo == NULL) || (SystemBootInfoCount != 1)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Expected one System Boot Information object, got %u\n",
      __func__,
      SystemBootInfoCount
      ));
    return EFI_INVALID_PARAMETER;
  }

  Status = ValidateSystemBootInfo (SystemBootInfo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  SmbiosRecord = (SMBIOS_TABLE_TYPE32 *)AllocateSmbiosRecord (
                                           sizeof (SMBIOS_TABLE_TYPE32),
                                           NULL
                                           );
  if (SmbiosRecord == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // AllocateSmbiosRecord() zeroes the six reserved bytes required by Type 32.
  SmbiosRecord->Hdr.Type   = SMBIOS_TYPE_SYSTEM_BOOT_INFORMATION;
  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE32);
  SmbiosRecord->BootStatus = SystemBootInfo->BootStatus;

  *Table         = (SMBIOS_STRUCTURE *)SmbiosRecord;
  *CmObjectToken = SystemBootInfo->SystemBootInfoToken;

  return EFI_SUCCESS;
}

/** The SMBIOS Type 32 Table Generator.
*/
STATIC CONST SMBIOS_TABLE_GENERATOR  SmbiosType32Generator = {
  // Generator ID
  CREATE_STD_SMBIOS_TABLE_GEN_ID (EStdSmbiosTableIdType32),
  // Generator Description
  L"SMBIOS.TYPE32.GENERATOR",
  // SMBIOS structure type
  SMBIOS_TYPE_SYSTEM_BOOT_INFORMATION,
  // Build table function
  BuildSmbiosType32Table,
  // Free function
  FreeSmbiosType32Table,
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
SmbiosType32LibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterSmbiosTableGenerator (&SmbiosType32Generator);
  DEBUG ((DEBUG_INFO, "SMBIOS Type 32: Register Generator. Status = %r\n", Status));
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
SmbiosType32LibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterSmbiosTableGenerator (&SmbiosType32Generator);
  DEBUG ((DEBUG_INFO, "SMBIOS Type 32: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);

  return Status;
}
