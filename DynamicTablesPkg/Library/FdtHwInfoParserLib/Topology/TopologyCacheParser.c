/** @file
  Cache topology parser.

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/FdtLib.h>
#include <Library/PrintLib.h>
#include <IndustryStandard/Acpi63.h>

#include "CmObjectDescUtility.h"
#include "Topology/TopologyHierarchyParser.h"
#include "Topology/TopologyParser.h"
#include "Topology/TopologyUtility.h"

/** Encode PPTT cache attributes from their allocation/type/policy fields. */
#define BUILD_CACHE_ATTRIBUTES(AllocationType, CacheType, WritePolicy) \
  ((UINT8)((AllocationType) | ((CacheType) << 2) | ((WritePolicy) << 4)))

//
// By default, account for at most 4 slots per CPU:
// L1D, L1C, L2, L3
//
#define MAX_CACHES_PER_CPU  (4)

/** Indexes into the per-cache property-name tables. */
typedef enum {
  CachePropertyNameSize = 0,
  CachePropertyNameSets,
  CachePropertyNameLineSize,
  CachePropertyNameMax
} CACHE_PROPERTY_NAME_INDEX;

/** Distinguish instruction, data, and unified CPU-local cache descriptions. */
typedef enum {
  CpuCachePropertyInstruction = 0,
  CpuCachePropertyData,
  CpuCachePropertyShared,
  CpuCachePropertyMax
} CPU_CACHE_PROPERTY_TYPE;

/** Map a DT cache property onto a CM_ARCH_COMMON_CACHE_INFO field. */
typedef struct {
  CONST CHAR8    *SharedPropertyName;
  UINTN          FieldOffset;
  UINTN          FieldSize;
} CACHE_INFO_PROPERTY_MAP;

/** Map an ACPI cache type to the default SMBIOS designation format. */
typedef struct {
  UINT8          CacheType;
  CONST CHAR8    *DesignationFormat;
} CACHE_TYPE_DESIGNATION_MAP;

/** DT property names for instruction, data, and unified CPU cache properties. */
STATIC CONST CHAR8  *CONST  mCpuCachePropertyNames[CpuCachePropertyMax][CachePropertyNameMax] = {
  {
    "i-cache-size",
    "i-cache-sets",
    "i-cache-line-size",
  },
  {
    "d-cache-size",
    "d-cache-sets",
    "d-cache-line-size",
  },
  {
    "cache-size",
    "cache-sets",
    "cache-line-size",
  }
};

/** Mapping from shared-cache DT property names to cache-info structure fields. */
STATIC CONST CACHE_INFO_PROPERTY_MAP  mCacheInfoPropertyMap[CachePropertyNameMax] = {
  {
    "cache-size",
    OFFSET_OF (CM_ARCH_COMMON_CACHE_INFO, Size),
    sizeof (((CM_ARCH_COMMON_CACHE_INFO *)0)->Size)
  },
  {
    "cache-sets",
    OFFSET_OF (CM_ARCH_COMMON_CACHE_INFO, NumberOfSets),
    sizeof (((CM_ARCH_COMMON_CACHE_INFO *)0)->NumberOfSets)
  },
  {
    "cache-line-size",
    OFFSET_OF (CM_ARCH_COMMON_CACHE_INFO, LineSize),
    sizeof (((CM_ARCH_COMMON_CACHE_INFO *)0)->LineSize)
  },
};

/** Default cache designation strings used when DT does not provide one. */
STATIC CONST CACHE_TYPE_DESIGNATION_MAP  mCacheTypeDesignationMap[CpuCachePropertyMax] = {
  {
    EFI_ACPI_6_3_CACHE_ATTRIBUTES_CACHE_TYPE_INSTRUCTION,
    "L%u I-Cache"
  },
  {
    EFI_ACPI_6_3_CACHE_ATTRIBUTES_CACHE_TYPE_DATA,
    "L%u D-Cache"
  },
  {
    EFI_ACPI_6_3_CACHE_ATTRIBUTES_CACHE_TYPE_UNIFIED,
    "L%u Cache"
  }
};

/** Precomputed PPTT cache-attribute bytes for each CPU cache-property type. */
STATIC CONST UINT8  mCpuCacheAttributes[CpuCachePropertyMax] = {
  BUILD_CACHE_ATTRIBUTES (
    EFI_ACPI_6_3_CACHE_ATTRIBUTES_ALLOCATION_READ,
    EFI_ACPI_6_3_CACHE_ATTRIBUTES_CACHE_TYPE_INSTRUCTION,
    EFI_ACPI_6_3_CACHE_ATTRIBUTES_WRITE_POLICY_WRITE_BACK
    ),
  BUILD_CACHE_ATTRIBUTES (
    EFI_ACPI_6_3_CACHE_ATTRIBUTES_ALLOCATION_READ_WRITE,
    EFI_ACPI_6_3_CACHE_ATTRIBUTES_CACHE_TYPE_DATA,
    EFI_ACPI_6_3_CACHE_ATTRIBUTES_WRITE_POLICY_WRITE_BACK
    ),
  BUILD_CACHE_ATTRIBUTES (
    EFI_ACPI_6_3_CACHE_ATTRIBUTES_ALLOCATION_READ_WRITE,
    EFI_ACPI_6_3_CACHE_ATTRIBUTES_CACHE_TYPE_UNIFIED,
    EFI_ACPI_6_3_CACHE_ATTRIBUTES_WRITE_POLICY_WRITE_BACK
    )
};

/** Create a deterministic abstract token for a cache node.

  @param [in] Index  Zero-based cache node index.

  @retval A CM_OBJECT_TOKEN in the FDT topology abstract-token namespace.
**/
STATIC
CM_OBJECT_TOKEN
EFIAPI
CreateCacheToken (
  IN UINT32  Index
  )
{
  return (CM_OBJECT_TOKEN)CM_ABSTRACT_TOKEN_MAKE (
                            ETokenNameSpaceFdtHwInfo,
                            EFdtHwInfoCacheObject,
                            Index + 1
                            );
}

/** Allocate cache buffers for the whole topology parse.

  The parser uses a fixed-size cache buffer array sized from the already-built
  processor hierarchy.

  @param [in, out] Context  Topology parser context.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Memory allocation failed.
**/
STATIC
EFI_STATUS
EFIAPI
AllocateCacheBuffers (
  IN OUT TOPOLOGY_PARSER_CONTEXT  *Context
  )
{
  TOPOLOGY_CACHE_BUFFERS  *NewCacheBuffers;
  UINT32                  CacheCapacity;
  UINT32                  CopyCount;

  if (Context == NULL) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  CacheCapacity = Context->ProcHierarchyCount * MAX_CACHES_PER_CPU;
  if (CacheCapacity == 0) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  NewCacheBuffers = AllocateZeroPool (sizeof (*NewCacheBuffers) * CacheCapacity);
  if (NewCacheBuffers == NULL) {
    ASSERT (FALSE);
    return EFI_OUT_OF_RESOURCES;
  }

  for (CopyCount = 0; CopyCount < CacheCapacity; CopyCount++) {
    NewCacheBuffers[CopyCount].DtNode         = -1;
    NewCacheBuffers[CopyCount].OwnerProcIndex = TOPOLOGY_INVALID_INDEX;
  }

  Context->CacheBuffers  = NewCacheBuffers;
  Context->CacheCapacity = CacheCapacity;
  return EFI_SUCCESS;
}

/** Read a cache property and write it into the mapped CacheInfo field.

  @param [in]      Fdt          Pointer to the Flattened Device Tree.
  @param [in]      Node         DT node offset.
  @param [in]      Property     Property name.
  @param [in]      PropertyMap  CacheInfo field mapping.
  @param [in, out] CacheInfo    Cache information to update.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             The property exists but is malformed.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           The property was not found.
**/
STATIC
EFI_STATUS
EFIAPI
ReadCacheInfoProperty (
  IN     CONST VOID                     *Fdt,
  IN     INT32                          Node,
  IN     CONST CHAR8                    *Property,
  IN     CONST CACHE_INFO_PROPERTY_MAP  *PropertyMap,
  IN OUT CM_ARCH_COMMON_CACHE_INFO      *CacheInfo
  )
{
  CONST UINT32  *Data;
  INT32         DataSize;
  UINT8         *Field;
  UINT32        Value;

  if ((Fdt == NULL) || (Property == NULL) || (PropertyMap == NULL) || (CacheInfo == NULL)) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Data = FdtGetProp (Fdt, Node, Property, &DataSize);
  if (Data == NULL) {
    return EFI_NOT_FOUND;
  }

  if (DataSize != sizeof (UINT32)) {
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  Value = Fdt32ToCpu (*Data);
  Field = (UINT8 *)CacheInfo + PropertyMap->FieldOffset;
  if (PropertyMap->FieldSize == sizeof (UINT16)) {
    *(UINT16 *)Field = (UINT16)Value;
  } else if (PropertyMap->FieldSize == sizeof (UINT32)) {
    *(UINT32 *)Field = Value;
  } else {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/** Generate a default SMBIOS cache designation string.

  @param [in, out] CacheInfo  Cache information to update.
**/
STATIC
VOID
EFIAPI
PopulateDefaultCacheDesignation (
  IN OUT CM_ARCH_COMMON_CACHE_INFO  *CacheInfo
  )
{
  UINT8        CacheType;
  UINTN        Index;
  CONST CHAR8  *DesignationFormat;

  ASSERT (CacheInfo != NULL);

  CacheType = (UINT8)((CacheInfo->Attributes >> 2) & 0x3);
  for (Index = 0; Index < ARRAY_SIZE (mCacheTypeDesignationMap); Index++) {
    if (mCacheTypeDesignationMap[Index].CacheType == CacheType) {
      break;
    }
  }

  if (Index >= ARRAY_SIZE (mCacheTypeDesignationMap)) {
    ASSERT (FALSE);
    DEBUG ((DEBUG_ERROR, "CacheDesignation: invalid cache type (%d)\n", CacheType));
    return;
  }

  DesignationFormat = mCacheTypeDesignationMap[Index].DesignationFormat;

  AsciiSPrint (
    CacheInfo->SocketDesignation,
    sizeof (CacheInfo->SocketDesignation),
    DesignationFormat,
    CacheInfo->Level + 1
    );
}

/** Find an existing parsed DT cache node.

  @param [in] Context    Topology parser context.
  @param [in] CacheNode  DT cache node offset.

  @return The cache index if found, or TOPOLOGY_INVALID_INDEX otherwise.
**/
STATIC
UINT32
EFIAPI
FindCacheByDtNode (
  IN CONST TOPOLOGY_PARSER_CONTEXT  *Context,
  IN INT32                          CacheNode
  )
{
  UINT32  Index;

  ASSERT (Context != NULL);

  for (Index = 0; Index < Context->CacheCount; Index++) {
    if (Context->CacheBuffers[Index].DtNode == CacheNode) {
      return Index;
    }
  }

  return TOPOLOGY_INVALID_INDEX;
}

/** Check whether a CPU node exposes any scalar properties for a cache type.

  @param [in] Fdt                Pointer to the Flattened Device Tree.
  @param [in] CpuNode            CPU DT node offset.
  @param [in] CachePropertyType  Cache property-name group to scan.

  @retval TRUE   At least one property for the cache type is present.
  @retval FALSE  No properties for the cache type are present.
**/
STATIC
BOOLEAN
EFIAPI
HasCpuScalarCacheProperty (
  IN CONST VOID               *Fdt,
  IN INT32                    CpuNode,
  IN CPU_CACHE_PROPERTY_TYPE  CachePropertyType
  )
{
  UINT32  PropertyIndex;

  ASSERT (Fdt != NULL);
  ASSERT (CachePropertyType < CpuCachePropertyMax);

  for (PropertyIndex = 0; PropertyIndex < CachePropertyNameMax; PropertyIndex++) {
    if (FdtGetProp (Fdt, CpuNode, mCpuCachePropertyNames[CachePropertyType][PropertyIndex], NULL) != NULL) {
      return TRUE;
    }
  }

  return FALSE;
}

/** Parse a local CPU cache described with DT scalar cache properties.

  @param [in, out] Context              Topology parser context.
  @param [in]      Fdt                  Pointer to the Flattened Device Tree.
  @param [in]      CpuNode              CPU DT node offset.
  @param [in]      CpuIndex             CPU ordinal.
  @param [in]      OwnerProcIndex       Owning processor hierarchy node index.
  @param [in]      CachePropertyType    Cache property-name group to parse.
  @param [in]      CacheLevel           One-based cache level.
  @param [in]      CacheAttributes      Encoded PPTT cache attributes.
  @param [in]      NextLevelCacheToken  Token of the next-level cache.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             Malformed cache information was found.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Memory allocation failed.
**/
STATIC
EFI_STATUS
EFIAPI
ParseCpuScalarCache (
  IN OUT TOPOLOGY_PARSER_CONTEXT  *Context,
  IN     CONST VOID               *Fdt,
  IN     INT32                    CpuNode,
  IN     UINT32                   CpuIndex,
  IN     UINT32                   OwnerProcIndex,
  IN     CPU_CACHE_PROPERTY_TYPE  CachePropertyType,
  IN     UINT32                   CacheLevel,
  IN     UINT8                    CacheAttributes,
  IN     CM_OBJECT_TOKEN          NextLevelCacheToken
  )
{
  EFI_STATUS                 Status;
  CM_ARCH_COMMON_CACHE_INFO  *CacheInfo;
  UINT32                     CacheIndex;
  UINT32                     PropertyIndex;

  if ((Context == NULL) || (Fdt == NULL) || (CachePropertyType >= CpuCachePropertyMax)) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  if (Context->CacheCount >= Context->CacheCapacity) {
    ASSERT (FALSE);
    return EFI_OUT_OF_RESOURCES;
  }

  Context->CurrCacheIndex = Context->CacheCount;
  CacheIndex              = Context->CurrCacheIndex;
  CacheInfo               = &Context->CacheBuffers[CacheIndex].Info;
  ZeroMem (CacheInfo, sizeof (*CacheInfo));

  CacheInfo->Token                 = CreateCacheToken (CacheIndex);
  CacheInfo->NextLevelOfCacheToken = NextLevelCacheToken;
  CacheInfo->Attributes            = CacheAttributes;
  CacheInfo->Level                 = (CacheLevel == 0) ? 0 : (CacheLevel - 1);
  CacheInfo->CacheId               = CacheIndex + 1;

  for (PropertyIndex = 0; PropertyIndex < CachePropertyNameMax; PropertyIndex++) {
    Status = ReadCacheInfoProperty (
               Fdt,
               CpuNode,
               mCpuCachePropertyNames[CachePropertyType][PropertyIndex],
               &mCacheInfoPropertyMap[PropertyIndex],
               CacheInfo
               );
    if ((Status != EFI_SUCCESS) && (Status != EFI_NOT_FOUND)) {
      ASSERT (FALSE);
      return Status;
    }
  }

  if (CacheInfo->NumberOfSets * CacheInfo->LineSize == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "Cache-parser: cache-sets (%d) or cache-line-size (%d) property is 0.\n",
      CacheInfo->NumberOfSets,
      CacheInfo->LineSize
      ));
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  CacheInfo->Associativity = CacheInfo->Size / (CacheInfo->NumberOfSets * CacheInfo->LineSize);

  PopulateDefaultCacheDesignation (CacheInfo);
  SetCpuMaskBit (GetCacheCpuMask (Context, CacheIndex), CpuIndex);
  Context->CacheBuffers[CacheIndex].OwnerProcIndex = OwnerProcIndex;
  Context->CacheBuffers[CacheIndex].DtNode         = -1;
  Context->CacheCount++;
  return EFI_SUCCESS;
}

/** Parse a shared DT cache node and its next-level-cache chain.

  @param [in, out] Context      Topology parser context.
  @param [in]      Fdt          Pointer to the Flattened Device Tree.
  @param [in]      CacheNode    DT cache node offset.
  @param [in]      CpuIndex     CPU ordinal reaching this cache.
  @param [in]      DefaultLevel One-based default cache level if the DT omits
                                "cache-level".
  @param [out]     CacheToken   Token of the parsed cache node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             Malformed cache information was found.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Memory allocation failed.
**/
STATIC
EFI_STATUS
EFIAPI
ParseCacheNode (
  IN OUT TOPOLOGY_PARSER_CONTEXT  *Context,
  IN     CONST VOID               *Fdt,
  IN     INT32                    CacheNode,
  IN     UINT32                   CpuIndex,
  IN     UINT32                   DefaultLevel,
  OUT    CM_OBJECT_TOKEN          *CacheToken
  )
{
  EFI_STATUS                 Status;
  UINT32                     CacheIndex;
  CM_ARCH_COMMON_CACHE_INFO  *CacheInfo;
  CONST UINT32               *CacheProp;
  CONST UINT32               *NextLevelProp;
  INT32                      PropSize;
  INT32                      NextLevelNode;
  UINT32                     CacheLevel;
  UINT32                     PropertyIndex;
  UINT32                     Value;

  if ((Context == NULL) || (Fdt == NULL) || (CacheToken == NULL)) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  CacheIndex = FindCacheByDtNode (Context, CacheNode);
  if (CacheIndex != TOPOLOGY_INVALID_INDEX) {
    SetCpuMaskBit (GetCacheCpuMask (Context, CacheIndex), CpuIndex);
    NextLevelProp = FdtGetProp (Fdt, CacheNode, "next-level-cache", &PropSize);
    if (NextLevelProp != NULL) {
      if (PropSize != sizeof (UINT32)) {
        ASSERT (FALSE);
        return EFI_ABORTED;
      }

      NextLevelNode = FdtNodeOffsetByPhandle (Fdt, Fdt32ToCpu (*NextLevelProp));
      if (NextLevelNode < 0) {
        ASSERT (FALSE);
        return EFI_ABORTED;
      }

      Status = ParseCacheNode (
                 Context,
                 Fdt,
                 NextLevelNode,
                 CpuIndex,
                 Context->CacheBuffers[CacheIndex].Info.Level + 2,
                 &Context->CacheBuffers[CacheIndex].Info.NextLevelOfCacheToken
                 );
      if (EFI_ERROR (Status)) {
        ASSERT (FALSE);
        return Status;
      }
    }

    *CacheToken = Context->CacheBuffers[CacheIndex].Info.Token;
    return EFI_SUCCESS;
  }

  if (Context->CacheCount >= Context->CacheCapacity) {
    ASSERT (FALSE);
    return EFI_OUT_OF_RESOURCES;
  }

  Context->CurrCacheIndex = Context->CacheCount;
  CacheIndex              = Context->CurrCacheIndex;
  CacheInfo               = &Context->CacheBuffers[CacheIndex].Info;
  ZeroMem (CacheInfo, sizeof (*CacheInfo));

  CacheInfo->Token                         = CreateCacheToken (CacheIndex);
  CacheInfo->CacheId                       = CacheIndex + 1;
  Context->CacheBuffers[CacheIndex].DtNode = CacheNode;
  Context->CacheCount++;

  SetCpuMaskBit (GetCacheCpuMask (Context, CacheIndex), CpuIndex);

  CacheProp = FdtGetProp (Fdt, CacheNode, "cache-level", &PropSize);
  if (CacheProp != NULL) {
    if (PropSize != sizeof (UINT32)) {
      ASSERT (FALSE);
      return EFI_ABORTED;
    }

    CacheLevel       = Fdt32ToCpu (*CacheProp);
    CacheInfo->Level = (CacheLevel == 0) ? 0 : (CacheLevel - 1);
  } else {
    CacheInfo->Level = (DefaultLevel == 0) ? 0 : (DefaultLevel - 1);
  }

  for (PropertyIndex = 0; PropertyIndex < CachePropertyNameMax; PropertyIndex++) {
    Status = ReadCacheInfoProperty (
               Fdt,
               CacheNode,
               mCpuCachePropertyNames[CpuCachePropertyShared][PropertyIndex],
               &mCacheInfoPropertyMap[PropertyIndex],
               CacheInfo
               );
    if ((Status != EFI_SUCCESS) && (Status != EFI_NOT_FOUND)) {
      ASSERT (FALSE);
      return Status;
    }

    if (CacheInfo->NumberOfSets * CacheInfo->LineSize != 0) {
      CacheInfo->Associativity = CacheInfo->Size / (CacheInfo->NumberOfSets * CacheInfo->LineSize);
    }
  }

  CacheProp = FdtGetProp (Fdt, CacheNode, "cache-id", &PropSize);
  if (CacheProp != NULL) {
    if (PropSize != sizeof (UINT32)) {
      ASSERT (FALSE);
      return EFI_ABORTED;
    }

    Value              = Fdt32ToCpu (*CacheProp);
    CacheInfo->CacheId = Value;
  }

  CacheInfo->Attributes = mCpuCacheAttributes[CpuCachePropertyShared];

  NextLevelProp = FdtGetProp (Fdt, CacheNode, "next-level-cache", &PropSize);
  if (NextLevelProp != NULL) {
    if (PropSize != sizeof (UINT32)) {
      ASSERT (FALSE);
      return EFI_ABORTED;
    }

    NextLevelNode = FdtNodeOffsetByPhandle (Fdt, Fdt32ToCpu (*NextLevelProp));
    if (NextLevelNode < 0) {
      ASSERT (FALSE);
      return EFI_ABORTED;
    }

    Status = ParseCacheNode (
               Context,
               Fdt,
               NextLevelNode,
               CpuIndex,
               CacheInfo->Level + 2,
               &CacheInfo->NextLevelOfCacheToken
               );
    if (EFI_ERROR (Status)) {
      ASSERT (FALSE);
      return Status;
    }
  }

  PopulateDefaultCacheDesignation (CacheInfo);
  *CacheToken = CacheInfo->Token;
  return EFI_SUCCESS;
}

/** Resolve the best processor hierarchy node to own a cache.

  The preferred match is the deepest node whose CPU set exactly matches the
  cache sharing set. If the topology is too coarse to represent that sharing
  set exactly, the deepest enclosing node is used instead.

  @param [in]  Context        Topology parser context.
  @param [in]  CacheCpuMask   CPU sharing mask of the cache.
  @param [out] ProcNodeIndex  Owning processor hierarchy node index.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           No suitable processor hierarchy node was found.
**/
STATIC
EFI_STATUS
EFIAPI
FindCacheOwnerProcNode (
  IN  CONST TOPOLOGY_PARSER_CONTEXT  *Context,
  IN  CONST UINT64                   *CacheCpuMask,
  OUT       UINT32                   *ProcNodeIndex
  )
{
  UINT32  Index;
  UINT32  BestExact;
  UINT32  BestSuperset;
  UINT32  BestExactDepth;
  UINT32  BestSupersetDepth;
  UINT64  *ProcCpuMask;

  if ((Context == NULL) || (CacheCpuMask == NULL) || (ProcNodeIndex == NULL)) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  BestExact         = TOPOLOGY_INVALID_INDEX;
  BestSuperset      = TOPOLOGY_INVALID_INDEX;
  BestExactDepth    = 0;
  BestSupersetDepth = 0;

  for (Index = 0; Index < Context->ProcHierarchyCount; Index++) {
    ProcCpuMask = GetProcHierarchyCpuMask (Context, Index);
    if (!IsCpuMaskSubset (CacheCpuMask, ProcCpuMask)) {
      continue;
    }

    if (IsCpuMaskEqual (CacheCpuMask, ProcCpuMask)) {
      if ((BestExact == TOPOLOGY_INVALID_INDEX) ||
          (Context->ProcHierarchyBuffers[Index].Depth > BestExactDepth))
      {
        BestExact      = Index;
        BestExactDepth = Context->ProcHierarchyBuffers[Index].Depth;
      }
    } else if ((BestSuperset == TOPOLOGY_INVALID_INDEX) ||
               (Context->ProcHierarchyBuffers[Index].Depth > BestSupersetDepth))
    {
      BestSuperset      = Index;
      BestSupersetDepth = Context->ProcHierarchyBuffers[Index].Depth;
    }
  }

  if (BestExact != TOPOLOGY_INVALID_INDEX) {
    *ProcNodeIndex = BestExact;
    return EFI_SUCCESS;
  }

  if (BestSuperset != TOPOLOGY_INVALID_INDEX) {
    *ProcNodeIndex = BestSuperset;
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

/** Attach parsed caches to their owning processor hierarchy nodes.

  @param [in, out] Context  Topology parser context.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           A cache could not be placed in the topology.
  @retval EFI_OUT_OF_RESOURCES    Memory allocation failed.
**/
STATIC
EFI_STATUS
EFIAPI
AttachCachesToProcHierarchy (
  IN OUT TOPOLOGY_PARSER_CONTEXT  *Context
  )
{
  EFI_STATUS              Status;
  UINT32                  CacheIndex;
  UINT32                  ProcIndex;
  UINT32                  TotalRefCount;
  UINT32                  *PerProcRefCount;
  UINT32                  *PerProcOffset;
  CM_ARCH_COMMON_OBJ_REF  *RefArray;

  if (Context == NULL) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  PerProcRefCount = NULL;
  PerProcOffset   = NULL;
  RefArray        = NULL;

  if (Context->CacheCount == 0) {
    return EFI_SUCCESS;
  }

  for (CacheIndex = 0; CacheIndex < Context->CacheCount; CacheIndex++) {
    if (Context->CacheBuffers[CacheIndex].OwnerProcIndex != TOPOLOGY_INVALID_INDEX) {
      continue;
    }

    Status = FindCacheOwnerProcNode (
               Context,
               GetCacheCpuMask (Context, CacheIndex),
               &Context->CacheBuffers[CacheIndex].OwnerProcIndex
               );
    if (EFI_ERROR (Status)) {
      ASSERT (FALSE);
      goto exit_handler;
    }
  }

  PerProcRefCount = AllocateZeroPool (sizeof (*PerProcRefCount) * Context->ProcHierarchyCount);
  PerProcOffset   = AllocateZeroPool (sizeof (*PerProcOffset) * Context->ProcHierarchyCount);
  if ((PerProcRefCount == NULL) || (PerProcOffset == NULL)) {
    Status = EFI_OUT_OF_RESOURCES;
    ASSERT (FALSE);
    goto exit_handler;
  }

  TotalRefCount = 0;
  for (CacheIndex = 0; CacheIndex < Context->CacheCount; CacheIndex++) {
    ProcIndex = Context->CacheBuffers[CacheIndex].OwnerProcIndex;
    PerProcRefCount[ProcIndex]++;
    TotalRefCount++;
  }

  RefArray = AllocateZeroPool (sizeof (*RefArray) * TotalRefCount);
  if ((TotalRefCount != 0) && (RefArray == NULL)) {
    Status = EFI_OUT_OF_RESOURCES;
    ASSERT (FALSE);
    goto exit_handler;
  }

  for (ProcIndex = 0; ProcIndex < Context->ProcHierarchyCount; ProcIndex++) {
    PerProcOffset[ProcIndex] = (ProcIndex == 0) ? 0 : (PerProcOffset[ProcIndex - 1] + PerProcRefCount[ProcIndex - 1]);
  }

  for (CacheIndex = 0; CacheIndex < Context->CacheCount; CacheIndex++) {
    ProcIndex                                         = Context->CacheBuffers[CacheIndex].OwnerProcIndex;
    RefArray[PerProcOffset[ProcIndex]].ReferenceToken = Context->CacheBuffers[CacheIndex].Info.Token;
    PerProcOffset[ProcIndex]++;
  }

  for (ProcIndex = Context->ProcHierarchyCount; ProcIndex-- > 0; ) {
    if (PerProcRefCount[ProcIndex] == 0) {
      continue;
    }

    PerProcOffset[ProcIndex]                                          -= PerProcRefCount[ProcIndex];
    Context->ProcHierarchyBuffers[ProcIndex].Info.NoOfPrivateResources = PerProcRefCount[ProcIndex];
    Status                                                             = AddSingleCmObjArray (
                                                                           Context->FdtParserHandle,
                                                                           CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjCmRef),
                                                                           &RefArray[PerProcOffset[ProcIndex]],
                                                                           sizeof (CM_ARCH_COMMON_OBJ_REF) * PerProcRefCount[ProcIndex],
                                                                           PerProcRefCount[ProcIndex],
                                                                           &Context->ProcHierarchyBuffers[ProcIndex].Info.PrivateResourcesArrayToken
                                                                           );
    if (EFI_ERROR (Status)) {
      ASSERT (FALSE);
      goto exit_handler;
    }
  }

  Status = EFI_SUCCESS;

exit_handler:
  if (PerProcRefCount != NULL) {
    FreePool (PerProcRefCount);
  }

  if (PerProcOffset != NULL) {
    FreePool (PerProcOffset);
  }

  if (RefArray != NULL) {
    FreePool (RefArray);
  }

  return Status;
}

/** Parse cache information for all CPUs in the topology context.

  This helper derives local instruction/data/unified caches from CPU scalar
  properties and shared caches from "next-level-cache" DT nodes, attaches the
  resulting cache references to processor-hierarchy nodes, and finally publishes
  the generated CM cache objects.

  @param [in, out] Context  Topology parser context.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             Malformed cache information was found.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           A CPU leaf could not be found for a DT CPU.
  @retval EFI_OUT_OF_RESOURCES    Memory allocation failed.
**/
EFI_STATUS
EFIAPI
ParseCacheInfo (
  IN OUT TOPOLOGY_PARSER_CONTEXT  *Context
  )
{
  EFI_STATUS               Status;
  CONST VOID               *Fdt;
  UINT32                   Index;
  UINT32                   CpuIndex;
  UINT32                   ProcIndex;
  CPU_CACHE_PROPERTY_TYPE  CachePropertyType;
  BOOLEAN                  HasSplitCache;
  UINT8                    CacheAttributes;
  CONST UINT32             *NextLevelProp;
  INT32                    PropSize;
  INT32                    NextLevelNode;
  CM_OBJECT_TOKEN          NextLevelCacheToken;

  if (Context == NULL) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Status = AllocateCacheBuffers (Context);
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    return Status;
  }

  Fdt = Context->FdtParserHandle->Fdt;
  for (CpuIndex = 0; CpuIndex < Context->CpuCount; CpuIndex++) {
    ProcIndex = TOPOLOGY_INVALID_INDEX;
    for (NextLevelNode = 0; NextLevelNode < (INT32)Context->ProcHierarchyCount; NextLevelNode++) {
      if (Context->ProcHierarchyBuffers[NextLevelNode].CpuNode == Context->CpuNodes[CpuIndex]) {
        ProcIndex = (UINT32)NextLevelNode;
        break;
      }
    }

    if (ProcIndex == TOPOLOGY_INVALID_INDEX) {
      ASSERT (FALSE);
      return EFI_NOT_FOUND;
    }

    NextLevelCacheToken = CM_NULL_TOKEN;
    NextLevelProp       = FdtGetProp (Fdt, Context->CpuNodes[CpuIndex], "next-level-cache", &PropSize);
    if (NextLevelProp != NULL) {
      if (PropSize != sizeof (UINT32)) {
        ASSERT (FALSE);
        return EFI_ABORTED;
      }

      NextLevelNode = FdtNodeOffsetByPhandle (Fdt, Fdt32ToCpu (*NextLevelProp));
      if (NextLevelNode < 0) {
        ASSERT (FALSE);
        return EFI_ABORTED;
      }

      Status = ParseCacheNode (
                 Context,
                 Fdt,
                 NextLevelNode,
                 CpuIndex,
                 2,
                 &NextLevelCacheToken
                 );
      if (EFI_ERROR (Status)) {
        ASSERT (FALSE);
        return Status;
      }
    }

    HasSplitCache =
      HasCpuScalarCacheProperty (Fdt, Context->CpuNodes[CpuIndex], CpuCachePropertyInstruction) ||
      HasCpuScalarCacheProperty (Fdt, Context->CpuNodes[CpuIndex], CpuCachePropertyData);

    for (CachePropertyType = CpuCachePropertyInstruction;
         CachePropertyType < CpuCachePropertyMax;
         CachePropertyType++)
    {
      if ((CachePropertyType == CpuCachePropertyShared) && HasSplitCache) {
        continue;
      }

      if (!HasCpuScalarCacheProperty (Fdt, Context->CpuNodes[CpuIndex], CachePropertyType)) {
        continue;
      }

      CacheAttributes = mCpuCacheAttributes[CachePropertyType];

      Status = ParseCpuScalarCache (
                 Context,
                 Fdt,
                 Context->CpuNodes[CpuIndex],
                 CpuIndex,
                 ProcIndex,
                 CachePropertyType,
                 1,
                 CacheAttributes,
                 NextLevelCacheToken
                 );
      if (EFI_ERROR (Status)) {
        ASSERT (FALSE);
        return Status;
      }
    }
  }

  Status = AttachCachesToProcHierarchy (Context);
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    return Status;
  }

  // Add the CacheInfo nodes.
  for (Index = 0; Index < Context->CacheCount; Index++) {
    Status = AddSingleCmObjWithToken (
               Context->FdtParserHandle,
               CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjCacheInfo),
               &Context->CacheBuffers[Index].Info,
               sizeof (Context->CacheBuffers[Index].Info),
               Context->CacheBuffers[Index].Info.Token
               );
    if (EFI_ERROR (Status)) {
      ASSERT (FALSE);
      return Status;
    }
  }

  return Status;
}
