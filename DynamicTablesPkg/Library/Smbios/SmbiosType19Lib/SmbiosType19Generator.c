/** @file
  SMBIOS Type19 Table Generator.

  Copyright (c) 2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
  Copyright (c) 2020 - 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/SmbiosStringTableLib.h>

// Module specific include files.
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <Protocol/DynamicTableFactoryProtocol.h>
#include <Protocol/Smbios.h>
#include <IndustryStandard/SmBios.h>

/** This macro expands to a function that retrieves the Memory Device
    information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjMemoryArrayMappedAddress,
  CM_ARCH_COMMON_MEMORY_ARRAY_MAPPED_ADDRESS
  )

#define EXTENDED_ADDRESS_THRESHOLD  (0xFFFFFFFFL)

/**
  Free any resources allocated when installing SMBIOS Type19 table.

  @param [in]  This                 Pointer to the SMBIOS table generator.
  @param [in]  TableFactoryProtocol Pointer to the SMBIOS Table Factory
                                    Protocol interface.
  @param [in]  SmbiosTableInfo      Pointer to the SMBIOS table information.
  @param [in]  CfgMgrProtocol       Pointer to the Configuration Manager
                                    Protocol interface.
  @param [in]  Table                Pointer to the SMBIOS table.
  @param [in]  CmObjectToken        Pointer to the CM ObjectToken Array.
  @param [in]  TableCount           Number of SMBIOS tables.

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
FreeSmbiosType19TableEx (
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

/**
  Add the SMBIOS table handle reference to the Physical Array Table.

  @param [in]   TableFactoryProtocol  Pointer to the SMBIOS Table Factory.
  @param [in]   CmObjToken            CM Token to lookup.
  @param [out]  SmbiosRecord          SMBIOS record to update.
**/
STATIC
VOID
AddPhysArrHandle (
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL   *CONST  TableFactoryProtocol,
  IN  CM_OBJECT_TOKEN                                      CmObjToken,
  OUT SMBIOS_TABLE_TYPE19                                  *SmbiosRecord
  )
{
  EFI_SMBIOS_HANDLE  PhysMemArrHandle;
  SMBIOS_HANDLE_MAP  *HandleMap;

  HandleMap = TableFactoryProtocol->GetSmbiosHandle (CmObjToken);
  if (HandleMap == NULL) {
    DEBUG ((DEBUG_ERROR, "%a:Failed to get SMBIOS Handle\n", __func__));
    PhysMemArrHandle = 0;
  } else {
    PhysMemArrHandle = HandleMap->SmbiosTblHandle;
  }

  SmbiosRecord->MemoryArrayHandle = PhysMemArrHandle;
}

/**
  Update the Address encoding for Type 19.

  @param [in]   StartAddress  Starting memory address covered by the device.
  @param [in]   EndAddress    Ending memory address covered by the device.
  @param [out]  SmbiosRecord  SMBIOS record to update.
**/
STATIC
VOID
UpdateSmbiosType19Address (
  IN  UINT64               StartAddress,
  IN  UINT64               EndAddress,
  OUT SMBIOS_TABLE_TYPE19  *SmbiosRecord
  )
{
  UINT64  StartingAddressKb;
  UINT64  EndingAddressKb;

  StartingAddressKb = StartAddress / SIZE_1KB;
  EndingAddressKb   = EndAddress / SIZE_1KB;

  if (StartingAddressKb >= EXTENDED_ADDRESS_THRESHOLD) {
    SmbiosRecord->StartingAddress         = EXTENDED_ADDRESS_THRESHOLD;
    SmbiosRecord->EndingAddress           = EXTENDED_ADDRESS_THRESHOLD;
    SmbiosRecord->ExtendedStartingAddress = StartAddress;
    SmbiosRecord->ExtendedEndingAddress   = EndAddress;
  } else {
    SmbiosRecord->StartingAddress = StartingAddressKb;
    SmbiosRecord->EndingAddress   = EndingAddressKb;
  }
}

/** Construct SMBIOS Type19 Table describing memory devices.

  If this function allocates any resources then they must be freed
  in the FreeXXXXTableResources function.

  @param [in]  This                 Pointer to the SMBIOS table generator.
  @param [in]  TableFactoryProtocol Pointer to the SMBIOS Table Factory
                                    Protocol interface.
  @param [in]  SmbiosTableInfo      Pointer to the SMBIOS table information.
  @param [in]  CfgMgrProtocol       Pointer to the Configuration Manager
                                    Protocol interface.
  @param [out] Table                Pointer to the SMBIOS table.
  @param [out] CmObjectToken        Pointer to the CM Object Token Array.
  @param [out] TableCount           Number of tables installed.

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
BuildSmbiosType19TableEx (
  IN  CONST SMBIOS_TABLE_GENERATOR                         *This,
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL   *CONST  TableFactoryProtocol,
  IN        CM_STD_OBJ_SMBIOS_TABLE_INFO           *CONST  SmbiosTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  OUT       SMBIOS_STRUCTURE                               ***Table,
  OUT       CM_OBJECT_TOKEN                                **CmObjectToken,
  OUT       UINTN                                  *CONST  TableCount
  )
{
  EFI_STATUS                                  Status;
  UINT32                                      NumMemMap;
  SMBIOS_STRUCTURE                            **TableList;
  CM_OBJECT_TOKEN                             *CmObjectList;
  CM_ARCH_COMMON_MEMORY_ARRAY_MAPPED_ADDRESS  *MemoryMapInfo;
  SMBIOS_TABLE_TYPE19                         *SmbiosRecord;
  UINTN                                       Index;

  ASSERT (This != NULL);
  ASSERT (SmbiosTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Table != NULL);
  ASSERT (TableCount != NULL);
  ASSERT (SmbiosTableInfo->TableGeneratorId == This->GeneratorID);

  if ((This == NULL) || (SmbiosTableInfo == NULL) || (CfgMgrProtocol == NULL) ||
      (Table == NULL) || (TableCount == NULL) ||
      (SmbiosTableInfo->TableGeneratorId != This->GeneratorID))
  {
    DEBUG ((DEBUG_ERROR, "%a:Invalid Parameter\n ", __func__));
    return EFI_INVALID_PARAMETER;
  }

  *Table = NULL;
  Status = GetEArchCommonObjMemoryArrayMappedAddress (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &MemoryMapInfo,
             &NumMemMap
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Failed to get Memory Devices CM Object %r\n",
      Status
      ));
    return Status;
  }

  TableList = (SMBIOS_STRUCTURE **)AllocateZeroPool (sizeof (SMBIOS_STRUCTURE *) * NumMemMap);
  if (TableList == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to alloc memory for %u devices table\n",
      __func__,
      NumMemMap
      ));
    Status = EFI_OUT_OF_RESOURCES;
    goto exitBuildSmbiosType19TableEx;
  }

  CmObjectList = (CM_OBJECT_TOKEN *)AllocateZeroPool (sizeof (CM_OBJECT_TOKEN *) * NumMemMap);
  if (CmObjectList == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to alloc memory for %u CMObjects\n",
      __func__,
      NumMemMap
      ));
    Status = EFI_OUT_OF_RESOURCES;
    goto exitBuildSmbiosType19TableEx;
  }

  for (Index = 0; Index < NumMemMap; Index++) {
    /**
     * Per Spec each structure is terminated by a double-NULL if there are no
     * strings.
     */
    SmbiosRecord = (SMBIOS_TABLE_TYPE19 *)AllocateZeroPool (sizeof (SMBIOS_TABLE_TYPE19) + 1 + 1);
    if (SmbiosRecord == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto exitBuildSmbiosType19TableEx;
    }

    UpdateSmbiosType19Address (
      MemoryMapInfo[Index].StartingAddress,
      MemoryMapInfo[Index].EndingAddress,
      SmbiosRecord
      );
    SmbiosRecord->PartitionWidth = MemoryMapInfo[Index].NumMemDevices;
    // Is there a reference to a Physical Array Device.
    if (MemoryMapInfo[Index].PhysMemArrayToken != CM_NULL_TOKEN) {
      AddPhysArrHandle (
        TableFactoryProtocol,
        MemoryMapInfo[Index].PhysMemArrayToken,
        SmbiosRecord
        );
    }

    // setup the header
    SmbiosRecord->Hdr.Type   = EFI_SMBIOS_TYPE_MEMORY_ARRAY_MAPPED_ADDRESS;
    SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE19);
    TableList[Index]         = (SMBIOS_STRUCTURE *)SmbiosRecord;
    CmObjectList[Index]      = MemoryMapInfo[Index].MemoryArrayMappedAddressToken;
  }

  *Table         = TableList;
  *CmObjectToken = CmObjectList;
  *TableCount    = NumMemMap;

exitBuildSmbiosType19TableEx:
  return Status;
}

/** The interface for the SMBIOS Type17 Table Generator.
*/
STATIC
CONST
SMBIOS_TABLE_GENERATOR  SmbiosType19Generator = {
  // Generator ID
  CREATE_STD_SMBIOS_TABLE_GEN_ID (EStdSmbiosTableIdType19),
  // Generator Description
  L"SMBIOS.TYPE19.GENERATOR",
  // SMBIOS Table Type
  EFI_SMBIOS_TYPE_MEMORY_DEVICE,
  NULL,
  NULL,
  // Build table function Extended.
  BuildSmbiosType19TableEx,
  // Free function Extended.
  FreeSmbiosType19TableEx
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
SmbiosType19LibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterSmbiosTableGenerator (&SmbiosType19Generator);
  DEBUG ((
    DEBUG_INFO,
    "SMBIOS Type 19: Register Generator. Status = %r\n",
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
SmbiosType19LibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterSmbiosTableGenerator (&SmbiosType19Generator);
  DEBUG ((
    DEBUG_INFO,
    "SMBIOS Type19: Deregister Generator. Status = %r\n",
    Status
    ));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
