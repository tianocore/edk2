/** @file
  SMBIOS Type16 Table Generator.

  Copyright (c) 2024 - 2026, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
  EArchCommonObjPhysicalMemoryArray,
  CM_ARCH_COMMON_PHYSICAL_MEMORY_ARRAY
  )

#define EXTENDED_SIZE_THRESHOLD  (SIZE_2TB)

/**
  Free any resources allocated when installing SMBIOS Type16 table.

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
FreeSmbiosType16TableEx (
  IN      CONST SMBIOS_TABLE_GENERATOR                    *CONST   This,
  IN      CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL      *CONST   TableFactoryProtocol,
  IN      CONST CM_STD_OBJ_SMBIOS_TABLE_INFO              *CONST   SmbiosTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL      *CONST   CfgMgrProtocol,
  IN      SMBIOS_STRUCTURE                               ***CONST  Table,
  IN      CM_OBJECT_TOKEN                                          **CmObjectToken,
  IN      CONST UINTN                                              TableCount
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
  Update the Size encoding for Type 16.

  @param [in]      SizeBytes    Size of the Memory device.
  @param [in,out]  SmbiosRecord SMBIOS record to update.
**/
STATIC
VOID
UpdateSmbiosType16Size (
  IN     UINT64               SizeBytes,
  IN OUT SMBIOS_TABLE_TYPE16  *SmbiosRecord
  )
{
  UINT64  SizeKb;

  SizeKb = SizeBytes / SIZE_1KB;

  if (SizeBytes < EXTENDED_SIZE_THRESHOLD) {
    SmbiosRecord->MaximumCapacity = SizeKb;
  } else {
    SmbiosRecord->MaximumCapacity         = 0x80000000;
    SmbiosRecord->ExtendedMaximumCapacity = SizeBytes;
  }
}

/**
  Add the SMBIOS table handle reference to the Error Tables.

  @param [in]   TableFactoryProtocol  Pointer to the SMBIOS Table Factory.
  @param [in]   CmObjToken            CM Token to lookup.
  @param [out]  SmbiosRecord          SMBIOS record to update.
**/
STATIC
VOID
AddMemErrDeviceHandle (
  OUT SMBIOS_TABLE_TYPE16  *SmbiosRecord
  )
{
  // TODO: Query for Type 18/33 CM objects via CfgMgrProtocol to determine
  // whether the platform supports memory error information structures.
  // Set 0xFFFF if supported but no error detected on this array, or 0xFFFE
  // if error information structures are not supported at all.
  // When implementing, add MemoryErrInfoToken to CM_ARCH_COMMON_PHYSICAL_MEMORY_ARRAY
  // and pass CfgMgrProtocol and TableFactoryProtocol as parameters to resolve
  // the token to an actual SMBIOS handle.
  SmbiosRecord->MemoryErrorInformationHandle = 0xFFFE;
}

/** Construct SMBIOS Type16 Table describing memory devices.

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
BuildSmbiosType16TableEx (
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
  SMBIOS_STRUCTURE                      **TableList;
  SMBIOS_TABLE_TYPE16                   *SmbiosRecord;
  CM_OBJECT_TOKEN                       *CmObjectList;
  CM_ARCH_COMMON_PHYSICAL_MEMORY_ARRAY  *PhysMemoryArray;
  UINT32                                NumObj;
  UINTN                                 Index;

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

  *Table       = NULL;
  TableList    = NULL;
  CmObjectList = NULL;
  Status       = GetEArchCommonObjPhysicalMemoryArray (
                   CfgMgrProtocol,
                   CM_NULL_TOKEN,
                   &PhysMemoryArray,
                   &NumObj
                   );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get Memory Devices CM Object %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  if (NumObj == 0) {
    DEBUG ((DEBUG_ERROR, "%a: No Physical Memory Array CM Objects found\n", __func__));
    return EFI_NOT_FOUND;
  }

  TableList = (SMBIOS_STRUCTURE **)AllocateZeroPool (sizeof (SMBIOS_STRUCTURE *) * NumObj);
  if (TableList == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to alloc memory for %u devices table\n",
      __func__,
      NumObj
      ));
    return EFI_OUT_OF_RESOURCES;
  }

  CmObjectList = (CM_OBJECT_TOKEN *)AllocateZeroPool (sizeof (CM_OBJECT_TOKEN *) * NumObj);
  if (CmObjectList == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to alloc memory for %u CM Objects.\n",
      __func__,
      NumObj
      ));
    Status = EFI_OUT_OF_RESOURCES;
    goto exitBuildSmbiosType16Table;
  }

  for (Index = 0; Index < NumObj; Index++) {
    SmbiosRecord = (SMBIOS_TABLE_TYPE16 *)AllocateSmbiosRecord (sizeof (SMBIOS_TABLE_TYPE16), NULL);
    if (SmbiosRecord == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto exitBuildSmbiosType16Table;
    }

    if (PhysMemoryArray[Index].Location > MemoryArrayLocationCXLAddonCard) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Invalid Location 0x%x for PhysMemoryArray[%u]\n",
        __func__,
        PhysMemoryArray[Index].Location,
        Index
        ));
      Status = EFI_INVALID_PARAMETER;
      goto exitBuildSmbiosType16Table;
    }

    if (PhysMemoryArray[Index].Use > MemoryArrayUseCacheMemory) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Invalid Use 0x%x for PhysMemoryArray[%u]\n",
        __func__,
        PhysMemoryArray[Index].Use,
        Index
        ));
      Status = EFI_INVALID_PARAMETER;
      goto exitBuildSmbiosType16Table;
    }

    if (PhysMemoryArray[Index].MemoryErrorCorrectionType > MemoryErrorCorrectionCrc) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Invalid MemoryErrorCorrectionType 0x%x for PhysMemoryArray[%u]\n",
        __func__,
        PhysMemoryArray[Index].MemoryErrorCorrectionType,
        Index
        ));
      Status = EFI_INVALID_PARAMETER;
      goto exitBuildSmbiosType16Table;
    }

    UpdateSmbiosType16Size (PhysMemoryArray[Index].Size, SmbiosRecord);
    SmbiosRecord->Location              = PhysMemoryArray[Index].Location;
    SmbiosRecord->Use                   = PhysMemoryArray[Index].Use;
    SmbiosRecord->MemoryErrorCorrection = PhysMemoryArray[Index].MemoryErrorCorrectionType;
    if (PhysMemoryArray[Index].NumberOfMemoryDevices == 0) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: NumberOfMemoryDevices is 0 for PhysMemoryArray[%u]\n",
        __func__,
        Index
        ));
      Status = EFI_INVALID_PARAMETER;
      goto exitBuildSmbiosType16Table;
    }

    SmbiosRecord->NumberOfMemoryDevices = PhysMemoryArray[Index].NumberOfMemoryDevices;
    AddMemErrDeviceHandle (SmbiosRecord);

    // Setup the header
    SmbiosRecord->Hdr.Type   = EFI_SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY;
    SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE16);

    TableList[Index]    = (SMBIOS_STRUCTURE *)SmbiosRecord;
    CmObjectList[Index] = PhysMemoryArray[Index].PhysMemArrayToken;
  }

  *Table         = TableList;
  *CmObjectToken = CmObjectList;
  *TableCount    = NumObj;
exitBuildSmbiosType16Table:
  if (EFI_ERROR (Status)) {
    if (TableList != NULL) {
      for (Index = 0; Index < NumObj; Index++) {
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

  return Status;
}

/** The interface for the SMBIOS Type16 Table Generator.
*/
STATIC
CONST
SMBIOS_TABLE_GENERATOR  SmbiosType16Generator = {
  // Generator ID
  CREATE_STD_SMBIOS_TABLE_GEN_ID (EStdSmbiosTableIdType16),
  // Generator Description
  L"SMBIOS.TYPE16.GENERATOR",
  // SMBIOS Table Type
  EFI_SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY,
  NULL,
  NULL,
  // Build table function.
  BuildSmbiosType16TableEx,
  // Free function.
  FreeSmbiosType16TableEx,
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
SmbiosType16LibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterSmbiosTableGenerator (&SmbiosType16Generator);
  DEBUG ((
    DEBUG_INFO,
    "SMBIOS Type 16: Register Generator. Status = %r\n",
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
SmbiosType16LibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterSmbiosTableGenerator (&SmbiosType16Generator);
  DEBUG ((
    DEBUG_INFO,
    "SMBIOS Type16: Deregister Generator. Status = %r\n",
    Status
    ));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
