/** @file
  Arm Gic Redistributor Parser.

  Copyright (c) 2021 - 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - linux/Documentation/devicetree/bindings/interrupt-controller/arm,gic-v3.yaml
**/

#include "FdtUtility.h"
#include "FdtInfoParser.h"
#include "ArmGicDispatcher.h"
#include "ArmGicRParser.h"

/** Parse a Gic compatible interrupt-controller node,
    extracting GicR information.

  This parser is valid for Gic v3 and higher.

  @param [in]  FdtParserHandle  A handle to the parser instance.
  @param [in]  GicIntcNode      Offset of a Gic compatible
                                interrupt-controller node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
GicRIntcNodeParser (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        INT32                      GicIntcNode
  )
{
  EFI_STATUS   Status;
  UINT32       Index;
  UINT32       RedistReg;
  UINT32       RegSize;
  INT32        AddressCells;
  INT32        SizeCells;
  CONST UINT8  *Data;
  INT32        DataSize;
  VOID         *Fdt;
  UINT64       DiscoveryRangeBaseAddress;
  UINT64       DiscoveryRangeLength;

  if (FdtParserHandle == NULL) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Fdt = FdtParserHandle->Fdt;

  Status = FdtGetParentAddressInfo (
             Fdt,
             GicIntcNode,
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

  // The "#redistributor-regions" property is optional.
  // It indicates the number of GicR.
  Data = fdt_getprop (Fdt, GicIntcNode, "#redistributor-regions", &DataSize);
  if ((Data != NULL) && (DataSize == sizeof (UINT32))) {
    // If available, must be on one cell.
    RedistReg = FdtReadUnaligned32 ((UINT32 *)Data);
  } else {
    // The DT Spec says GicR is mandatory so we will
    // always have one.
    RedistReg = 1;
  }

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

    Example:
      interrupt-controller@2c010000 {
        compatible = "arm,gic-v3";
        #interrupt-cells = <4>;
        #address-cells = <1>;
        #size-cells = <1>;
        ranges;
        interrupt-controller;
        redistributor-stride = <0x0 0x40000>;  // 256kB stride
        #redistributor-regions = <2>;
        reg = <0x2c010000 0x10000>,  // GICD
              <0x2d000000 0x800000>,  // GICR 1: CPUs 0-31
              <0x2e000000 0x800000>,  // GICR 2: CPUs 32-63
              <0x2c040000 0x2000>,  // GICC
              <0x2c060000 0x2000>,  // GICH
              <0x2c080000 0x2000>;  // GICV
        interrupts = <1 9 4>;
        ...
      }
  */
  RegSize = (AddressCells + SizeCells) * sizeof (UINT32);
  Data    = fdt_getprop (Fdt, GicIntcNode, "reg", &DataSize);
  if ((Data == NULL)  ||
      (DataSize < 0)  ||
      ((DataSize % RegSize) != 0))
  {
    // If error or wrong size.
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  Data += GET_DT_REG_ADDRESS_OFFSET (1, AddressCells, SizeCells)
          * sizeof (UINT32);
  for (Index = 0; Index < RedistReg; Index++) {
    if (AddressCells == 2) {
      DiscoveryRangeBaseAddress = FdtReadUnaligned64 ((UINT64 *)Data);
    } else {
      DiscoveryRangeBaseAddress = FdtReadUnaligned32 ((UINT32 *)Data);
    }

    Data += sizeof (UINT32) * AddressCells;

    if (SizeCells == 2) {
      DiscoveryRangeLength = FdtReadUnaligned64 ((UINT64 *)Data);
    } else {
      DiscoveryRangeLength = FdtReadUnaligned32 ((UINT32 *)Data);
    }

    if (FdtParserHandle->HwAddressInfo != NULL) {
      Status = FdtParserHandle->HwAddressInfo (
                                  FdtParserHandle->Context,
                                  "GICR",
                                  DiscoveryRangeBaseAddress,
                                  DiscoveryRangeLength
                                  );
      if (EFI_ERROR (Status)) {
        ASSERT (FALSE);
        break;
      }
    }

    Data += sizeof (UINT32) * SizeCells;
  } // for

  return Status;
}

/** GICR information parser function.

  @param [in]  FdtParserHandle  A handle to the parser instance.
  @param [in]  FdtBranch        When searching for DT node name, restrict
                                the search to this Device Tree branch.
  @param [in]  GicVersion       The version of the GIC.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           Not found.
  @retval EFI_UNSUPPORTED         Unsupported.
**/
EFI_STATUS
EFIAPI
ArmGicRInfoParser (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        INT32                      FdtBranch,
  IN  CONST UINT32                     GicVersion
  )
{
  EFI_STATUS  Status;
  VOID        *Fdt;

  if (FdtParserHandle == NULL) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Fdt = FdtParserHandle->Fdt;

  if (!FdtNodeHasProperty (Fdt, FdtBranch, "interrupt-controller")) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  if (GicVersion < 3) {
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  Status = GicRIntcNodeParser (FdtParserHandle, FdtBranch);
  ASSERT_EFI_ERROR (Status);
  return Status;
}
