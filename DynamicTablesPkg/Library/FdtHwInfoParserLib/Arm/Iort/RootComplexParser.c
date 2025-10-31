/** @file
  Root Complex parser.

  Copyright (c) 2025, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - linux/Documentation/devicetree/bindings/pci/host-generic-pci.yaml
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/FdtLib.h>
#include <IndustryStandard/IoRemappingTable.h>
#include "FdtHwInfoParser.h"
#include "CmObjectDescUtility.h"
#include "Arm/Iort/ArmIortParser.h"
#include "Arm/Iort/RootComplexParser.h"

#define IOMMU_MAP_CELL_COUNT  4

/** List of "compatible" property values for PciRootComplex nodes.

  Other "compatible" values are not supported by this module.
*/
STATIC CONST COMPATIBILITY_STR  RootComplexCompatibleStr[] = {
  { "pci-host-ecam-generic" }
};

/** PciRootComplex compatiblity information.
*/
STATIC CONST COMPATIBILITY_INFO  RootComplexCompatibleInfo = {
  ARRAY_SIZE (RootComplexCompatibleStr),
  RootComplexCompatibleStr
};

/** Determine the memory address size for the given root complex

  Determine the memory address size based on the dma-ranges property. If this
  property is not present then default to 32 bits.

  @param [in]  Fdt                Pointer to FDT
  @param [in]  RootComplexNode    Offset of root complex node
  @param [out] MemoryAddressSize  Location to store memory address size

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
RootComplexNodeGetMemoryAddressSize (
  IN  CONST VOID   *Fdt,
  IN  CONST INT32  RootComplexNode,
  OUT       UINT8  *MemoryAddressSize
  )
{
  CONST UINT32  *DmaRanges;
  INT32         ParentNode;
  INT32         DataSize;
  INT32         AddressCells;
  INT32         SizeCells;
  INT32         ParentAddressCells;
  INT32         Stride;
  INTN          DataOffset;
  INTN          SizeOffset;
  UINT64        DmaAddressEnd;
  UINT64        DmaAddress;
  UINT64        Size;

  DmaRanges = FdtGetProp (Fdt, RootComplexNode, "dma-ranges", &DataSize);
  if ((DmaRanges == NULL) || (DataSize <= 0)) {
    *MemoryAddressSize = 32;
    return 0;
  }

  AddressCells = FdtAddressCells (Fdt, RootComplexNode);
  if (AddressCells < 0) {
    ASSERT (AddressCells >= 0);
    return EFI_INVALID_PARAMETER;
  }

  SizeCells = FdtSizeCells (Fdt, RootComplexNode);
  if (SizeCells < 0) {
    ASSERT (SizeCells >= 0);
    return EFI_INVALID_PARAMETER;
  }

  // Find parent node with #address-cells
  ParentNode = FdtParentOffset (Fdt, RootComplexNode);
  if (ParentNode < 0) {
    ASSERT (ParentNode >= 0);
    return EFI_INVALID_PARAMETER;
  }

  ParentAddressCells = -1;

  do {
    ParentAddressCells = FdtAddressCells (Fdt, ParentNode);
    if (ParentAddressCells < 0) {
      ParentNode = FdtParentOffset (Fdt, ParentNode);
      if (ParentNode < 0) {
        ASSERT (ParentNode >= 0);
        return EFI_INVALID_PARAMETER;
      }
    }
  } while (ParentAddressCells < 0);

  Stride     = AddressCells + ParentAddressCells + SizeCells;
  SizeOffset = AddressCells + ParentAddressCells;
  if ((DataSize < Stride) || ((DataSize % Stride) != 0)) {
    ASSERT (DataSize >= Stride && (DataSize % Stride) == 0);
    return EFI_INVALID_PARAMETER;
  }

  DataOffset    = 0;
  DmaAddressEnd = 0;

  // Walk dma-ranges and find maximum DMA address
  while ((DataOffset + Stride) <= DataSize) {
    switch (AddressCells) {
      case 1:
        DmaAddress = Fdt32ToCpu (DmaRanges[DataOffset]);
        break;
      case 2:
        DmaAddress = Fdt64ToCpu (*(UINT64 *)&DmaRanges[DataOffset]);
        break;
      case 3:
        DmaAddress = Fdt64ToCpu (*(UINT64 *)&DmaRanges[DataOffset + 1]);
        break;
      default:
        ASSERT (0);
        return EFI_INVALID_PARAMETER;
    }

    switch (SizeCells) {
      case 1:
        Size = Fdt32ToCpu (DmaRanges[DataOffset + SizeOffset]);
        break;
      case 2:
        Size = Fdt64ToCpu (*(UINT64 *)&DmaRanges[DataOffset + SizeOffset]);
        break;
      default:
        ASSERT (0);
        return EFI_INVALID_PARAMETER;
    }

    if (DmaAddressEnd < (DmaAddress + Size)) {
      DmaAddressEnd = DmaAddress + Size;
    }

    DataOffset += Stride;
  }

  // Round up if required
  if (DmaAddressEnd & (DmaAddressEnd - 1)) {
    *MemoryAddressSize = HighBitSet64 (DmaAddressEnd) + 2;
  } else {
    *MemoryAddressSize = HighBitSet64 (DmaAddressEnd) + 1;
  }

  return EFI_SUCCESS;
}

/** Parse a PCI root complex node.

  @param [in]  FdtParserHandle    A handle to the parser instance.
  @param [in]  Fdt                Pointer to a Flattened Device Tree (Fdt).
  @param [in]  RootComplexNode    Offset of a root complex node.
  @param [in]  RootComplexInfo    The CM_ARM_ROOT_COMPLEX_NODE to populate.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
RootComplexNodeParser (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN  CONST VOID                       *Fdt,
  IN        INT32                      RootComplexNode,
  IN        CM_ARM_ROOT_COMPLEX_NODE   *RootComplexInfo
  )
{
  EFI_STATUS         Status;
  CONST UINT32       *Data;
  INT32              DataSize;
  CM_ARM_ID_MAPPING  *IdMappings;
  INT32              MapIndex;

  if ((Fdt == NULL) || (RootComplexInfo == NULL)) {
    ASSERT ((Fdt != NULL) && (RootComplexInfo != NULL));
    return EFI_INVALID_PARAMETER;
  }

  /// Memory access properties : Cache coherent attributes
  /// Memory access properties : Memory access flags
  Data = FdtGetProp (Fdt, RootComplexNode, "dma-coherent", &DataSize);
  if ((Data != NULL) && (DataSize >= 0)) {
    RootComplexInfo->CacheCoherent     = EFI_ACPI_IORT_MEM_ACCESS_PROP_CCA;
    RootComplexInfo->MemoryAccessFlags = EFI_ACPI_IORT_MEM_ACCESS_FLAGS_CPM |
                                         EFI_ACPI_IORT_MEM_ACCESS_FLAGS_DACS;
  } else {
    RootComplexInfo->CacheCoherent     = 0;
    RootComplexInfo->MemoryAccessFlags = 0;
  }

  /// Memory access properties : Allocation hints
  RootComplexInfo->AllocationHints = 0;

  /// ATS attributes
  Data = FdtGetProp (Fdt, RootComplexNode, "ats-supported", &DataSize);
  if ((Data != NULL) && (DataSize >= 0)) {
    RootComplexInfo->AtsAttribute = EFI_ACPI_IORT_ROOT_COMPLEX_ATS_SUPPORTED;
  } else {
    RootComplexInfo->AtsAttribute = EFI_ACPI_IORT_ROOT_COMPLEX_ATS_UNSUPPORTED;
  }

  /// PCI segment number
  Data = FdtGetProp (Fdt, RootComplexNode, "linux,pci-domain", &DataSize);
  if (Data != NULL) {
    RootComplexInfo->PciSegmentNumber = *Data;
  }

  /// Memory address size limit
  Status = RootComplexNodeGetMemoryAddressSize (Fdt, RootComplexNode, &RootComplexInfo->MemoryAddressSize);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  /// PASID capabilities
  RootComplexInfo->PasidCapabilities = 0;

  /// Flags
  RootComplexInfo->Flags = 0;

  /// Unique identifier for this node.
  RootComplexInfo->Identifier = GetNextIortIdentifier ();

  Data = FdtGetProp (Fdt, RootComplexNode, "iommu-map", &DataSize);
  if ((Data == NULL) || ((DataSize % (IOMMU_MAP_CELL_COUNT * sizeof (UINT32))) != 0)) {
    // If error or invalid number of cells (not multiple of IOMMU_MAP_CELL_COUNT).
    ASSERT ((Data != NULL) && ((DataSize % (IOMMU_MAP_CELL_COUNT * sizeof (UINT32))) == 0));
    return EFI_ABORTED;
  }

  DataSize /= IOMMU_MAP_CELL_COUNT * sizeof (UINT32);

  IdMappings = AllocateZeroPool (DataSize * sizeof (CM_ARM_ID_MAPPING));
  if (IdMappings == NULL) {
    ASSERT (IdMappings != NULL);
    return EFI_OUT_OF_RESOURCES;
  }

  for (MapIndex = 0; MapIndex < DataSize; MapIndex++) {
    IdMappings[MapIndex].InputBase            = Fdt32ToCpu (((UINT32 *)Data)[MapIndex * IOMMU_MAP_CELL_COUNT]);
    IdMappings[MapIndex].NumIds               = Fdt32ToCpu (((UINT32 *)Data)[MapIndex * IOMMU_MAP_CELL_COUNT + 3]);
    IdMappings[MapIndex].OutputBase           = Fdt32ToCpu (((UINT32 *)Data)[MapIndex * IOMMU_MAP_CELL_COUNT + 2]);
    IdMappings[MapIndex].OutputReferenceToken = CM_ABSTRACT_TOKEN_MAKE (
                                                  ETokenNameSpaceFdtHwInfo,
                                                  EFdtHwInfoIortObject,
                                                  Fdt32ToCpu (((UINT32 *)Data)[MapIndex * IOMMU_MAP_CELL_COUNT + 1])
                                                  );
  }

  // Add the CmObj to the Configuration Manager.
  Status = AddSingleCmObjArray (
             FdtParserHandle,
             CREATE_CM_ARM_OBJECT_ID (EArmObjIdMappingArray),
             IdMappings,
             sizeof (CM_ARM_ID_MAPPING) * DataSize,
             DataSize,
             &RootComplexInfo->IdMappingToken
             );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    FreePool (IdMappings);
    return Status;
  }

  /// Number of ID mappings
  RootComplexInfo->IdMappingCount = DataSize;

  return EFI_SUCCESS;
}

/** CM_ARM_ROOT_COMPLEX_NODE parser function.

  The following structure is populated:
  typedef struct CmArmRootComplexNode {
    CM_OBJECT_TOKEN    Token;
    UINT32             IdMappingCount;          // {Populated}
    CM_OBJECT_TOKEN    IdMappingToken;          // {Populated}
    UINT32             CacheCoherent;           // {Populated}
    UINT8              AllocationHints;         // {default = 0}
    UINT8              MemoryAccessFlags;       // {Populated}
    UINT32             AtsAttribute;            // {Populated}
    UINT32             PciSegmentNumber;        // {Populated}
    UINT8              MemoryAddressSize;       // {Populated}
    UINT16             PasidCapabilities;       // {default = 0}
    UINT32             Flags;                   // {default = 0}
    UINT32             Identifier;              // {Populated}
  } CM_ARM_ROOT_COMPLEX_NODE;

  A parser parses a Device Tree to populate a specific CmObj type. None,
  one or many CmObj can be created by the parser.
  The created CmObj are then handed to the parser's caller through the
  HW_INFO_ADD_OBJECT interface.
  This can also be a dispatcher. I.e. a function that not parsing a
  Device Tree but calling other parsers.

  @param [in]  FdtParserHandle A handle to the parser instance.
  @param [in]  FdtBranch       When searching for DT node name, restrict
                               the search to this Device Tree branch.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           Not found.
  @retval EFI_UNSUPPORTED         Unsupported.
**/
EFI_STATUS
EFIAPI
ArmPciRootComplexParser (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        INT32                      FdtBranch
  )
{
  EFI_STATUS                Status;
  UINT32                    Index;
  INT32                     RootComplexNode;
  UINT32                    RootComplexNodeCount;
  CM_ARM_ROOT_COMPLEX_NODE  RootComplexInfo;
  VOID                      *Fdt;

  if (FdtParserHandle == NULL) {
    ASSERT (FdtParserHandle != NULL);
    return EFI_INVALID_PARAMETER;
  }

  Fdt    = FdtParserHandle->Fdt;
  Status = FdtCountCompatNodeInBranch (
             Fdt,
             FdtBranch,
             &RootComplexCompatibleInfo,
             &RootComplexNodeCount
             );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  if (RootComplexNodeCount == 0) {
    return EFI_NOT_FOUND;
  }

  // Parse each root complex node in the branch.
  RootComplexNode = FdtBranch;
  for (Index = 0; Index < RootComplexNodeCount; Index++) {
    ZeroMem (&RootComplexInfo, sizeof (CM_ARM_ROOT_COMPLEX_NODE));

    Status = FdtGetNextCompatNodeInBranch (
               Fdt,
               FdtBranch,
               &RootComplexCompatibleInfo,
               &RootComplexNode
               );
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      if (Status == EFI_NOT_FOUND) {
        // Should have found the node.
        Status = EFI_ABORTED;
      }

      return Status;
    }

    Status = RootComplexNodeParser (FdtParserHandle, Fdt, RootComplexNode, &RootComplexInfo);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    // Add the CmObj to the Configuration Manager.
    Status = AddSingleCmObj (
               FdtParserHandle,
               CREATE_CM_ARM_OBJECT_ID (EArmObjRootComplex),
               &RootComplexInfo,
               sizeof (CM_ARM_ROOT_COMPLEX_NODE),
               NULL
               );
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }
  } // for

  return Status;
}
