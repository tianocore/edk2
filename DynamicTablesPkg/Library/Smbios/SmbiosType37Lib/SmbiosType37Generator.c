/** @file
  SMBIOS Type37 Table Generator.

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

/** SMBIOS Type 37 Memory Channel Generator

Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - EArchCommonObjMemoryChannelInfo
  - EArchCommonObjMemoryChannelDevice

  The following Configuration Manager Object(s) are required to resolve
  the Memory Device handles:
  - EArchCommonObjMemoryDeviceInfo
*/

/**
  This macro expands to a function that retrieves the Memory Channel
  information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjMemoryChannelInfo,
  CM_ARCH_COMMON_MEMORY_CHANNEL_INFO
  );

/**
  This macro expands to a function that retrieves the Memory Channel
  Device entries from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjMemoryChannelDevice,
  CM_ARCH_COMMON_MEMORY_CHANNEL_DEVICE
  );

/** Construct SMBIOS Type 37 table describing Memory Channel information.

  If this function allocates any resources then they must be freed in
  FreeSmbiosType37TableEx().

  @param [in]  This                  Pointer to the SMBIOS table generator.
  @param [in]  TableFactoryProtocol  Pointer to the SMBIOS table factory protocol.
  @param [in]  SmbiosTableInfo       Pointer to the SMBIOS table information.
  @param [in]  CfgMgrProtocol        Pointer to the Configuration Manager
                                     Protocol interface.
  @param [out] Table                 Pointer to the generated SMBIOS table.
  @param [out] CmObjectToken         Pointer to the CM object token for the
                                     generated SMBIOS table.
  @param [out] TableCount            Number of generated SMBIOS tables.

  @retval EFI_SUCCESS            Table generated successfully.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_NOT_FOUND          Required CM object is not found.
**/
STATIC
EFI_STATUS
EFIAPI
BuildSmbiosType37TableEx (
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
  UINT32                                NumMemChannels;
  UINT32                                MemoryChannelDeviceCount;
  CM_ARCH_COMMON_MEMORY_CHANNEL_INFO    *MemoryChannelInfo;
  CM_ARCH_COMMON_MEMORY_CHANNEL_DEVICE  *MemoryChannelDevice;
  CM_OBJECT_TOKEN                       *CmObjectList;
  SMBIOS_STRUCTURE                      **TableList;
  SMBIOS_TABLE_TYPE37                   *SmbiosRecord;
  SMBIOS_HANDLE                         MemoryDeviceHandle;
  UINTN                                 Index;
  UINTN                                 DeviceIndex;
  UINTN                                 RecordSize;
  UINTN                                 TotalDeviceLoad;

  TableList                = NULL;
  CmObjectList             = NULL;
  SmbiosRecord             = NULL;
  MemoryChannelDevice      = NULL;
  MemoryChannelDeviceCount = 0;
  TotalDeviceLoad          = 0;

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
      (Table == NULL) || (CmObjectToken == NULL) ||
      (TableCount == NULL))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Parameter\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  *Table         = NULL;
  *CmObjectToken = NULL;
  *TableCount    = 0;

  Status = GetEArchCommonObjMemoryChannelInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &MemoryChannelInfo,
             &NumMemChannels
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get Memory Channel CM Object. Status = %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  if (NumMemChannels == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: No Memory Channel CM Objects found\n",
      __func__
      ));
    return EFI_NOT_FOUND;
  }

  TableList = (SMBIOS_STRUCTURE **)AllocateZeroPool (
                                     sizeof (SMBIOS_STRUCTURE *) * NumMemChannels
                                     );

  if (TableList == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to allocate memory for %u Memory Channel tables\n",
      __func__,
      NumMemChannels
      ));
    Status = EFI_OUT_OF_RESOURCES;
    goto exitBuildSmbiosType37TableEx;
  }

  CmObjectList = (CM_OBJECT_TOKEN *)AllocateZeroPool (
                                      sizeof (CM_OBJECT_TOKEN) * NumMemChannels
                                      );
  if (CmObjectList == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to allocate memory for %u CM Object tokens\n",
      __func__,
      NumMemChannels
      ));
    Status = EFI_OUT_OF_RESOURCES;
    goto exitBuildSmbiosType37TableEx;
  }

  for (Index = 0; Index < NumMemChannels; Index++) {
    MemoryChannelDevice      = NULL;
    MemoryChannelDeviceCount = 0;
    TotalDeviceLoad          = 0;

    if (MemoryChannelInfo[Index].MemoryChannelToken == CM_NULL_TOKEN) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Invalid MemoryChannelToken for Memory Channel %u\n",
        __func__,
        Index
        ));
      Status = EFI_INVALID_PARAMETER;
      goto exitBuildSmbiosType37TableEx;
    }

    if (MemoryChannelInfo[Index].MemoryDeviceListToken == CM_NULL_TOKEN) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Invalid MemoryDeviceListToken for Memory Channel %u\n",
        __func__,
        Index
        ));
      Status = EFI_INVALID_PARAMETER;
      goto exitBuildSmbiosType37TableEx;
    }

    if ((MemoryChannelInfo[Index].ChannelType < MemoryChannelTypeOther) ||
        (MemoryChannelInfo[Index].ChannelType > MemoryChannelTypeSyncLink))
    {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Invalid ChannelType 0x%x for Memory Channel %u\n",
        __func__,
        MemoryChannelInfo[Index].ChannelType,
        Index
        ));
      Status = EFI_INVALID_PARAMETER;
      goto exitBuildSmbiosType37TableEx;
    }

    if (MemoryChannelInfo[Index].MaximumChannelLoad == 0) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: MaximumChannelLoad is zero for Memory Channel %u\n",
        __func__,
        Index
        ));
      Status = EFI_INVALID_PARAMETER;
      goto exitBuildSmbiosType37TableEx;
    }

    //
    // The Type 37 record contains a variable number of Memory Device
    // entries. Fetch the per-channel device list referenced by this
    // Memory Channel CM object.
    //
    Status = GetEArchCommonObjMemoryChannelDevice (
               CfgMgrProtocol,
               MemoryChannelInfo[Index].MemoryDeviceListToken,
               &MemoryChannelDevice,
               &MemoryChannelDeviceCount
               );

    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to get Memory Channel Device CM Object. Status = %r\n",
        __func__,
        Status
        ));
      goto exitBuildSmbiosType37TableEx;
    }

    if (MemoryChannelDeviceCount == 0) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: No Memory Channel Device entries for Memory Channel %u\n",
        __func__,
        Index
        ));
      Status = EFI_INVALID_PARAMETER;
      goto exitBuildSmbiosType37TableEx;
    }

    if (MemoryChannelDeviceCount > MAX_UINT8) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Memory Channel Device count %u exceeds SMBIOS Type 37 limit\n",
        __func__,
        MemoryChannelDeviceCount
        ));
      Status = EFI_INVALID_PARAMETER;
      goto exitBuildSmbiosType37TableEx;
    }

    //
    // SMBIOS_TABLE_TYPE37 already includes one MEMORY_DEVICE entry.
    // Add space only for the remaining entries.
    //
    RecordSize = sizeof (SMBIOS_TABLE_TYPE37) +
                 ((MemoryChannelDeviceCount - 1) * sizeof (MEMORY_DEVICE));

    if (RecordSize > MAX_UINT8) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Type 37 record size %u exceeds SMBIOS header length limit\n",
        __func__,
        RecordSize
        ));
      Status = EFI_INVALID_PARAMETER;
      goto exitBuildSmbiosType37TableEx;
    }

    SmbiosRecord = (SMBIOS_TABLE_TYPE37 *)AllocateSmbiosRecord (
                                            RecordSize,
                                            NULL
                                            );

    if (SmbiosRecord == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto exitBuildSmbiosType37TableEx;
    }

    SmbiosRecord->Hdr.Type           = EFI_SMBIOS_TYPE_MEMORY_CHANNEL;
    SmbiosRecord->Hdr.Length         = (UINT8)RecordSize;
    SmbiosRecord->Hdr.Handle         = SMBIOS_HANDLE_PI_RESERVED;
    SmbiosRecord->ChannelType        = MemoryChannelInfo[Index].ChannelType;
    SmbiosRecord->MaximumChannelLoad = MemoryChannelInfo[Index].MaximumChannelLoad;
    SmbiosRecord->MemoryDeviceCount  = (UINT8)MemoryChannelDeviceCount;

    for (DeviceIndex = 0; DeviceIndex < MemoryChannelDeviceCount; DeviceIndex++) {
      if (MemoryChannelDevice[DeviceIndex].DeviceLoad == 0) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: DeviceLoad is zero for Memory Channel %u Device %u\n",
          __func__,
          Index,
          DeviceIndex
          ));
        Status = EFI_INVALID_PARAMETER;
        goto exitBuildSmbiosType37TableEx;
      }

      if (MemoryChannelDevice[DeviceIndex].MemoryDeviceInfoToken == CM_NULL_TOKEN) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Invalid MemoryDeviceInfoToken for Memory Channel %u Device %u\n",
          __func__,
          Index,
          DeviceIndex
          ));
        Status = EFI_INVALID_PARAMETER;
        goto exitBuildSmbiosType37TableEx;
      }

      //
      // Type 37 references Type 17 Memory Device records by SMBIOS handle.
      // Resolve the CM token to the Type 17 handle generated earlier.
      //
      MemoryDeviceHandle = TableFactoryProtocol->GetSmbiosHandleEx (
                                                   CREATE_STD_SMBIOS_TABLE_GEN_ID (EStdSmbiosTableIdType17),
                                                   MemoryChannelDevice[DeviceIndex].MemoryDeviceInfoToken
                                                   );

      if (MemoryDeviceHandle == SMBIOS_HANDLE_INVALID) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to get Type 17 SMBIOS Handle for Memory Channel %u Device %u\n",
          __func__,
          Index,
          DeviceIndex
          ));
        Status = EFI_NOT_FOUND;
        goto exitBuildSmbiosType37TableEx;
      }

      TotalDeviceLoad += MemoryChannelDevice[DeviceIndex].DeviceLoad;

      SmbiosRecord->MemoryDevice[DeviceIndex].DeviceLoad =
        MemoryChannelDevice[DeviceIndex].DeviceLoad;

      SmbiosRecord->MemoryDevice[DeviceIndex].DeviceHandle =
        MemoryDeviceHandle;
    }

    //
    // Each memory device presents one or more loads to the channel, and
    // the sum of all device loads must not exceed the channel maximum.
    //
    if (TotalDeviceLoad > MemoryChannelInfo[Index].MaximumChannelLoad) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Total DeviceLoad %u exceeds MaximumChannelLoad %u for Memory Channel %u\n",
        __func__,
        TotalDeviceLoad,
        MemoryChannelInfo[Index].MaximumChannelLoad,
        Index
        ));
      Status = EFI_INVALID_PARAMETER;
      goto exitBuildSmbiosType37TableEx;
    }

    //
    // Publish the completed record only after all per-device validation
    // and handle resolution has succeeded.
    //
    TableList[Index]    = (SMBIOS_STRUCTURE *)SmbiosRecord;
    CmObjectList[Index] = MemoryChannelInfo[Index].MemoryChannelToken;

    SmbiosRecord = NULL;
  }

  *Table         = TableList;
  *CmObjectToken = CmObjectList;
  *TableCount    = NumMemChannels;

exitBuildSmbiosType37TableEx:
  if (EFI_ERROR (Status)) {
    if (TableList != NULL) {
      for (Index = 0; Index < NumMemChannels; Index++) {
        if (TableList[Index] != NULL) {
          FreePool (TableList[Index]);
        }
      }

      FreePool (TableList);
    }

    if (CmObjectList != NULL) {
      FreePool (CmObjectList);
    }

    if (SmbiosRecord != NULL) {
      FreePool (SmbiosRecord);
    }
  }

  return Status;
}

/** Free any resources allocated for constructing SMBIOS Type 37 table.

  @param [in]      This                 Pointer to the SMBIOS table generator.
  @param [in]      TableFactoryProtocol Pointer to the SMBIOS table factory
                                           protocol.
  @param [in]      SmbiosTableInfo      Pointer to the SMBIOS table information.
  @param [in]      CfgMgrProtocol       Pointer to the Configuration Manager
                                           Protocol interface.
  @param [in]      Table                Pointer to the SMBIOS table.
  @param [in]      CmObjectToken        Pointer to the CM object token.
  @param [in]      TableCount           Number of generated SMBIOS tables.

  @retval EFI_SUCCESS            Resources freed successfully.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
FreeSmbiosType37TableEx (
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

/** The SMBIOS Type 37 Table Generator.
*/
STATIC CONST SMBIOS_TABLE_GENERATOR  SmbiosType37Generator = {
  // Generator ID
  CREATE_STD_SMBIOS_TABLE_GEN_ID (EStdSmbiosTableIdType37),
  // Generator Description
  L"SMBIOS.TYPE37.GENERATOR",
  // SMBIOS structure type
  SMBIOS_TYPE_MEMORY_CHANNEL,
  NULL,
  NULL,
  // Build table function.
  BuildSmbiosType37TableEx,
  // Free function.
  FreeSmbiosType37TableEx
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
SmbiosType37LibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterSmbiosTableGenerator (&SmbiosType37Generator);
  DEBUG ((DEBUG_INFO, "SMBIOS Type 37: Register Generator. Status = %r\n", Status));
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
SmbiosType37LibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterSmbiosTableGenerator (&SmbiosType37Generator);
  DEBUG ((DEBUG_INFO, "SMBIOS Type 37: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);

  return Status;
}
