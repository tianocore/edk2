/** @file
  Arm Gic dispatcher.

  Copyright (c) 2021 - 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - linux/Documentation/devicetree/bindings/interrupt-controller/arm,gic.yaml
  - linux/Documentation/devicetree/bindings/interrupt-controller/arm,gic-v3.yaml
**/

#include "FdtUtility.h"
#include "FdtInfoParser.h"
#include "ArmGicCParser.h"
#include "ArmGicDispatcher.h"
#include "ArmGicDParser.h"
#include "ArmGicRParser.h"

/** List of "compatible" property values for GicV2 interrupt nodes.

  Any other "compatible" value is not supported by this module.
*/
STATIC CONST COMPATIBILITY_STR  GicV2CompatibleStr[] = {
  { "arm,cortex-a15-gic" }
};

/** COMPATIBILITY_INFO structure for the GICv2.
*/
CONST COMPATIBILITY_INFO  GicV2CompatibleInfo = {
  ARRAY_SIZE (GicV2CompatibleStr),
  GicV2CompatibleStr
};

/** List of "compatible" property values for GicV3 interrupt nodes.

  Any other "compatible" value is not supported by this module.
*/
STATIC CONST COMPATIBILITY_STR  GicV3CompatibleStr[] = {
  { "arm,gic-v3" }
};

/** COMPATIBILITY_INFO structure for the GICv3.
*/
CONST COMPATIBILITY_INFO  GicV3CompatibleInfo = {
  ARRAY_SIZE (GicV3CompatibleStr),
  GicV3CompatibleStr
};

/** Get the Gic version of an interrupt-controller node.

  @param [in]  Fdt          Pointer to a Flattened Device Tree (Fdt).
  @param [in]  IntcNode     Interrupt-controller node.
  @param [out] GicVersion   If success, contains the Gic version of the
                            interrupt-controller node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_UNSUPPORTED         Unsupported.
**/
STATIC
EFI_STATUS
EFIAPI
GetGicVersion (
  IN  CONST VOID    *Fdt,
  IN        INT32   IntcNode,
  OUT       UINT32  *GicVersion
  )
{
  if ((Fdt == NULL) ||
      (GicVersion == NULL))
  {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  if (FdtNodeIsCompatible (Fdt, IntcNode, &GicV2CompatibleInfo)) {
    *GicVersion = 2;
  } else if (FdtNodeIsCompatible (Fdt, IntcNode, &GicV3CompatibleInfo)) {
    *GicVersion = 3;
  } else {
    // Unsupported Gic version.
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/** Gic dispatcher.

  This disptacher parses the Device tree for the following infromation:
   - GICC
   - GICD
   - GICR

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
ArmGicDispatcher (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        INT32                      FdtBranch
  )
{
  EFI_STATUS  Status;
  INT32       CpusNode;
  INT32       IntcNode;
  UINT32      GicVersion;
  VOID        *Fdt;

  if (FdtParserHandle == NULL) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Fdt = FdtParserHandle->Fdt;

  // The "cpus" node resides at the root of the DT. Fetch it.
  CpusNode = fdt_path_offset (Fdt, "/cpus");
  if (CpusNode < 0) {
    return EFI_NOT_FOUND;
  }

  // Get the interrupt-controller node associated to the "cpus" node.
  Status = FdtGetIntcParentNode (Fdt, CpusNode, &IntcNode);
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    if (Status == EFI_NOT_FOUND) {
      // Should have found the node.
      Status = EFI_ABORTED;
    }

    return Status;
  }

  Status = GetGicVersion (Fdt, IntcNode, &GicVersion);
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    return Status;
  }

  // Parse the GicC information.
  Status = ArmGicCInfoParser (FdtParserHandle, IntcNode, GicVersion);
  if (EFI_ERROR (Status)) {
    // Don't try to parse GicD and GicMsiFrame information
    // if no GicC information is found. Return.
    ASSERT (Status == EFI_NOT_FOUND);
    return Status;
  }

  // Parse the GicD information of the "cpus" interrupt-controller node.
  Status = ArmGicDInfoParser (FdtParserHandle, IntcNode, GicVersion);
  if (EFI_ERROR (Status)) {
    // EFI_NOT_FOUND is not tolerated at this point.
    ASSERT (FALSE);
    return Status;
  }

  switch (GicVersion) {
    case 4:
    case 3:
    {
      // Parse the GicR information of the interrupt-controller node.
      Status = ArmGicRInfoParser (FdtParserHandle, IntcNode, GicVersion);
      if (EFI_ERROR (Status)) {
        // EFI_NOT_FOUND is not tolerated at this point.
        ASSERT (FALSE);
        return Status;
      }

      break;
    }
    default:
    {
      ASSERT (FALSE);
      return EFI_UNSUPPORTED;
    }
  }

  return EFI_SUCCESS;
}
