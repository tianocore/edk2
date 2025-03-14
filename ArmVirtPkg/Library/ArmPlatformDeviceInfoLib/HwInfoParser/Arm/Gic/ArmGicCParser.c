/** @file
  Arm Gic cpu parser.

  Copyright (c) 2021 - 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - linux/Documentation/devicetree/bindings/arm/cpus.yaml
  - linux/Documentation/devicetree/bindings/interrupt-controller/arm,gic.yaml
  - linux/Documentation/devicetree/bindings/interrupt-controller/arm,gic-v3.yaml
  - linux/Documentation/devicetree/bindings/arm/pmu.yaml
**/

#include "FdtUtility.h"
#include "FdtInfoParser.h"
#include "ArmGicCParser.h"
#include "ArmGicDispatcher.h"

/** Parse a "cpus" node and its children "cpu" nodes and return
    the CPU node count.

  @param [in]  Fdt              Pointer to a Flattened Device Tree (Fdt).
  @param [out] CpuNodeCount     The count of CPU nodes.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
CpusNodeParser (
  IN  CONST VOID    *Fdt,
  OUT       UINT32  *CpuNodeCount
  )
{
  EFI_STATUS  Status;
  INT32       CpusNode;
  INT32       AddressCells;

  if (Fdt == NULL) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  // The "cpus" node resides at the root of the DT. Fetch it.
  CpusNode = fdt_path_offset (Fdt, "/cpus");
  if (CpusNode < 0) {
    return EFI_NOT_FOUND;
  }

  AddressCells = fdt_address_cells (Fdt, CpusNode);
  if (AddressCells < 0) {
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  // Count the number of "cpu" nodes under the "cpus" node.
  Status = FdtCountNamedNodeInBranch (Fdt, CpusNode, "cpu", CpuNodeCount);
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    return Status;
  }

  if (*CpuNodeCount == 0) {
    ASSERT (FALSE);
    return EFI_NOT_FOUND;
  }

  return Status;
}

/** Parse a Gic compatible interrupt-controller node,
    extracting GicCv2 information.

  @param [in]  FdtParserHandle    A handle to the parser instance.
  @param [in]  Gicv2IntcNode      Offset of a Gicv2 compatible
                                  interrupt-controller node.
  @param [in]  CpuNodeCount       The count of CPU nodes.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
GicCv2IntcNodeParser (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        INT32                      Gicv2IntcNode,
  IN        UINT32                     CpuNodeCount
  )
{
  EFI_STATUS   Status;
  UINT32       Index;
  INT32        AddressCells;
  INT32        SizeCells;
  VOID         *Fdt;
  CONST UINT8  *GicCValue;
  CONST UINT8  *GicCRange;

  CONST UINT8  *Data;
  INT32        DataSize;
  UINT32       RegSize;
  UINT32       RegCount;
  UINT64       PhysicalBaseAddress;
  UINT64       PhysicalBaseAddressRange;

  if (FdtParserHandle == NULL) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Fdt = FdtParserHandle->Fdt;

  GicCValue = NULL;
  GicCRange = NULL;

  // Get the #address-cells and #size-cells property values.
  Status = FdtGetParentAddressInfo (
             Fdt,
             Gicv2IntcNode,
             &AddressCells,
             &SizeCells
             );
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    return Status;
  }

  // Don't support more than 64 bits and less than 32 bits addresses.
  if ((AddressCells < 1)  ||
      (AddressCells > 2)  ||
      (SizeCells < 1)     ||
      (SizeCells > 2))
  {
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  RegSize = (AddressCells + SizeCells) * sizeof (UINT32);

  Data = fdt_getprop (Fdt, Gicv2IntcNode, "reg", &DataSize);
  if ((Data == NULL)  ||
      (DataSize < 0)  ||
      ((DataSize % RegSize) != 0))
  {
    // If error or wrong size.
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  RegCount = DataSize/RegSize;

  switch (RegCount) {
    case 2:
    {
      // GicC is at index 1 in the reg property. GicC is mandatory.
      GicCValue = Data + (sizeof (UINT32) *
                          GET_DT_REG_ADDRESS_OFFSET (1, AddressCells, SizeCells));
      GicCRange = Data + (sizeof (UINT32) *
                          GET_DT_REG_ADDRESS_OFFSET (2, AddressCells, SizeCells));
      break;
    }
    default:
    {
      // Not enough or too much information.
      ASSERT (FALSE);
      return EFI_ABORTED;
    }
  }

  for (Index = 0; Index < CpuNodeCount; Index++) {
    if (AddressCells == 2) {
      PhysicalBaseAddress      = FdtReadUnaligned64 ((UINT64 *)GicCValue);
      PhysicalBaseAddressRange = FdtReadUnaligned64 ((UINT64 *)GicCRange);
    } else {
      PhysicalBaseAddress      = FdtReadUnaligned32 ((UINT32 *)GicCValue);
      PhysicalBaseAddressRange = FdtReadUnaligned32 ((UINT32 *)GicCRange);
    }

    if (FdtParserHandle->HwAddressInfo != NULL) {
      Status = FdtParserHandle->HwAddressInfo (
                                  FdtParserHandle->Context,
                                  "GICC GicV2",
                                  PhysicalBaseAddress,
                                  PhysicalBaseAddressRange
                                  );
      if (EFI_ERROR (Status)) {
        ASSERT (FALSE);
        break;
      }
    }
  } // for

  return Status;
}

/** Parse a Gic compatible interrupt-controller node,
    extracting GicCv3 information.

  @param [in]  FdtParserHandle    A handle to the parser instance.
  @param [in]  Gicv3IntcNode      Offset of a Gicv3 compatible
                                  interrupt-controller node.
  @param [in]  CpuNodeCount       The count of CPU nodes.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
GicCv3IntcNodeParser (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        INT32                      Gicv3IntcNode,
  IN        UINT32                     CpuNodeCount
  )
{
  EFI_STATUS   Status;
  UINT32       Index;
  INT32        AddressCells;
  INT32        SizeCells;
  UINT32       AdditionalRedistReg;
  UINT64       PhysicalBaseAddress;
  UINT64       PhysicalBaseAddressRange;
  VOID         *Fdt;
  CONST UINT8  *GicCValue;
  CONST UINT8  *GicCRange;

  CONST UINT8  *Data;
  INT32        DataSize;
  UINT32       RegSize;
  UINT32       RegCount;

  if (FdtParserHandle == NULL) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Fdt = FdtParserHandle->Fdt;

  GicCValue = NULL;
  GicCRange = NULL;

  // Get the #address-cells and #size-cells property values.
  Status = FdtGetParentAddressInfo (
             Fdt,
             Gicv3IntcNode,
             &AddressCells,
             &SizeCells
             );
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    return Status;
  }

  // Don't support more than 64 bits and less than 32 bits addresses.
  if ((AddressCells < 1) ||
      (AddressCells > 2) ||
      (SizeCells < 1) ||
      (SizeCells > 2))
  {
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  // The "#redistributor-regions" property is optional.
  Data = fdt_getprop (Fdt, Gicv3IntcNode, "#redistributor-regions", &DataSize);
  if ((Data != NULL) && (DataSize == sizeof (UINT32))) {
    ASSERT (FdtReadUnaligned32 ((UINT32 *)Data) > 1);
    AdditionalRedistReg = FdtReadUnaligned32 ((UINT32 *)Data) - 1;
  } else {
    AdditionalRedistReg = 0;
  }

  RegSize = (AddressCells + SizeCells) * sizeof (UINT32);

  /*
    Ref: linux/blob/master/Documentation/devicetree/bindings/
         interrupt-controller/arm%2Cgic-v3.yaml

    reg:
    description: |
      Specifies base physical address(s) and size of the GIC
      registers, in the following order:
      - GIC Distributor interface (GICD)
      - GIC Redistributors (GICR), one range per redistributor region
      - GIC CPU interface (GICC)
      - GIC Hypervisor interface (GICH)
      - GIC Virtual CPU interface (GICV)
      GICC, GICH and GICV are optional.
    minItems: 2
    maxItems: 4096
  */
  Data = fdt_getprop (Fdt, Gicv3IntcNode, "reg", &DataSize);
  if ((Data == NULL) ||
      (DataSize < 0) ||
      ((DataSize % RegSize) != 0))
  {
    // If error or wrong size.
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  RegCount = (DataSize / RegSize) - AdditionalRedistReg;

  // The GicD and GicR info is mandatory.
  switch (RegCount) {
    case 3:
    {
      // GicC is at index 2 in the reg property. GicC is optional.
      // Even though GicC is optional, it is made mandatory in this parser.
      GicCValue = Data + (sizeof (UINT32) *
                          GET_DT_REG_ADDRESS_OFFSET (
                            2 + AdditionalRedistReg,
                            AddressCells,
                            SizeCells
                            ));
      GicCRange = Data + (sizeof (UINT32) *
                          GET_DT_REG_ADDRESS_OFFSET (
                            3 + AdditionalRedistReg,
                            AddressCells,
                            SizeCells
                            ));
      // fall-through
    }
    case 2:
    {
      // GicR
      // GicD
      break;
    }
    default:
    {
      // Not enough or too much information.
      ASSERT (FALSE);
      return EFI_ABORTED;
    }
  } // switch

  if ((GicCValue != NULL) && (GicCRange != NULL)) {
    for (Index = 0; Index < CpuNodeCount; Index++) {
      if (AddressCells == 2) {
        PhysicalBaseAddress      = FdtReadUnaligned64 ((UINT64 *)GicCValue);
        PhysicalBaseAddressRange = FdtReadUnaligned64 ((UINT64 *)GicCRange);
      } else {
        PhysicalBaseAddress      = FdtReadUnaligned32 ((UINT32 *)GicCValue);
        PhysicalBaseAddressRange = FdtReadUnaligned32 ((UINT32 *)GicCRange);
      }

      if (FdtParserHandle->HwAddressInfo != NULL) {
        Status = FdtParserHandle->HwAddressInfo (
                                    FdtParserHandle->Context,
                                    "GICC GicV3",
                                    PhysicalBaseAddress,
                                    PhysicalBaseAddressRange
                                    );
        if (EFI_ERROR (Status)) {
          ASSERT (FALSE);
          break;
        }
      }
    } // for
  }

  return Status;
}

/** GICC information parser function.

  @param [in]  FdtParserHandle  A handle to the parser instance.
  @param [in]  IntcNode         Offset of the interrupt-controller node.
  @param [in]  CpuNodeCount     The count of CPU nodes.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           Not found.
  @retval EFI_UNSUPPORTED         Unsupported.
**/
EFI_STATUS
EFIAPI
ArmGicCInfoParser (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        INT32                      IntcNode,
  IN  CONST UINT32                     GicVersion
  )
{
  EFI_STATUS  Status;
  VOID        *Fdt;
  UINT32      CpuNodeCount;

  if (FdtParserHandle == NULL) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Fdt = FdtParserHandle->Fdt;

  // Parse the "cpus" nodes and its children "cpu" nodes, and
  // return the CPU node count.
  Status = CpusNodeParser (Fdt, &CpuNodeCount);
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    return Status;
  }

  // Parse the interrupt-controller node according to the Gic version.
  switch (GicVersion) {
    case 2:
    {
      Status = GicCv2IntcNodeParser (FdtParserHandle, IntcNode, CpuNodeCount);
      break;
    }
    case 3:
    {
      Status = GicCv3IntcNodeParser (FdtParserHandle, IntcNode, CpuNodeCount);
      break;
    }
    default:
    {
      // Unsupported Gic version.
      ASSERT (FALSE);
      Status = EFI_UNSUPPORTED;
    }
  }

  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    goto exit_handler;
  }

exit_handler:
  return Status;
}
