/** @file
  Arm SMMUv3 IORT parser.

  Copyright (c) 2025, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - linux/Documentation/devicetree/bindings/iommu/arm,smmu-v3.yaml
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/FdtLib.h>
#include <IndustryStandard/IoRemappingTable.h>
#include "FdtHwInfoParser.h"
#include "CmObjectDescUtility.h"
#include "Arm/Iort/ArmIortParser.h"
#include "Arm/Iort/SmmuV3Parser.h"

#define IOMMU_MAP_CELL_COUNT  4
#define MSI_MAP_CELL_COUNT    4

/** Find the interrupt ID specified by the given interrupt name

  @param [in]  InterruptNames      Pointer to the FDT interrupt-names field
  @param [in]  InterruptNamesSize  Size of the FDT interrupt-names field, in bytes
  @param [in]  Interrupts          Pointer to the FDT interrupts field
  @param [in]  IntCells            Number of cells per interrupt
  @param [in]  Name                Pointer to the name of the interrupt to find

  @return Interrupt ID, or 0 if no matching interrupt found
**/
STATIC
UINT32
FdtGetInterruptFromName (
  IN  CONST CHAR8   *InterruptNames,
  IN  CONST UINT32  InterruptNamesSize,
  IN  CONST UINT32  *Interrupts,
  IN  CONST UINTN   IntCells,
  IN  CONST CHAR8   *Name
  )
{
  INTN  StrIndex;
  INTN  Index;

  StrIndex = 0;
  Index    = 0;

  while (Index < InterruptNamesSize) {
    INTN  StringLength = MIN (AsciiStrSize (Name), InterruptNamesSize - Index);

    if (!AsciiStrnCmp (&InterruptNames[Index], Name, StringLength)) {
      break;
    }

    StrIndex++;
    Index += AsciiStrSize (&InterruptNames[Index]);
  }

  if (Index >= InterruptNamesSize) {
    return 0;
  }

  return FdtGetInterruptId (&Interrupts[StrIndex * IntCells]);
}

/** List of "compatible" property values for SmmuV3 nodes.

  Other "compatible" values are not supported by this module.
*/
STATIC CONST COMPATIBILITY_STR  SmmuV3CompatibleStr[] = {
  { "arm,smmu-v3" }
};

/** SmmuV3 compatiblity information.
*/
STATIC CONST COMPATIBILITY_INFO  SmmuV3CompatibleInfo = {
  ARRAY_SIZE (SmmuV3CompatibleStr),
  SmmuV3CompatibleStr
};

/** List of "compatible" property values for PciRootComplex nodes.
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

/** Find the appropriate msi-map

  @param [in]  FdtParserHandle    A handle to the parser instance.
  @param [in]  SmmuV3Node         Offset of an SmmuV3 node.
  @param [out] DataOut            Location to store pointer to msi-map property.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           msi-map not found for this SmmuV3 node.
**/
STATIC
EFI_STATUS
FindMsiMapForSmmuV3 (
  IN   CONST VOID    *Fdt,
  IN         INT32   SmmuV3Node,
  OUT  CONST UINT32  **DataOut
  )
{
  EFI_STATUS    Status;
  UINT32        Index;
  INT32         RootComplexNode;
  UINT32        RootComplexNodeCount;
  INT32         DataSize;
  INT32         Node;
  CONST UINT32  *Data;

  Status = FdtCountCompatNodeInBranch (
             Fdt,
             -1, // FdtBranch,
             &RootComplexCompatibleInfo,
             &RootComplexNodeCount
             );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return EFI_INVALID_PARAMETER;
  }

  RootComplexNode = -1;
  for (Index = 0; Index < RootComplexNodeCount; Index++) {
    Status = FdtGetNextCompatNodeInBranch (
               Fdt,
               -1,
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

    Data = FdtGetProp (Fdt, RootComplexNode, "iommu-map", &DataSize);
    if ((Data == NULL) || (DataSize != (IOMMU_MAP_CELL_COUNT * sizeof (UINT32)))) {
      // If error or not IOMMU_MAP_CELL_COUNT cells.
      ASSERT ((Data != NULL) && (DataSize == (IOMMU_MAP_CELL_COUNT * sizeof (UINT32))));
      return EFI_ABORTED;
    }

    Node = FdtNodeOffsetByPhandle (Fdt, Fdt32ToCpu (((UINT32 *)Data)[1]));
    if (Node != SmmuV3Node) {
      continue; // Not for this SMMU
    }

    Data = FdtGetProp (Fdt, RootComplexNode, "msi-map", &DataSize);
    if ((Data == NULL) || (DataSize != (MSI_MAP_CELL_COUNT * sizeof (UINT32)))) {
      // If error or not MSI_MAP_CELL_COUNT cells.
      ASSERT ((Data != NULL) && (DataSize == (MSI_MAP_CELL_COUNT * sizeof (UINT32))));
      return EFI_ABORTED;
    }

    *DataOut = Data;
    return EFI_SUCCESS;
  } // for

  ASSERT (0);
  return EFI_NOT_FOUND;
}

/** Parse an SmmuV3 node.

  @param [in]  FdtParserHandle    A handle to the parser instance.
  @param [in]  Fdt                Pointer to a Flattened Device Tree (Fdt).
  @param [in]  SmmuV3Node         Offset of an SmmuV3 node.
  @param [in]  SmmuV3Info         The CM_ARM_SMMUV3_INFO to populate.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
SmmuV3NodeParser (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN  CONST VOID                       *Fdt,
  IN        INT32                      SmmuV3Node,
  IN        CM_ARM_SMMUV3_NODE         *SmmuV3Info
  )
{
  EFI_STATUS         Status;
  CONST UINT32       *Data;
  INT32              IntcNode;
  INT32              DataSize;
  INT32              IntCells;
  INT32              AddressCells;
  CONST UINT8        *InterruptNames;
  INT32              InterruptNamesSize;
  CM_ARM_ID_MAPPING  IdMappings[2];

  if ((Fdt == NULL) || (SmmuV3Info == NULL)) {
    ASSERT ((Fdt != NULL) && (SmmuV3Info != NULL));
    return EFI_INVALID_PARAMETER;
  }

  AddressCells = FdtAddressCells (Fdt, SmmuV3Node);
  if (AddressCells < 0) {
    ASSERT (AddressCells >= 0);
    return EFI_ABORTED;
  }

  // Get the associated interrupt-controller.
  Status = FdtGetIntcParentNode (Fdt, SmmuV3Node, &IntcNode);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  // Get the number of cells used to encode an interrupt.
  Status = FdtGetInterruptCellsInfo (Fdt, IntcNode, &IntCells);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    if (Status == EFI_NOT_FOUND) {
      // Should have found the node.
      Status = EFI_ABORTED;
    }

    return Status;
  }

  InterruptNames = FdtGetProp (Fdt, SmmuV3Node, "interrupt-names", &InterruptNamesSize);

  Data = FdtGetProp (Fdt, SmmuV3Node, "interrupts", &DataSize);
  if (Data == NULL) {
    // If error.
    ASSERT (Data != NULL);
    return EFI_ABORTED;
  }

  /// GSIV of the Event interrupt if SPI based
  SmmuV3Info->EventInterrupt = FdtGetInterruptFromName ((CONST CHAR8 *)InterruptNames, InterruptNamesSize, Data, IntCells, "eventq");
  /// PRI Interrupt if SPI based
  SmmuV3Info->PriInterrupt = FdtGetInterruptFromName ((CONST CHAR8 *)InterruptNames, InterruptNamesSize, Data, IntCells, "priq");
  /// GERR interrupt if GSIV based
  SmmuV3Info->GerrInterrupt = FdtGetInterruptFromName ((CONST CHAR8 *)InterruptNames, InterruptNamesSize, Data, IntCells, "gerror");
  /// Sync interrupt if GSIV based
  SmmuV3Info->SyncInterrupt = FdtGetInterruptFromName ((CONST CHAR8 *)InterruptNames, InterruptNamesSize, Data, IntCells, "cmdq-sync");

  /// SMMU flags
  SmmuV3Info->Flags = 0;
  /// VATOS address
  SmmuV3Info->VatosAddress = 0;

  /// Model
  SmmuV3Info->Model = EFI_ACPI_IORT_SMMUv3_MODEL_GENERIC;

  Data = FdtGetProp (Fdt, SmmuV3Node, "hisilicon,broken-prefetch-cmd", &DataSize);
  if ((Data != NULL) && (DataSize >= 0)) {
    SmmuV3Info->Model = EFI_ACPI_IORT_SMMUv3_MODEL_HISILICON_HI161X;
  }

  Data = FdtGetProp (Fdt, SmmuV3Node, "cavium,cn9900-broken-page1-regspace", &DataSize);
  if ((Data != NULL) && (DataSize >= 0)) {
    SmmuV3Info->Model = EFI_ACPI_IORT_SMMUv3_MODEL_CAVIUM_CN99XX;
  }

  Data = FdtGetProp (Fdt, SmmuV3Node, "reg", &DataSize);
  if (Data == NULL) {
    ASSERT (Data != NULL);
    return EFI_ABORTED;
  }

  if (AddressCells == 2) {
    SmmuV3Info->BaseAddress = Fdt64ToCpu (*((UINT64 *)Data));
  } else {
    SmmuV3Info->BaseAddress = Fdt32ToCpu (*((UINT32 *)Data));
  }

  ZeroMem (IdMappings, sizeof (IdMappings));

  Status = FindMsiMapForSmmuV3 (Fdt, SmmuV3Node, &Data);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  if (Data == NULL) {
    ASSERT (Data != NULL);
    return EFI_ABORTED;
  }

  IdMappings[0].InputBase            = Fdt32ToCpu (((UINT32 *)Data)[0]);
  IdMappings[0].NumIds               = Fdt32ToCpu (((UINT32 *)Data)[3]);
  IdMappings[0].OutputBase           = Fdt32ToCpu (((UINT32 *)Data)[2]);
  IdMappings[0].OutputReferenceToken = CM_ABSTRACT_TOKEN_MAKE (ETokenNameSpaceFdtHwInfo, EFdtHwInfoIortObject, Fdt32ToCpu (((UINT32 *)Data)[1]));

  /// Number of ID mappings
  SmmuV3Info->IdMappingCount = 1;

  if ((SmmuV3Info->EventInterrupt == 0) || (SmmuV3Info->PriInterrupt  == 0) ||
      (SmmuV3Info->GerrInterrupt  == 0) || (SmmuV3Info->SyncInterrupt == 0))
  {
    Data = FdtGetProp (Fdt, SmmuV3Node, "msi-parent", &DataSize);
    if ((Data == NULL) || (DataSize != 2 * sizeof (UINT32))) {
      ASSERT ((Data != NULL) && (DataSize == 2 * sizeof (UINT32)));
      return EFI_ABORTED;
    }

    IdMappings[1].InputBase            = 0;
    IdMappings[1].OutputBase           = Fdt32ToCpu (((UINT32 *)Data)[1]);
    IdMappings[1].OutputReferenceToken = CM_ABSTRACT_TOKEN_MAKE (ETokenNameSpaceFdtHwInfo, EFdtHwInfoIortObject, Fdt32ToCpu (((UINT32 *)Data)[0]));
    IdMappings[1].NumIds               = 1;
    IdMappings[1].Flags                = EFI_ACPI_IORT_ID_MAPPING_FLAGS_SINGLE;

    SmmuV3Info->DeviceIdMappingIndex = 1;
    SmmuV3Info->IdMappingCount       = 2;
  }

  // Add the CmObj to the Configuration Manager.
  Status = AddSingleCmObjArray (
             FdtParserHandle,
             CREATE_CM_ARM_OBJECT_ID (EArmObjIdMappingArray),
             &IdMappings[0],
             sizeof (CM_ARM_ID_MAPPING) * SmmuV3Info->IdMappingCount,
             SmmuV3Info->IdMappingCount,
             &SmmuV3Info->IdMappingToken
             );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  /// Proximity domain flag
  SmmuV3Info->ProximityDomain = 0;

  /// Unique identifier for this node.
  SmmuV3Info->Identifier = GetNextIortIdentifier ();

  return EFI_SUCCESS;
}

/** CM_ARM_SMMUV3_NODE parser function.

  The following structure is populated:
  typedef struct CmArmSmmuV3Node {
    CM_OBJECT_TOKEN    Token;
    UINT32             IdMappingCount;          // {Populated}
    CM_OBJECT_TOKEN    IdMappingToken;          // {Populated}
    UINT64             BaseAddress;             // {Populated}
    UINT32             Flags;                   // {default = 0}
    UINT64             VatosAddress;            // {default = 0}
    UINT32             Model;                   // {Populated}
    UINT32             EventInterrupt;          // {Populated}
    UINT32             PriInterrupt;            // {Populated}
    UINT32             GerrInterrupt;           // {Populated}
    UINT32             SyncInterrupt;           // {Populated}
    UINT32             ProximityDomain;         // {default = 0}
    UINT32             DeviceIdMappingIndex;    // {Populated}
    UINT32             Identifier;              // {Populated}
    CM_OBJECT_TOKEN    ProximityDomainToken;    // {default = 0}
  } CM_ARM_SMMUV3_NODE;

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
ArmSmmuV3Parser (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        INT32                      FdtBranch
  )
{
  EFI_STATUS          Status;
  UINT32              Index;
  INT32               SmmuV3Node;
  UINT32              SmmuV3NodeCount;
  CM_ARM_SMMUV3_NODE  SmmuV3Info;
  VOID                *Fdt;

  if (FdtParserHandle == NULL) {
    ASSERT (FdtParserHandle != NULL);
    return EFI_INVALID_PARAMETER;
  }

  Fdt    = FdtParserHandle->Fdt;
  Status = FdtCountCompatNodeInBranch (
             Fdt,
             FdtBranch,
             &SmmuV3CompatibleInfo,
             &SmmuV3NodeCount
             );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  if (SmmuV3NodeCount == 0) {
    return EFI_NOT_FOUND;
  }

  // Parse each SmmuV3 node in the branch.
  SmmuV3Node = FdtBranch;
  for (Index = 0; Index < SmmuV3NodeCount; Index++) {
    ZeroMem (&SmmuV3Info, sizeof (CM_ARM_SMMUV3_NODE));

    Status = FdtGetNextCompatNodeInBranch (
               Fdt,
               FdtBranch,
               &SmmuV3CompatibleInfo,
               &SmmuV3Node
               );
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      if (Status == EFI_NOT_FOUND) {
        // Should have found the node.
        Status = EFI_ABORTED;
      }

      return Status;
    }

    Status = SmmuV3NodeParser (FdtParserHandle, Fdt, SmmuV3Node, &SmmuV3Info);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    // Add the CmObj to the Configuration Manager.
    Status = AddSingleCmObjWithToken (
               FdtParserHandle,
               CREATE_CM_ARM_OBJECT_ID (EArmObjSmmuV3),
               &SmmuV3Info,
               sizeof (CM_ARM_SMMUV3_NODE),
               CM_ABSTRACT_TOKEN_MAKE (ETokenNameSpaceFdtHwInfo, EFdtHwInfoIortObject, FdtGetPhandle (Fdt, SmmuV3Node))
               );
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }
  } // for

  return Status;
}
