/** @file
  Arm Gic Interrupt Translation Service Parser.

  Copyright (c) 2021, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - linux/Documentation/devicetree/bindings/interrupt-controller/arm,gic-v3.yaml
**/

#include "CmObjectDescUtility.h"
#include "FdtHwInfoParser.h"
#include "Gic/ArmGicDispatcher.h"
#include "Gic/ArmGicItsParser.h"

/** Parse a Gic compatible interrupt-controller node,
    extracting GicIts information.

  This parser is valid for Gic v3 and higher.

  @param [in]  Fdt              Pointer to a Flattened Device Tree (Fdt).
  @param [in]  GicIntcNode      Offset of a Gic compatible
                                interrupt-controller node.
  @param [in]  GicItsId         Id for the Gic ITS node.
  @param [in]  GicItsInfo       The CM_ARM_GIC_ITS_INFO to populate.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
GicItsIntcNodeParser (
  IN  CONST VOID           *Fdt,
  IN  INT32                GicIntcNode,
  IN  UINT32               GicItsId,
  IN  CM_ARM_GIC_ITS_INFO  *GicItsInfo
  )
{
  EFI_STATUS   Status;
  INT32        AddressCells;
  CONST UINT8  *Data;
  INT32        DataSize;

  if ((Fdt == NULL) ||
      (GicItsInfo == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Status = FdtGetParentAddressInfo (Fdt, GicIntcNode, &AddressCells, NULL);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Don't support more than 64 bits and less than 32 bits addresses.
  if ((AddressCells < 1)  ||
      (AddressCells > 2))
  {
    ASSERT (0);
    return EFI_ABORTED;
  }

  Data = fdt_getprop (Fdt, GicIntcNode, "reg", &DataSize);
  if ((Data == NULL) || (DataSize < (INT32)(AddressCells * sizeof (UINT32)))) {
    // If error or not enough space.
    ASSERT (0);
    return EFI_ABORTED;
  }

  if (AddressCells == 2) {
    GicItsInfo->PhysicalBaseAddress = fdt64_to_cpu (*(UINT64 *)Data);
  } else {
    GicItsInfo->PhysicalBaseAddress = fdt32_to_cpu (*(UINT32 *)Data);
  }

  // Gic Its Id
  GicItsInfo->GicItsId = GicItsId;

  // {default = 0}
  GicItsInfo->ProximityDomain = 0;
  return Status;
}

/** CM_ARM_GIC_ITS_INFO parser function.

  This parser expects FdtBranch to be a Gic interrupt-controller node.
  Gic version must be v3 or higher.
  typedef struct CmArmGicItsInfo {
    UINT32  GicItsId;                         // {Populated}
    UINT64  PhysicalBaseAddress;              // {Populated}
    UINT32  ProximityDomain;                  // {default = 0}
  } CM_ARM_GIC_ITS_INFO;

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
ArmGicItsInfoParser (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        INT32                      FdtBranch
  )
{
  EFI_STATUS           Status;
  UINT32               GicVersion;
  CM_ARM_GIC_ITS_INFO  GicItsInfo;
  UINT32               Index;
  INT32                GicItsNode;
  UINT32               GicItsNodeCount;
  VOID                 *Fdt;

  if (FdtParserHandle == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Fdt = FdtParserHandle->Fdt;

  if (!FdtNodeHasProperty (Fdt, FdtBranch, "interrupt-controller")) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Get the Gic version of the interrupt-controller.
  Status = GetGicVersion (Fdt, FdtBranch, &GicVersion);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  if (GicVersion < 3) {
    ASSERT (0);
    return EFI_UNSUPPORTED;
  }

  // Count the nodes with the "msi-controller" property.
  // The interrupt-controller itself can have this property,
  // but the first node is skipped in the search.
  Status = FdtCountPropNodeInBranch (
             Fdt,
             FdtBranch,
             "msi-controller",
             &GicItsNodeCount
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  if (GicItsNodeCount == 0) {
    return EFI_NOT_FOUND;
  }

  GicItsNode = FdtBranch;
  for (Index = 0; Index < GicItsNodeCount; Index++) {
    ZeroMem (&GicItsInfo, sizeof (CM_ARM_GIC_ITS_INFO));

    Status = FdtGetNextPropNodeInBranch (
               Fdt,
               FdtBranch,
               "msi-controller",
               &GicItsNode
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      if (Status == EFI_NOT_FOUND) {
        // Should have found the node.
        Status = EFI_ABORTED;
      }

      return Status;
    }

    Status = GicItsIntcNodeParser (
               Fdt,
               GicItsNode,
               Index,
               &GicItsInfo
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }

    // Add the CmObj to the Configuration Manager.
    Status = AddSingleCmObj (
               FdtParserHandle,
               CREATE_CM_ARM_OBJECT_ID (EArmObjGicItsInfo),
               &GicItsInfo,
               sizeof (CM_ARM_GIC_ITS_INFO),
               NULL
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }
  } // for

  return Status;
}
