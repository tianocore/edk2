/** @file
  SMBIOS Type20 Table Generator.

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

/** SMBIOS Type 20 Memory Device Mapped Address Generator

Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - EArchCommonObjMemoryDeviceMappedAddress

  The following Configuration Manager Object(s) are required when
  the corresponding token is not CM_NULL_TOKEN:
  - EArchCommonObjMemoryDeviceInfo
  - EArchCommonObjMemoryArrayMappedAddress
*/

/**
  This macro expands to a function that retrieves the Memory Device
  Mapped Address information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjMemoryDeviceMappedAddress,
  CM_ARCH_COMMON_MEMORY_DEVICE_MAPPED_ADDRESS
  );

#define EXTENDED_ADDRESS_THRESHOLD  (0xFFFFFFFFULL)

/**
  Free any resources allocated when installing SMBIOS Type 20 table.

  @param [in]  This                 Pointer to the SMBIOS table generator.
  @param [in]  TableFactoryProtocol Pointer to the SMBIOS Table Factory
                                    Protocol interface.
  @param [in]  SmbiosTableInfo      Pointer to the SMBIOS table information.
  @param [in]  CfgMgrProtocol       Pointer to the Configuration Manager
                                    Protocol interface.
  @param [in]  Table                Pointer to the SMBIOS table.
  @param [in]  CmObjectToken        Pointer to the CM ObjectToken Array.
  @param [in]  TableCount           Number of SMBIOS tables.

  @retval EFI_SUCCESS  Resources freed successfully.
**/
STATIC
EFI_STATUS
EFIAPI
FreeSmbiosType20TableEx (
  IN      CONST SMBIOS_TABLE_GENERATOR                   *CONST  This,
  IN      CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL     *CONST  TableFactoryProtocol,
  IN      CONST CM_STD_OBJ_SMBIOS_TABLE_INFO             *CONST  SmbiosTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL     *CONST  CfgMgrProtocol,
  IN      SMBIOS_STRUCTURE                             ***CONST  Table,
  IN      CM_OBJECT_TOKEN                                        **CmObjectToken,
  IN      CONST UINTN                                            TableCount
  )
{
  UINTN             Index;
  SMBIOS_STRUCTURE  **TableList;

  TableList = *Table;
  for (Index = 0; Index < TableCount; Index++) {
    if (TableList[Index] != NULL) {
      FreePool (TableList[Index]);
    }
  }

  if (*CmObjectToken != NULL) {
    FreePool (*CmObjectToken);
  }

  if (TableList != NULL) {
    FreePool (TableList);
  }

  return EFI_SUCCESS;
}

/** Construct SMBIOS Type 20 Table describing memory device mapped addresses.

  If this function allocates any resources then they must be freed
  in the FreeSmbiosType20TableEx function.

  @param [in]  This                 Pointer to the SMBIOS table generator.
  @param [in]  TableFactoryProtocol Pointer to the SMBIOS Table Factory
                                    Protocol interface.
  @param [in]  SmbiosTableInfo      Pointer to the SMBIOS table information.
  @param [in]  CfgMgrProtocol       Pointer to the Configuration Manager
                                    Protocol interface.
  @param [out] Table                Pointer to the SMBIOS table.
  @param [out] CmObjectToken        Pointer to the CM ObjectToken Array.
  @param [out] TableCount           Number of SMBIOS tables.

  @retval EFI_SUCCESS            Table generated successfully.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_NOT_FOUND          Could not find required information.
  @retval EFI_OUT_OF_RESOURCES   Could not allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
BuildSmbiosType20TableEx (
  IN  CONST SMBIOS_TABLE_GENERATOR                          *This,
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL   *CONST   TableFactoryProtocol,
  IN        CM_STD_OBJ_SMBIOS_TABLE_INFO           *CONST   SmbiosTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST   CfgMgrProtocol,
  OUT       SMBIOS_STRUCTURE                                ***Table,
  OUT       CM_OBJECT_TOKEN                        **CONST  CmObjectToken,
  OUT       UINTN                                  *CONST   TableCount
  )
{
  EFI_STATUS                                   Status;
  UINT32                                       NumMemDeviceMap;
  CM_ARCH_COMMON_MEMORY_DEVICE_MAPPED_ADDRESS  *MemoryDeviceMapInfo;
  SMBIOS_STRUCTURE                             **TableList;
  CM_OBJECT_TOKEN                              *CmObjectList;
  SMBIOS_TABLE_TYPE20                          *SmbiosRecord;
  SMBIOS_HANDLE_MAP                            *HandleMap;
  UINT64                                       StartingAddressKb;
  UINT64                                       EndingAddressKb;
  UINTN                                        Index;

  TableList    = NULL;
  CmObjectList = NULL;
  SmbiosRecord = NULL;

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

  Status = GetEArchCommonObjMemoryDeviceMappedAddress (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &MemoryDeviceMapInfo,
             &NumMemDeviceMap
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get Memory Device Mapped Address CM Object. Status = %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  if (NumMemDeviceMap == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: No Memory Device Mapped Address CM Objects found\n",
      __func__
      ));
    return EFI_NOT_FOUND;
  }

  TableList = (SMBIOS_STRUCTURE **)AllocateZeroPool (
                                     sizeof (SMBIOS_STRUCTURE *) * NumMemDeviceMap
                                     );
  if (TableList == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to allocate memory for %u Memory Device Mapped Address tables\n",
      __func__,
      NumMemDeviceMap
      ));
    return EFI_OUT_OF_RESOURCES;
  }

  CmObjectList = (CM_OBJECT_TOKEN *)AllocateZeroPool (
                                      sizeof (CM_OBJECT_TOKEN) * NumMemDeviceMap
                                      );
  if (CmObjectList == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to allocate memory for %u CM Object tokens\n",
      __func__,
      NumMemDeviceMap
      ));
    Status = EFI_OUT_OF_RESOURCES;
    goto exitBuildSmbiosType20TableEx;
  }

  for (Index = 0; Index < NumMemDeviceMap; Index++) {
    if (MemoryDeviceMapInfo[Index].StartingAddress > MemoryDeviceMapInfo[Index].EndingAddress) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Invalid address range. StartingAddress = 0x%lx, EndingAddress = 0x%lx\n",
        __func__,
        MemoryDeviceMapInfo[Index].StartingAddress,
        MemoryDeviceMapInfo[Index].EndingAddress
        ));
      Status = EFI_INVALID_PARAMETER;
      goto exitBuildSmbiosType20TableEx;
    }

    if (MemoryDeviceMapInfo[Index].PartitionRowPosition == 0 ) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Invalid PartitionRowPosition. Value must not be 0.\n",
        __func__
        ));
      Status = EFI_INVALID_PARAMETER;
      goto exitBuildSmbiosType20TableEx;
    }

    SmbiosRecord = (SMBIOS_TABLE_TYPE20 *)AllocateSmbiosRecord (
                                            sizeof (SMBIOS_TABLE_TYPE20),
                                            NULL
                                            );
    if (SmbiosRecord == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto exitBuildSmbiosType20TableEx;
    }

    StartingAddressKb = MemoryDeviceMapInfo[Index].StartingAddress / SIZE_1KB;
    EndingAddressKb   = MemoryDeviceMapInfo[Index].EndingAddress / SIZE_1KB;

    if ((StartingAddressKb >= EXTENDED_ADDRESS_THRESHOLD) ||
        (EndingAddressKb >= EXTENDED_ADDRESS_THRESHOLD))
    {
      SmbiosRecord->StartingAddress         = EXTENDED_ADDRESS_THRESHOLD;
      SmbiosRecord->EndingAddress           = EXTENDED_ADDRESS_THRESHOLD;
      SmbiosRecord->ExtendedStartingAddress = MemoryDeviceMapInfo[Index].StartingAddress;
      SmbiosRecord->ExtendedEndingAddress   = MemoryDeviceMapInfo[Index].EndingAddress;
    } else {
      SmbiosRecord->StartingAddress = (UINT32)StartingAddressKb;
      SmbiosRecord->EndingAddress   = (UINT32)EndingAddressKb;
    }

    if (MemoryDeviceMapInfo[Index].MemoryDeviceInfoToken == CM_NULL_TOKEN) {
      SmbiosRecord->MemoryDeviceHandle = SMBIOS_HANDLE_INVALID;
    } else {
      HandleMap = TableFactoryProtocol->GetSmbiosHandle (
                                          MemoryDeviceMapInfo[Index].MemoryDeviceInfoToken
                                          );
      if (HandleMap == NULL) {
        DEBUG ((DEBUG_ERROR, "%a: Failed to get Type 17 SMBIOS Handle\n", __func__));
        Status = EFI_NOT_FOUND;
        goto exitBuildSmbiosType20TableEx;
      }

      SmbiosRecord->MemoryDeviceHandle = HandleMap->SmbiosTblHandle;
    }

    if (MemoryDeviceMapInfo[Index].MemoryArrayMappedAddressToken == CM_NULL_TOKEN) {
      SmbiosRecord->MemoryArrayMappedAddressHandle = SMBIOS_HANDLE_INVALID;
    } else {
      HandleMap = TableFactoryProtocol->GetSmbiosHandle (
                                          MemoryDeviceMapInfo[Index].MemoryArrayMappedAddressToken
                                          );
      if (HandleMap == NULL) {
        DEBUG ((DEBUG_ERROR, "%a: Failed to get Type 19 SMBIOS Handle\n", __func__));
        Status = EFI_NOT_FOUND;
        goto exitBuildSmbiosType20TableEx;
      }

      SmbiosRecord->MemoryArrayMappedAddressHandle = HandleMap->SmbiosTblHandle;
    }

    SmbiosRecord->PartitionRowPosition = MemoryDeviceMapInfo[Index].PartitionRowPosition;
    SmbiosRecord->InterleavePosition   = MemoryDeviceMapInfo[Index].InterleavePosition;
    SmbiosRecord->InterleavedDataDepth = MemoryDeviceMapInfo[Index].InterleavedDataDepth;

    SmbiosRecord->Hdr.Type   = EFI_SMBIOS_TYPE_MEMORY_DEVICE_MAPPED_ADDRESS;
    SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE20);
    TableList[Index]         = (SMBIOS_STRUCTURE *)SmbiosRecord;
    CmObjectList[Index]      = MemoryDeviceMapInfo[Index].MemoryDeviceMappedAddressToken;

    SmbiosRecord = NULL;
  }

  *Table         = TableList;
  *CmObjectToken = CmObjectList;
  *TableCount    = NumMemDeviceMap;

exitBuildSmbiosType20TableEx:
  if (EFI_ERROR (Status)) {
    if (TableList != NULL) {
      for (Index = 0; Index < NumMemDeviceMap; Index++) {
        if (TableList[Index] != NULL) {
          FreePool (TableList[Index]);
        }
      }

      FreePool (TableList);
    }

    if (CmObjectList != NULL) {
      FreePool (CmObjectList);
    }
  }

  if (SmbiosRecord != NULL) {
    FreePool (SmbiosRecord);
  }

  return Status;
}

/** The interface for the SMBIOS Type 20 Table Generator.
*/
STATIC
CONST
SMBIOS_TABLE_GENERATOR  SmbiosType20Generator = {
  // Generator ID
  CREATE_STD_SMBIOS_TABLE_GEN_ID (EStdSmbiosTableIdType20),
  // Generator Description
  L"SMBIOS.TYPE20.GENERATOR",
  // SMBIOS Table Type
  SMBIOS_TYPE_MEMORY_DEVICE_MAPPED_ADDRESS,
  NULL,
  NULL,
  // Build table function Extended.
  BuildSmbiosType20TableEx,
  // Free function Extended.
  FreeSmbiosType20TableEx
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
SmbiosType20LibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterSmbiosTableGenerator (&SmbiosType20Generator);
  DEBUG ((
    DEBUG_INFO,
    "SMBIOS Type 20: Register Generator. Status = %r\n",
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
SmbiosType20LibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterSmbiosTableGenerator (&SmbiosType20Generator);
  DEBUG ((
    DEBUG_INFO,
    "SMBIOS Type 20: Deregister Generator. Status = %r\n",
    Status
    ));
  ASSERT_EFI_ERROR (Status);

  return Status;
}
