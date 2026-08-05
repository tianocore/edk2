/** @file
  SMBIOS Type23 Table Generator.

  @par Reference(s):
  - SMBIOS Specification 3.9.0

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>

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

/** SMBIOS Type 23 System Reset Generator

Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - EArchCommonObjSystemResetInfo
*/

/**
  This macro expands to a function that retrieves the System Reset
  information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjSystemResetInfo,
  CM_ARCH_COMMON_SYSTEM_RESET_INFO
  );

/**
  SMBIOS Type 23 Capabilities bits 7:6 are reserved and must be 00b.
*/
#define SMBIOS_TYPE23_CAPABILITIES_RESERVED_MASK  0xC0

/**
  Free any resources allocated when installing SMBIOS Type 23 table.

  @param [in]  This                 Pointer to the SMBIOS table generator.
  @param [in]  TableFactoryProtocol Pointer to the SMBIOS Table Factory
                                    Protocol interface.
  @param [in]  SmbiosTableInfo      Pointer to the SMBIOS table information.
  @param [in]  CfgMgrProtocol       Pointer to the Configuration Manager
                                    Protocol interface.
  @param [in]  Table                Pointer to the generated SMBIOS table.

  @retval EFI_SUCCESS            Resources were freed successfully.
**/
STATIC
EFI_STATUS
FreeSmbiosType23Table (
  IN      CONST SMBIOS_TABLE_GENERATOR                    *CONST  This,
  IN      CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL      *CONST  TableFactoryProtocol,
  IN      CONST CM_STD_OBJ_SMBIOS_TABLE_INFO              *CONST  SmbiosTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL      *CONST  CfgMgrProtocol,
  IN      SMBIOS_STRUCTURE                               **CONST  Table
  )
{
  FreePool (*Table);
  *Table = NULL;

  return EFI_SUCCESS;
}

/** Construct SMBIOS Type 23 Table describing system reset information.

  If this function allocates any resources then they must be freed
  in the FreeSmbiosType23Table function.

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
  @retval EFI_BAD_BUFFER_SIZE    The size returned by the Configuration
                                 Manager is less than the Object size for
                                 the requested object.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_NOT_FOUND          Could not find information.
  @retval EFI_OUT_OF_RESOURCES   Could not allocate memory.
  @retval EFI_UNSUPPORTED        Unsupported configuration.
**/
STATIC
EFI_STATUS
BuildSmbiosType23Table (
  IN  CONST SMBIOS_TABLE_GENERATOR                         *This,
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL   *CONST  TableFactoryProtocol,
  IN        CM_STD_OBJ_SMBIOS_TABLE_INFO           *CONST  SmbiosTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  OUT       SMBIOS_STRUCTURE                               **Table,
  OUT       CM_OBJECT_TOKEN                        *CONST  CmObjectToken
  )
{
  EFI_STATUS                        Status;
  UINT32                            SystemResetCount;
  SMBIOS_TABLE_TYPE23               *SmbiosRecord;
  CM_ARCH_COMMON_SYSTEM_RESET_INFO  *SystemResetInfo;

  ASSERT (This != NULL);
  ASSERT (SmbiosTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (CmObjectToken != NULL);
  ASSERT (Table != NULL);
  ASSERT (SmbiosTableInfo->TableGeneratorId == This->GeneratorID);

  if ((This == NULL) || (SmbiosTableInfo == NULL) || (CfgMgrProtocol == NULL) ||
      (Table == NULL) || (CmObjectToken == NULL) ||
      (SmbiosTableInfo->TableGeneratorId != This->GeneratorID))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Parameter\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  *Table         = NULL;
  *CmObjectToken = CM_NULL_TOKEN;
  SmbiosRecord   = NULL;

  Status = GetEArchCommonObjSystemResetInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &SystemResetInfo,
             &SystemResetCount
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get system reset info. Status = %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  if (SystemResetCount != 1) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Expected 1 System Reset CM Object, got %u\n",
      __func__,
      SystemResetCount
      ));
    return EFI_INVALID_PARAMETER;
  }

  if ((SystemResetInfo[0].Capabilities & SMBIOS_TYPE23_CAPABILITIES_RESERVED_MASK) != 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Invalid System Reset Capabilities 0x%x. Bits 7:6 must be zero.\n",
      __func__,
      SystemResetInfo[0].Capabilities
      ));
    return EFI_INVALID_PARAMETER;
  }

  SmbiosRecord = (SMBIOS_TABLE_TYPE23 *)AllocateSmbiosRecord (
                                          sizeof (SMBIOS_TABLE_TYPE23),
                                          NULL
                                          );
  if (SmbiosRecord == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to allocate memory for System Reset table\n",
      __func__
      ));
    return EFI_OUT_OF_RESOURCES;
  }

  SmbiosRecord->Hdr.Type   = EFI_SMBIOS_TYPE_SYSTEM_RESET;
  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE23);

  SmbiosRecord->Capabilities  = SystemResetInfo[0].Capabilities;
  SmbiosRecord->ResetCount    = SystemResetInfo[0].ResetCount;
  SmbiosRecord->ResetLimit    = SystemResetInfo[0].ResetLimit;
  SmbiosRecord->TimerInterval = SystemResetInfo[0].TimerInterval;
  SmbiosRecord->Timeout       = SystemResetInfo[0].Timeout;

  *Table         = (SMBIOS_STRUCTURE *)SmbiosRecord;
  *CmObjectToken = SystemResetInfo[0].SystemResetToken;
  SmbiosRecord   = NULL;

  return EFI_SUCCESS;
}

/** The interface for the SMBIOS Type23 Table Generator.
*/
STATIC
CONST
SMBIOS_TABLE_GENERATOR  SmbiosType23Generator = {
  // Generator ID
  CREATE_STD_SMBIOS_TABLE_GEN_ID (EStdSmbiosTableIdType23),
  // Generator Description
  L"SMBIOS.TYPE23.GENERATOR",
  // SMBIOS Table Type
  EFI_SMBIOS_TYPE_SYSTEM_RESET,
  // Build table function.
  BuildSmbiosType23Table,
  // Free function.
  FreeSmbiosType23Table,
  NULL,
  NULL,
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
SmbiosType23LibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterSmbiosTableGenerator (&SmbiosType23Generator);
  DEBUG ((
    DEBUG_INFO,
    "SMBIOS Type 23: Register Generator. Status = %r\n",
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
SmbiosType23LibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterSmbiosTableGenerator (&SmbiosType23Generator);
  DEBUG ((
    DEBUG_INFO,
    "SMBIOS Type 23: Deregister Generator. Status = %r\n",
    Status
    ));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
