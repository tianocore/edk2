/** @file
  SMBIOS Type4 Table Generator.

  Copyright (c) 2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
  Copyright (c) 2020 - 2025, Arm Limited. All rights reserved.<BR>

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
#include <IndustryStandard/SmBios.h>

/**
  This macro expands to a function that retrieves the Processor Hierarchy
  information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjProcHierarchyInfo,
  CM_ARCH_COMMON_PROC_HIERARCHY_INFO
  );

/**
  This macro expands to a function that retrieves the Cache information from the
  Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjCacheInfo,
  CM_ARCH_COMMON_CACHE_INFO
  );

#define SMBIOS_TYPE4_MAX_STRINGS  (7)

/**
 * Free any resources allocated when installing SMBIOS Type4 table.
 *
 * @param [in]  This                 Pointer to the SMBIOS table generator.
 * @param [in]  TableFactoryProtocol Pointer to the SMBIOS Table Factory
                                      Protocol interface.

 * @param [in]  SmbiosTableInfo      Pointer to the SMBIOS table information.
 * @param [in]  CfgMgrProtocol       Pointer to the Configuration Manager
                                     Protocol interface.
 * @param [in] Table                 Pointer to the SMBIOS table.
 * @param [in] CmObjectToken         Pointer to the CM ObjectToken Array.
 * @param [in] TableCount            Number of SMBIOS tables.

 * @retval EFI_SUCCESS            Table generated successfully.
 * @retval EFI_BAD_BUFFER_SIZE    The size returned by the Configuration
                                  Manager is less than the Object size for
                                  the requested object.
 * @retval EFI_INVALID_PARAMETER  A parameter is invalid.
 * @retval EFI_NOT_FOUND          Could not find information.
 * @retval EFI_OUT_OF_RESOURCES   Could not allocate memory.
 * @retval EFI_UNSUPPORTED        Unsupported configuration.
**/
STATIC
EFI_STATUS
FreeSmbiosType4TableEx (
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

/** Find the Processor Hierarchy node from the associated token.

 * @param [in]  ProcHierarchyNodeList   Pointer to the Processor Hierarchy node
 *                                      list.
 * @param [in]  ProcHierarchyNodeCount  Number of nodes in the Processor
 *                                      Hierarchy node list.
 * @param [in]  Token                   The token to search for.

 * @return Pointer to the Processor hierarchy node.
 * @return NULL if token is not in the Processor Hierarchy node list.
**/
STATIC
CM_ARCH_COMMON_PROC_HIERARCHY_INFO *
FindProcHierarchyInfoFromToken (
  CM_ARCH_COMMON_PROC_HIERARCHY_INFO  *ProcHierarchyNodeList,
  UINT32                              ProcHierarchyNodeCount,
  CM_OBJECT_TOKEN                     Token
  )
{
  UINTN  Index;

  for (Index = 0; Index < ProcHierarchyNodeCount; Index++) {
    if (ProcHierarchyNodeList[Index].Token == Token) {
      return &ProcHierarchyNodeList[Index];
    }
  }

  ASSERT (0);
  return NULL;
}

/** Construct SMBIOS Type 4 Table describing CPU devices.

  If this function allocates any resources then they must be freed
  in the FreeXXXXTableResources function.

 * @param [in]  This                 Pointer to the SMBIOS table generator.
 * @param [in]  TableFactoryProtocol Pointer to the SMBIOS Table Factory
 *                                   Protocol interface.
 * @param [in]  SmbiosTableInfo      Pointer to the SMBIOS table information.
 * @param [in]  CfgMgrProtocol       Pointer to the Configuration Manager
 *                                   Protocol interface.
 * @param [out] Table                Pointer to the SMBIOS table.
 * @param [out] CmObjectToken        Pointer to the CM Object Token Array.
 * @param [out] TableCount           Number of tables installed.

 * @retval EFI_SUCCESS            Table generated successfully.
 * @retval EFI_BAD_BUFFER_SIZE    The size returned by the Configuration
 *                                Manager is less than the Object size for
 *                                the requested object.
 * @retval EFI_INVALID_PARAMETER  A parameter is invalid.
 * @retval EFI_NOT_FOUND          Could not find information.
 * @retval EFI_OUT_OF_RESOURCES   Could not allocate memory.
 * @retval EFI_UNSUPPORTED        Unsupported configuration.
**/
STATIC
EFI_STATUS
BuildSmbiosType4TableEx (
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
  SMBIOS_STRUCTURE                **TableList = NULL;
  SMBIOS_TABLE_TYPE4              *SmbiosRecord;
  UINTN                           SmbiosRecordSize;
  UINTN                           Index;
  UINTN                           ObjIndex;
  UINTN                           CpuIndex;
  UINTN                           CpuIndex2;
  PROCESSOR_STATUS_DATA           *StatusData;
  PROCESSOR_CHARACTERISTIC_FLAGS  *CharacteristicFlags;

  UINT32  ProcHierarchyNodeCount;
  UINT32  CacheStructCount;
  UINT32  SocketCount;
  UINT32  CpuCount;
  UINT32  ThreadCount;

  EFI_ACPI_6_3_PPTT_STRUCTURE_PROCESSOR_FLAGS  *Flags;
  CM_ARCH_COMMON_PROC_HIERARCHY_INFO           *ProcHierarchyNodeList;
  CM_ARCH_COMMON_PROC_HIERARCHY_INFO           *Node;
  CM_ARCH_COMMON_CACHE_INFO                    *CacheStructList;

  STRING_TABLE  StrTable;
  UINT8         SocketDesignationRef;
  UINT8         ProcessorManufacturerRef;
  UINT8         ProcessorVersionRef;
  UINT8         SerialNumberRef;
  UINT8         AssetTagRef;
  UINT8         PartNumberRef;
  UINT8         SocketTypeRef;

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
    DEBUG ((DEBUG_ERROR, "%a:Invalid Paramater\n ", __func__));
    return EFI_INVALID_PARAMETER;
  }

  *Table = NULL;

  // Get the processor hierarchy info and update the processor topology
  // structure count with Processor Hierarchy Nodes (Type 0)
  Status = GetEArchCommonObjProcHierarchyInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &ProcHierarchyNodeList,
             &ProcHierarchyNodeCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get processor hierarchy info. Status = %r\n",
      __func__,
      Status
      ));
    return EFI_INVALID_PARAMETER;
  }

  // Get the cache info and update the processor topology structure count with
  // Cache Type Structures (Type 1)
  Status = GetEArchCommonObjCacheInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &CacheStructList,
             &CacheStructCount
             );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get cache info. Status = %r\n",
      __func__,
      Status
      ));
    return EFI_INVALID_PARAMETER;
  }

  // Count the number of sockets in the current hierarchy. This will determine
  // the number of type 4 records that will be generated.
  SocketCount = 0;
  for (Index = 0; Index < ProcHierarchyNodeCount; Index++) {
    if (ProcHierarchyNodeList[Index].Flags & EFI_ACPI_6_3_PPTT_PACKAGE_PHYSICAL) {
      SocketCount++;
    }
  }

  if (SocketCount == 0) {
    ASSERT (SocketCount != 0);
    return EFI_INVALID_PARAMETER;
  }

  TableList = (SMBIOS_STRUCTURE **)AllocateZeroPool (sizeof (SMBIOS_STRUCTURE *) * SocketCount);
  if (TableList == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to alloc memory for %u devices table\n",
      __func__,
      SocketCount
      ));
    Status = EFI_OUT_OF_RESOURCES;
    goto exitErrorBuildSmbiosType4Table;
  }

  ObjIndex = 0;

  for (Index = 0; Index < ProcHierarchyNodeCount; Index++) {
    if ((ProcHierarchyNodeList[Index].Flags & EFI_ACPI_6_3_PPTT_PACKAGE_PHYSICAL) == 0) {
      continue;
    }

    StringTableInitialize (&StrTable, SMBIOS_TYPE4_MAX_STRINGS);

    SocketDesignationRef     = 0;
    ProcessorManufacturerRef = 0;
    ProcessorVersionRef      = 0;
    SerialNumberRef          = 0;
    AssetTagRef              = 0;
    PartNumberRef            = 0;
    SocketTypeRef            = 0;

    if (ProcHierarchyNodeList[Index].SocketDesignation[0]) {
      Status = StringTableAddString (&StrTable, ProcHierarchyNodeList[Index].SocketDesignation, &SocketDesignationRef);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Failed to add Socket Designation String %r\n", Status));
        ASSERT (!EFI_ERROR (Status));
        goto exitErrorBuildSmbiosType4Table;
      }
    }

    if (ProcHierarchyNodeList[Index].ProcessorManufacturer[0]) {
      Status = StringTableAddString (&StrTable, ProcHierarchyNodeList[Index].ProcessorManufacturer, &ProcessorManufacturerRef);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Failed to add Processor Manufacturer String %r\n", Status));
        ASSERT (!EFI_ERROR (Status));
        goto exitErrorBuildSmbiosType4Table;
      }
    }

    if (ProcHierarchyNodeList[Index].ProcessorVersion[0]) {
      Status = StringTableAddString (&StrTable, ProcHierarchyNodeList[Index].ProcessorVersion, &ProcessorVersionRef);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Failed to add Processor Version String %r\n", Status));
        ASSERT (!EFI_ERROR (Status));
        goto exitErrorBuildSmbiosType4Table;
      }
    }

    if (ProcHierarchyNodeList[Index].SerialNumber[0]) {
      Status = StringTableAddString (&StrTable, ProcHierarchyNodeList[Index].SerialNumber, &SerialNumberRef);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Failed to add Serial Number String %r\n", Status));
        ASSERT (!EFI_ERROR (Status));
        goto exitErrorBuildSmbiosType4Table;
      }
    }

    if (ProcHierarchyNodeList[Index].AssetTag[0]) {
      Status = StringTableAddString (&StrTable, ProcHierarchyNodeList[Index].AssetTag, &AssetTagRef);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Failed to add Asset Tag String %r\n", Status));
        ASSERT (!EFI_ERROR (Status));
        goto exitErrorBuildSmbiosType4Table;
      }
    }

    if (ProcHierarchyNodeList[Index].PartNumber[0]) {
      Status = StringTableAddString (&StrTable, ProcHierarchyNodeList[Index].PartNumber, &PartNumberRef);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Failed to add Part Number String %r\n", Status));
        ASSERT (!EFI_ERROR (Status));
        goto exitErrorBuildSmbiosType4Table;
      }
    }

    if (ProcHierarchyNodeList[Index].SocketType[0]) {
      Status = StringTableAddString (&StrTable, ProcHierarchyNodeList[Index].SocketType, &SocketTypeRef);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Failed to add Socket Type String %r\n", Status));
        ASSERT (!EFI_ERROR (Status));
        goto exitErrorBuildSmbiosType4Table;
      }
    }

    SmbiosRecordSize = sizeof (SMBIOS_TABLE_TYPE4) + StringTableGetStringSetSize (&StrTable);
    SmbiosRecord     = (SMBIOS_TABLE_TYPE4 *)AllocateZeroPool (SmbiosRecordSize);
    if (SmbiosRecord == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto exitErrorBuildSmbiosType4Table;
    }

    // Set up the header
    SmbiosRecord->Hdr.Type   = EFI_SMBIOS_TYPE_PROCESSOR_INFORMATION;
    SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE4);

    CpuCount    = 0;
    ThreadCount = 0;

    SmbiosRecord->L1CacheHandle = FindSmbiosHandleEx (
                                    CREATE_STD_SMBIOS_TABLE_GEN_ID (EStdSmbiosTableIdType07),
                                    CM_ABSTRACT_TOKEN_MAKE (ETokenNameSpaceSmbios, EStdSmbiosTableIdType07, 0 | (Index << 2))
                                    );
    SmbiosRecord->L2CacheHandle = FindSmbiosHandleEx (
                                    CREATE_STD_SMBIOS_TABLE_GEN_ID (EStdSmbiosTableIdType07),
                                    CM_ABSTRACT_TOKEN_MAKE (ETokenNameSpaceSmbios, EStdSmbiosTableIdType07, 1 | (Index << 2))
                                    );
    SmbiosRecord->L3CacheHandle = FindSmbiosHandleEx (
                                    CREATE_STD_SMBIOS_TABLE_GEN_ID (EStdSmbiosTableIdType07),
                                    CM_ABSTRACT_TOKEN_MAKE (ETokenNameSpaceSmbios, EStdSmbiosTableIdType07, 2 | (Index << 2))
                                    );

    // Find all CPUs on this socket in order to set the cache handles
    for (CpuIndex = 0; CpuIndex < ProcHierarchyNodeCount; CpuIndex++) {
      Node  = &ProcHierarchyNodeList[CpuIndex];
      Flags = (EFI_ACPI_6_3_PPTT_STRUCTURE_PROCESSOR_FLAGS *)&Node->Flags;

      if (!Flags->NodeIsALeaf) {
        continue;
      }

      // Walk through ParentToken to find the socket node
      while (!Flags->PhysicalPackage) {
        Node = FindProcHierarchyInfoFromToken (ProcHierarchyNodeList, ProcHierarchyNodeCount, Node->ParentToken);

        if (Node == NULL) {
          ASSERT (Node != NULL);
          Status = EFI_INVALID_PARAMETER;
          goto exitErrorBuildSmbiosType4Table;
        }

        Flags = (EFI_ACPI_6_3_PPTT_STRUCTURE_PROCESSOR_FLAGS *)&Node->Flags;
      }

      // Is this CPU on the current socket?
      if (Node != &ProcHierarchyNodeList[Index]) {
        continue;
      }

      Node  = &ProcHierarchyNodeList[CpuIndex];
      Flags = (EFI_ACPI_6_3_PPTT_STRUCTURE_PROCESSOR_FLAGS *)&Node->Flags;
      if (Flags->ProcessorIsAThread) {
        // Only increment CPU count on the first leaf node
        for (CpuIndex2 = 0; CpuIndex2 < CpuIndex; CpuIndex2++) {
          if (ProcHierarchyNodeList[CpuIndex2].ParentToken == Node->ParentToken) {
            break;
          }
        }

        if (CpuIndex == CpuIndex2) {
          CpuCount++;
        }
      } else {
        CpuCount++;
      }

      ThreadCount++;
    }

    SmbiosRecord->ProcessorType    = CentralProcessor;
    SmbiosRecord->ProcessorUpgrade = ProcessorUpgradeUnknown;
 #if defined (MDE_CPU_AARCH64)
    SmbiosRecord->ProcessorFamily          = ProcessorFamilyIndicatorFamily2;
    SmbiosRecord->ProcessorFamily2         = ProcessorFamilyARMv8;
    SmbiosRecord->ProcessorId.SocId        = ProcHierarchyNodeList[Index].ProcessorId & 0xffff;
    SmbiosRecord->ProcessorId.SipId        = (ProcHierarchyNodeList[Index].ProcessorId >> 16) & 0xff;
    SmbiosRecord->ProcessorId.SipBankIndex = (ProcHierarchyNodeList[Index].ProcessorId >> 24) & 0xff;
    SmbiosRecord->ProcessorId.SocRevision  = ProcHierarchyNodeList[Index].ProcessorId >> 32;
 #else
    DEBUG ((DEBUG_ERROR, "SmbiosType4Generator needs extending for your CPU\n"));
 #endif

    StatusData                       = (PROCESSOR_STATUS_DATA *)&SmbiosRecord->Status;
    StatusData->Bits.SocketPopulated = 1;

    CharacteristicFlags = (PROCESSOR_CHARACTERISTIC_FLAGS *)&SmbiosRecord->ProcessorCharacteristics;
 #if defined (MDE_CPU_AARCH64)
    CharacteristicFlags->Processor64BitCapable = 1;
 #endif
    CharacteristicFlags->ProcessorMultiCore      = (CpuCount > 1) ? 1 : 0;
    CharacteristicFlags->ProcessorHardwareThread = (ThreadCount > CpuCount) ? 1 : 0;

    SmbiosRecord->CoreCount         = (CpuCount < 256) ? CpuCount : 0xff;
    SmbiosRecord->CoreCount2        = CpuCount;
    SmbiosRecord->EnabledCoreCount  = (CpuCount < 256) ? CpuCount : 0xff;
    SmbiosRecord->EnabledCoreCount2 = CpuCount;
    SmbiosRecord->ThreadCount       = (ThreadCount < 256) ? ThreadCount : 0xff;
    SmbiosRecord->ThreadCount2      = ThreadCount;
    SmbiosRecord->ThreadEnabled     = ThreadCount;

    SmbiosRecord->Socket                = SocketDesignationRef;
    SmbiosRecord->ProcessorManufacturer = ProcessorManufacturerRef;
    SmbiosRecord->ProcessorVersion      = ProcessorVersionRef;
    SmbiosRecord->SerialNumber          = SerialNumberRef;
    SmbiosRecord->AssetTag              = AssetTagRef;
    SmbiosRecord->PartNumber            = PartNumberRef;
    SmbiosRecord->SocketType            = SocketTypeRef;

    StringTablePublishStringSet (&StrTable, (CHAR8 *)(SmbiosRecord + 1), SmbiosRecordSize - sizeof (SMBIOS_TABLE_TYPE4));

    TableList[ObjIndex] = (SMBIOS_STRUCTURE *)SmbiosRecord;
    ObjIndex++;
    ASSERT (ObjIndex <= SocketCount);
  }

  ASSERT (ObjIndex == SocketCount);

  *Table         = TableList;
  *CmObjectToken = NULL;
  *TableCount    = SocketCount;

  return EFI_SUCCESS;

exitErrorBuildSmbiosType4Table:
  if (TableList) {
    FreePool (TableList);
  }

  return Status;
}

/** The interface for the SMBIOS Type4 Table Generator.
*/
STATIC
CONST
SMBIOS_TABLE_GENERATOR  SmbiosType4Generator = {
  // Generator ID
  CREATE_STD_SMBIOS_TABLE_GEN_ID (EStdSmbiosTableIdType04),
  // Generator Description
  L"SMBIOS.TYPE4.GENERATOR",
  // SMBIOS Table Type
  EFI_SMBIOS_TYPE_PROCESSOR_INFORMATION,
  NULL,
  NULL,
  // Build table function.
  BuildSmbiosType4TableEx,
  // Free function.
  FreeSmbiosType4TableEx,
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
SmbiosType4LibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterSmbiosTableGenerator (&SmbiosType4Generator);
  DEBUG ((
    DEBUG_INFO,
    "SMBIOS Type 4: Register Generator. Status = %r\n",
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
SmbiosType4LibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterSmbiosTableGenerator (&SmbiosType4Generator);
  DEBUG ((
    DEBUG_INFO,
    "SMBIOS Type 4: Deregister Generator. Status = %r\n",
    Status
    ));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
