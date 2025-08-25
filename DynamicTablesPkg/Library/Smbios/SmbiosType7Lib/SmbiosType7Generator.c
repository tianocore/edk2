/** @file
  SMBIOS Type7 Table Generator.

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

/**
  This macro expands to a function that retrieves common object information from the
  Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjCmRef,
  CM_ARCH_COMMON_OBJ_REF
  );

#define SMBIOS_TYPE7_MAX_STRINGS  (1)

/**
 * Free any resources allocated when installing SMBIOS Type7 table.
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
FreeSmbiosType7TableEx (
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

/** Get SMBIOS system cache type enum from given ACPI cache attributes

 * @param [in]  ACPI cache attributes

 * @return  SMBIOS system cache type
**/
STATIC
CACHE_TYPE_DATA
GetSystemCacheType (
  IN UINT32  Attributes
  )
{
  switch ((Attributes >> 2) & 3) {
    case EFI_ACPI_6_3_CACHE_ATTRIBUTES_CACHE_TYPE_DATA:
      return CacheTypeData;
    case EFI_ACPI_6_3_CACHE_ATTRIBUTES_CACHE_TYPE_INSTRUCTION:
      return CacheTypeInstruction;
    case EFI_ACPI_6_3_CACHE_ATTRIBUTES_CACHE_TYPE_UNIFIED:
      return CacheTypeUnified;
    default:
      return CacheTypeUnknown;
  }
}

/** Get SMBIOS cache associativity enum from a given associativity level

 * @param [in]  Input associativity

 * @return  SMBIOS cache associativity
**/
STATIC
CACHE_ASSOCIATIVITY_DATA
GetCacheAssociativity (
  IN UINT32  Associativity
  )
{
  switch (Associativity) {
    case 1:
      return CacheAssociativityDirectMapped;
    case 2:
      return CacheAssociativity2Way;
    case 4:
      return CacheAssociativity4Way;
    case 8:
      return CacheAssociativity8Way;
    case 12:
      return CacheAssociativity12Way;
    case 16:
      return CacheAssociativity16Way;
    case 20:
      return CacheAssociativity20Way;
    case 24:
      return CacheAssociativity24Way;
    case 32:
      return CacheAssociativity32Way;
    case 48:
      return CacheAssociativity48Way;
    case 64:
      return CacheAssociativity64Way;
    default:
      return CacheAssociativityOther;
  }
}

/** Find all users of a particular cache in the Processor Hierarchy

 * @param [in]  CfgMgrProtocol          Pointer to the Configuration Manager
 *                                      Protocol interface.
 * @param [in]  CacheToken              Token of the cache to search for
 * @param [in]  ProcHierarchyNodeList   Pointer to the Processor Hierarchy node
 *                                      list
 * @param [in]  ProcHierarchyNodeCount  Number of nodes in the Processor
 *                                      Hierarchy node list

 * @return Number of Processor Hierarchy nodes using the given cache on success
 * @return -1 on error
**/
STATIC
INTN
FindCacheUsers (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  IN  CM_OBJECT_TOKEN                                      CacheToken,
  IN  CM_ARCH_COMMON_PROC_HIERARCHY_INFO                   *ProcHierarchyNodeList,
  IN  UINT32                                               ProcHierarchyNodeCount
  )
{
  EFI_STATUS                                Status;
  CM_ARCH_COMMON_PROC_HIERARCHY_INFO CONST  *Node;
  CM_ARCH_COMMON_OBJ_REF                    *PrivResources;
  UINT32                                    Index;
  UINT32                                    Index2;
  UINT32                                    PrivResourcesCount;
  INTN                                      Count = 0;

  for (Index = 0; Index < ProcHierarchyNodeCount; Index++) {
    Node = &ProcHierarchyNodeList[Index];

    if (Node->PrivateResourcesArrayToken == CM_NULL_TOKEN) {
      continue;
    }

    Status = GetEArchCommonObjCmRef (
               CfgMgrProtocol,
               Node->PrivateResourcesArrayToken,
               &PrivResources,
               &PrivResourcesCount
               );

    if (Status == EFI_NOT_FOUND) {
      continue;
    } else if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return -1;
    }

    for (Index2 = 0; Index2 < PrivResourcesCount; Index2++) {
      if (PrivResources[Index2].ReferenceToken == CacheToken) {
        Count++;
      }
    }
  }

  return Count;
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

/** Determine whether the provided cache node is valid for the provided socket node

 * @param [in]  CfgMgrProtocol          Pointer to the Configuration Manager
 *                                      Protocol interface.
 * @param [in]  ProcHierarchyNodeList   Pointer to the Processor Hierarchy node
 *                                      list.
 * @param [in]  ProcHierarchyNodeCount  Number of nodes in the Processor
 *                                      Hierarchy node list.
 * @param [in]  CacheNode               Pointer to the cache node to search for.
 * @param [in]  SocketNode              Pointer to the socket node to search for.

 * @return EFI_SUCCESS if cache node is valid for socket
 * @return EFI_NOT_FOUND if cache node is not valid for socket
**/
STATIC
EFI_STATUS
IsCacheInSocket (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  IN  CM_ARCH_COMMON_PROC_HIERARCHY_INFO   *CONST          ProcHierarchyNodeList,
  IN  UINT32                                               ProcHierarchyNodeCount,
  IN  CM_ARCH_COMMON_CACHE_INFO                            *CacheNode,
  IN  CM_ARCH_COMMON_PROC_HIERARCHY_INFO                   *SocketNode
  )
{
  CM_ARCH_COMMON_PROC_HIERARCHY_INFO  *ProcNode;
  CM_ARCH_COMMON_OBJ_REF              *PrivResources;
  EFI_STATUS                          Status;
  UINTN                               Index;
  UINTN                               ObjIndex;
  UINT32                              PrivResourcesCount;

  // Search for a processor hierarchy node using this cache
  // Cache use will be indicated by the private resource list
  for (Index = 0; Index < ProcHierarchyNodeCount; Index++) {
    ProcNode = &ProcHierarchyNodeList[Index];

    if (ProcNode->PrivateResourcesArrayToken == CM_NULL_TOKEN) {
      continue;
    }

    Status = GetEArchCommonObjCmRef (
               CfgMgrProtocol,
               ProcNode->PrivateResourcesArrayToken,
               &PrivResources,
               &PrivResourcesCount
               );

    if (Status == EFI_NOT_FOUND) {
      continue;
    } else if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    for (ObjIndex = 0; ObjIndex < PrivResourcesCount; ObjIndex++) {
      if (PrivResources[ObjIndex].ReferenceToken != CacheNode->Token) {
        continue;
      }

      // Private resources used by this processor node contain this cache
      // Walk the processor hierarchy to find the socket
      while (ProcNode != NULL && !(ProcNode->Flags & EFI_ACPI_6_3_PPTT_PACKAGE_PHYSICAL)) {
        ProcNode = FindProcHierarchyInfoFromToken (ProcHierarchyNodeList, ProcHierarchyNodeCount, ProcNode->ParentToken);
      }

      if (ProcNode == SocketNode) {
        // Found the socket node, return
        return EFI_SUCCESS;
      }
    }
  }

  return EFI_NOT_FOUND;
}

/** Find an existing type 7 record for a cache on SocketNode of the same level
    as CacheNode.

 * @param [in]  CacheNode           Pointer to cache node currently being
 *                                  processed.
 * @param [in]  SocketNode          Pointer to socket node currently being
 *                                  processed.
 * @param [in]  TableList           Pointer to list of existing type 7 entries.
 * @param [in]  SocketCmObjectList  Pointer to SocketCmObjectList.
 * @param [in]  CmObjectCount       Number of entries in CmObjectList.

 * @retval Pointer to matching type 7 record if one exists.
 * @retval NULL if matching type 7 record does not exist.
**/
STATIC
SMBIOS_TABLE_TYPE7 *
FindExistingCacheRecord (
  IN CM_ARCH_COMMON_CACHE_INFO           *CacheNode,
  IN CM_ARCH_COMMON_PROC_HIERARCHY_INFO  *SocketNode,
  IN SMBIOS_STRUCTURE                    **TableList,
  IN CM_OBJECT_TOKEN                     *SocketCmObjectList,
  IN UINT32                              CmObjectCount
  )
{
  SMBIOS_TABLE_TYPE7        *SmbiosRecord;
  CACHE_CONFIGURATION_DATA  *ConfigurationData;
  UINT32                    Index;

  for (Index = 0; Index < CmObjectCount; Index++) {
    if (SocketCmObjectList[Index] != SocketNode->Token) {
      continue;
    }

    SmbiosRecord      = (SMBIOS_TABLE_TYPE7 *)TableList[Index];
    ConfigurationData = (CACHE_CONFIGURATION_DATA *)&SmbiosRecord->CacheConfiguration;

    if (ConfigurationData->CacheLevel == CacheNode->Level) {
      return SmbiosRecord;
    }
  }

  return NULL;
}

/** Construct SMBIOS Type 7 Table describing caches.

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
BuildSmbiosType7TableEx (
  IN  CONST SMBIOS_TABLE_GENERATOR                         *This,
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL   *CONST  TableFactoryProtocol,
  IN        CM_STD_OBJ_SMBIOS_TABLE_INFO           *CONST  SmbiosTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  OUT       SMBIOS_STRUCTURE                               ***Table,
  OUT       CM_OBJECT_TOKEN                                **CmObjectToken,
  OUT       UINTN                                  *CONST  TableCount
  )
{
  EFI_STATUS                Status;
  SMBIOS_STRUCTURE          **TableList;
  SMBIOS_TABLE_TYPE7        *SmbiosRecord;
  UINTN                     SmbiosRecordSize;
  CM_OBJECT_TOKEN           *CmObjectList;
  CM_OBJECT_TOKEN           *SocketCmObjectList;
  UINTN                     Index;
  UINTN                     ObjIndex;
  UINTN                     SocketIndex;
  UINTN                     SocketListIndex;
  UINTN                     SocketCount;
  CACHE_CONFIGURATION_DATA  *ConfigurationData;

  UINT32  ProcHierarchyNodeCount;
  UINT32  CacheStructCount;
  UINT32  CacheSize;

  CM_ARCH_COMMON_PROC_HIERARCHY_INFO  *ProcHierarchyNodeList;
  CM_ARCH_COMMON_PROC_HIERARCHY_INFO  *SocketNode;
  CM_ARCH_COMMON_CACHE_INFO           *CacheStructList;
  CM_ARCH_COMMON_CACHE_INFO           *CacheNode;

  CM_ARCH_COMMON_PROC_HIERARCHY_INFO  **SocketNodeList;

  STRING_TABLE  StrTable;
  UINT8         SocketDesignationRef;

  INTN  CacheUsers;

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

  TableList          = NULL;
  CmObjectList       = NULL;
  SocketCmObjectList = NULL;
  SocketNodeList     = NULL;

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
    return Status;
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
    return Status;
  }

  if ((Status == EFI_NOT_FOUND) || (CacheStructCount == 0)) {
    // Nothing to do, just return
    return EFI_SUCCESS;
  }

  TableList = (SMBIOS_STRUCTURE **)AllocateZeroPool (sizeof (SMBIOS_STRUCTURE *) * CacheStructCount);
  if (TableList == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to alloc memory for %u devices table\n",
      __func__,
      CacheStructCount
      ));
    Status = EFI_OUT_OF_RESOURCES;
    goto exitErrorBuildSmbiosType7Table;
  }

  CmObjectList = (CM_OBJECT_TOKEN *)AllocateZeroPool (sizeof (CM_OBJECT_TOKEN *) * CacheStructCount);
  if (CmObjectList == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to alloc memory for %u CM Objects.\n",
      __func__,
      CacheStructCount
      ));
    Status = EFI_OUT_OF_RESOURCES;
    goto exitErrorBuildSmbiosType7Table;
  }

  SocketCmObjectList = (CM_OBJECT_TOKEN *)AllocateZeroPool (sizeof (CM_OBJECT_TOKEN *) * CacheStructCount);
  if (SocketCmObjectList == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to alloc memory for %u CM Objects.\n",
      __func__,
      CacheStructCount
      ));
    Status = EFI_OUT_OF_RESOURCES;
    goto exitErrorBuildSmbiosType7Table;
  }

  // Find number of sockets
  SocketCount = 0;
  for (Index = 0; Index < ProcHierarchyNodeCount; Index++) {
    SocketNode = &ProcHierarchyNodeList[Index];

    if (SocketNode->Flags & EFI_ACPI_6_3_PPTT_PACKAGE_PHYSICAL) {
      SocketCount++;
    }
  }

  SocketNodeList = (CM_ARCH_COMMON_PROC_HIERARCHY_INFO **)AllocateZeroPool (sizeof (CM_ARCH_COMMON_PROC_HIERARCHY_INFO *) * SocketCount);
  SocketCount    = 0;

  for (Index = 0; Index < ProcHierarchyNodeCount; Index++) {
    SocketNode = &ProcHierarchyNodeList[Index];

    if (SocketNode->Flags & EFI_ACPI_6_3_PPTT_PACKAGE_PHYSICAL) {
      SocketNodeList[SocketCount] = SocketNode;
      SocketCount++;
    }
  }

  ObjIndex = 0;

  for (Index = 0; Index < CacheStructCount; Index++) {
    CacheNode = &CacheStructList[Index];

    for (SocketListIndex = 0; SocketListIndex < SocketCount; SocketListIndex++) {
      SocketNode = SocketNodeList[SocketListIndex];

      Status = IsCacheInSocket (CfgMgrProtocol, ProcHierarchyNodeList, ProcHierarchyNodeCount, CacheNode, SocketNode);

      if (Status == EFI_NOT_FOUND) {
        continue;
      }

      if (EFI_ERROR (Status)) {
        ASSERT (!EFI_ERROR (Status));
        goto exitErrorBuildSmbiosType7Table;
      }

      SocketIndex = ((UINTN)SocketNode - (UINTN)ProcHierarchyNodeList) / sizeof (*SocketNode);

      // A Type 7 table entry contains all caches at a particular level for a
      // socket. Check for an existing type 7 table entry for this cache level on
      // this socket.
      SmbiosRecord = FindExistingCacheRecord (
                       CacheNode,
                       SocketNode,
                       TableList,
                       SocketCmObjectList,
                       ObjIndex
                       );

      if (SmbiosRecord) {
        // Previous cache found, merge this cache with the previous entry
        CacheUsers = FindCacheUsers (CfgMgrProtocol, CacheNode->Token, ProcHierarchyNodeList, ProcHierarchyNodeCount);
        if (CacheUsers < 0) {
          ASSERT (CacheUsers >= 0);
          Status = EFI_INVALID_PARAMETER;
          goto exitErrorBuildSmbiosType7Table;
        }

        CacheSize = CacheNode->Size * CacheUsers;

        SmbiosRecord->MaximumCacheSize2.Size += CacheSize / 1024;

        if (((GetSystemCacheType (CacheNode->Attributes) == CacheTypeData) &&
             ((SmbiosRecord->SystemCacheType == CacheTypeInstruction) || (SmbiosRecord->SystemCacheType == CacheTypeUnified))) ||
            ((GetSystemCacheType (CacheNode->Attributes) == CacheTypeInstruction) &&
             ((SmbiosRecord->SystemCacheType == CacheTypeData) || (SmbiosRecord->SystemCacheType == CacheTypeUnified))))
        {
          SmbiosRecord->SystemCacheType = CacheTypeUnified;
        } else if (GetSystemCacheType (CacheNode->Attributes) != SmbiosRecord->SystemCacheType) {
          ASSERT (GetSystemCacheType (CacheNode->Attributes) == SmbiosRecord->SystemCacheType);
          SmbiosRecord->SystemCacheType = CacheTypeOther;
        }

        SmbiosRecord->Associativity = CacheAssociativityOther;
      } else {
        // No previously seen cache at this level, create new table entry
        StringTableInitialize (&StrTable, SMBIOS_TYPE7_MAX_STRINGS);
        SocketDesignationRef = 0;

        if (CacheNode->SocketDesignation[0]) {
          Status = StringTableAddString (&StrTable, CacheNode->SocketDesignation, &SocketDesignationRef);
          if (EFI_ERROR (Status)) {
            DEBUG ((DEBUG_ERROR, "Failed to add Socket Designation String %r\n", Status));
            goto exitErrorBuildSmbiosType7Table;
          }
        }

        SmbiosRecordSize = sizeof (SMBIOS_TABLE_TYPE7) + StringTableGetStringSetSize (&StrTable);
        SmbiosRecord     = (SMBIOS_TABLE_TYPE7 *)AllocateZeroPool (SmbiosRecordSize);

        // Set up the header
        SmbiosRecord->Hdr.Type   = EFI_SMBIOS_TYPE_CACHE_INFORMATION;
        SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE7);

        SmbiosRecord->SocketDesignation = SocketDesignationRef;

        ConfigurationData                = (CACHE_CONFIGURATION_DATA *)&SmbiosRecord->CacheConfiguration;
        ConfigurationData->CacheLevel    = CacheNode->Level;
        ConfigurationData->CacheSocketed = 0;
        ConfigurationData->Location      = 0;
        ConfigurationData->Enabled       = 1;
        if (((CacheNode->Attributes >> 4) & 1) == EFI_ACPI_6_3_CACHE_ATTRIBUTES_WRITE_POLICY_WRITE_BACK) {
          ConfigurationData->OperationMode = 1;
        } else {
          ConfigurationData->OperationMode = 0;
        }

        CacheUsers = FindCacheUsers (CfgMgrProtocol, CacheNode->Token, ProcHierarchyNodeList, ProcHierarchyNodeCount);
        if (CacheUsers < 0) {
          ASSERT (CacheUsers >= 0);
          Status = EFI_INVALID_PARAMETER;
          goto exitErrorBuildSmbiosType7Table;
        }

        CacheSize = CacheNode->Size * CacheUsers;

        // Store cache size in MaximumCacheSize2.Size in 1K granularity. This will be
        // processed once all caches have been accumulated.
        SmbiosRecord->MaximumCacheSize2.Size = CacheSize / 1024;

        SmbiosRecord->SupportedSRAMType.Unknown = 1;
        SmbiosRecord->CurrentSRAMType.Unknown   = 1;
        SmbiosRecord->ErrorCorrectionType       = CacheErrorUnknown;

        SmbiosRecord->SystemCacheType = GetSystemCacheType (CacheNode->Attributes);
        SmbiosRecord->Associativity   = GetCacheAssociativity (CacheNode->Associativity);

        StringTablePublishStringSet (&StrTable, (CHAR8 *)(SmbiosRecord + 1), SmbiosRecordSize - sizeof (SMBIOS_TABLE_TYPE7));

        TableList[ObjIndex]          = (SMBIOS_STRUCTURE *)SmbiosRecord;
        CmObjectList[ObjIndex]       = CM_ABSTRACT_TOKEN_MAKE (ETokenNameSpaceSmbios, EStdSmbiosTableIdType07, CacheNode->Level | (SocketIndex << 2));
        SocketCmObjectList[ObjIndex] = SocketNode->Token;
        ObjIndex++;
      }
    }
  }

  // Handle cache size encoding
  for (Index = 0; Index < ObjIndex; Index++) {
    SmbiosRecord = (SMBIOS_TABLE_TYPE7 *)TableList[Index];
    // Accumulated cache size in 1K granularity
    CacheSize = SmbiosRecord->MaximumCacheSize2.Size;

    if (CacheSize > SMBIOS_CACHE_SIZE_MAX_SIZE_64K_GRANULARITY) {
      // Cache size greater than can be represented in MaximumCacheSize at 64K granularity
      SmbiosRecord->MaximumCacheSize2.Granularity64K = 1;
      SmbiosRecord->MaximumCacheSize2.Size           = (CacheSize / 64);

      SmbiosRecord->MaximumCacheSize.Granularity64K = 1;
      SmbiosRecord->MaximumCacheSize.Size           = 0x7fff;
    } else if (CacheSize > SMBIOS_CACHE_SIZE_MAX_SIZE_1K_GRANULARITY) {
      // Cache size greater than can be represented in MaximumCacheSize at 1K granularity
      SmbiosRecord->MaximumCacheSize2.Granularity64K = 1;
      SmbiosRecord->MaximumCacheSize2.Size           = (CacheSize / 64);

      SmbiosRecord->MaximumCacheSize.Granularity64K = 1;
      SmbiosRecord->MaximumCacheSize.Size           = (CacheSize / 64);
    } else {
      SmbiosRecord->MaximumCacheSize2.Granularity64K = 0;
      SmbiosRecord->MaximumCacheSize2.Size           = CacheSize;

      SmbiosRecord->MaximumCacheSize.Granularity64K = 0;
      SmbiosRecord->MaximumCacheSize.Size           = CacheSize;
    }

    SmbiosRecord->InstalledSize  = SmbiosRecord->MaximumCacheSize;
    SmbiosRecord->InstalledSize2 = SmbiosRecord->MaximumCacheSize2;
  }

  *Table         = TableList;
  *CmObjectToken = CmObjectList;
  *TableCount    = ObjIndex;
  FreePool (SocketNodeList);
  FreePool (SocketCmObjectList);

  return EFI_SUCCESS;

exitErrorBuildSmbiosType7Table:
  if (SocketNodeList) {
    FreePool (SocketNodeList);
  }

  if (TableList) {
    FreePool (TableList);
  }

  if (CmObjectList) {
    FreePool (CmObjectList);
  }

  if (SocketCmObjectList) {
    FreePool (SocketCmObjectList);
  }

  return Status;
}

/** The interface for the SMBIOS Type7 Table Generator.
*/
STATIC
CONST
SMBIOS_TABLE_GENERATOR  SmbiosType7Generator = {
  // Generator ID
  CREATE_STD_SMBIOS_TABLE_GEN_ID (EStdSmbiosTableIdType07),
  // Generator Description
  L"SMBIOS.TYPE7.GENERATOR",
  // SMBIOS Table Type
  EFI_SMBIOS_TYPE_CACHE_INFORMATION,
  NULL,
  NULL,
  // Build table function.
  BuildSmbiosType7TableEx,
  // Free function.
  FreeSmbiosType7TableEx,
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
SmbiosType7LibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterSmbiosTableGenerator (&SmbiosType7Generator);
  DEBUG ((
    DEBUG_INFO,
    "SMBIOS Type 7: Register Generator. Status = %r\n",
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
SmbiosType7LibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterSmbiosTableGenerator (&SmbiosType7Generator);
  DEBUG ((
    DEBUG_INFO,
    "SMBIOS Type 7: Deregister Generator. Status = %r\n",
    Status
    ));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
