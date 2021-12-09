/** @file
  Arm Gic Msi frame Parser.

  Copyright (c) 2021, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - linux/Documentation/devicetree/bindings/interrupt-controller/arm,gic.yaml
  - linux/Documentation/devicetree/bindings/interrupt-controller/arm,gic-v3.yaml
**/

#include "CmObjectDescUtility.h"
#include "FdtHwInfoParser.h"
#include "Gic/ArmGicDispatcher.h"
#include "Gic/ArmGicMsiFrameParser.h"

/** List of "compatible" property values for Msi-frame nodes.

  Any other "compatible" value is not supported by this module.
*/
STATIC CONST COMPATIBILITY_STR  MsiFrameCompatibleStr[] = {
  { "arm,gic-v2m-frame" }
};

/** COMPATIBILITY_INFO structure for the MSI frame.
*/
STATIC CONST COMPATIBILITY_INFO  MsiFrameCompatibleInfo = {
  ARRAY_SIZE (MsiFrameCompatibleStr),
  MsiFrameCompatibleStr
};

/** Parse a Msi frame node.

  @param [in]  Fdt            Pointer to a Flattened Device Tree (Fdt).
  @param [in]  MsiFrameNode   Offset of a Msi frame node.
  @param [in]  MsiFrameId     Frame ID.
  @param [out] MsiFrameInfo   The CM_ARM_GIC_MSI_FRAME_INFO to populate.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
MsiFrameNodeParser (
  IN  CONST VOID                 *Fdt,
  IN  INT32                      MsiFrameNode,
  IN  UINT32                     MsiFrameId,
  OUT CM_ARM_GIC_MSI_FRAME_INFO  *MsiFrameInfo
  )
{
  EFI_STATUS   Status;
  INT32        AddressCells;
  CONST UINT8  *Data;
  INT32        DataSize;

  if ((Fdt == NULL) ||
      (MsiFrameInfo == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Status = FdtGetParentAddressInfo (Fdt, MsiFrameNode, &AddressCells, NULL);
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

  Data = fdt_getprop (Fdt, MsiFrameNode, "reg", &DataSize);
  if ((Data == NULL) || (DataSize < (INT32)(AddressCells * sizeof (UINT32)))) {
    // If error or not enough space.
    ASSERT (0);
    return EFI_ABORTED;
  }

  if (AddressCells == 2) {
    MsiFrameInfo->PhysicalBaseAddress = fdt64_to_cpu (*(UINT64 *)Data);
  } else {
    MsiFrameInfo->PhysicalBaseAddress = fdt32_to_cpu (*(UINT32 *)Data);
  }

  MsiFrameInfo->GicMsiFrameId = MsiFrameId;

  return EFI_SUCCESS;
}

/** CM_ARM_GIC_MSI_FRAME_INFO parser function.

  The following structure is populated:
  typedef struct CmArmGicMsiFrameInfo {
    UINT32  GicMsiFrameId;                    // {Populated}
    UINT64  PhysicalBaseAddress;              // {Populated}
    UINT32  Flags;                            // {default = 0}
    UINT16  SPICount;
    UINT16  SPIBase;
  } CM_ARM_GIC_MSI_FRAME_INFO;

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
ArmGicMsiFrameInfoParser (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        INT32                      FdtBranch
  )
{
  EFI_STATUS  Status;
  INT32       MsiFrameNode;
  UINT32      MsiFrameNodeCount;

  UINT32                     Index;
  CM_ARM_GIC_MSI_FRAME_INFO  MsiFrameInfo;
  VOID                       *Fdt;

  if (FdtParserHandle == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Fdt = FdtParserHandle->Fdt;

  // Count the number of nodes having the "interrupt-controller" property.
  Status = FdtCountPropNodeInBranch (
             Fdt,
             FdtBranch,
             "msi-controller",
             &MsiFrameNodeCount
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  if (MsiFrameNodeCount == 0) {
    return EFI_NOT_FOUND;
  }

  // Parse each node having the "msi-controller" property.
  MsiFrameNode = FdtBranch;
  for (Index = 0; Index < MsiFrameNodeCount; Index++) {
    ZeroMem (&MsiFrameInfo, sizeof (CM_ARM_GIC_MSI_FRAME_INFO));

    Status = FdtGetNextPropNodeInBranch (
               Fdt,
               FdtBranch,
               "msi-controller",
               &MsiFrameNode
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      if (Status == EFI_NOT_FOUND) {
        // Should have found the node.
        Status = EFI_ABORTED;
      }

      return Status;
    }

    if (!FdtNodeIsCompatible (Fdt, MsiFrameNode, &MsiFrameCompatibleInfo)) {
      ASSERT (0);
      Status = EFI_UNSUPPORTED;
      return Status;
    }

    // Parse the Msi information.
    Status = MsiFrameNodeParser (
               Fdt,
               MsiFrameNode,
               Index,
               &MsiFrameInfo
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }

    // Add the CmObj to the Configuration Manager.
    Status = AddSingleCmObj (
               FdtParserHandle,
               CREATE_CM_ARM_OBJECT_ID (EArmObjGicMsiFrameInfo),
               &MsiFrameInfo,
               sizeof (CM_ARM_GIC_MSI_FRAME_INFO),
               NULL
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }
  } // for

  return Status;
}
