/** @file
  Arm Gic Redistributor Parser.

  Copyright (c) 2021, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - linux/Documentation/devicetree/bindings/interrupt-controller/arm,gic-v3.yaml
**/

#include "CmObjectDescUtility.h"
#include "FdtHwInfoParser.h"
#include "Gic/ArmGicDispatcher.h"
#include "Gic/ArmGicRParser.h"

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
  EFI_STATUS              Status;
  UINT32                  Index;
  UINT32                  RedistReg;
  UINT32                  RegSize;
  INT32                   AddressCells;
  INT32                   SizeCells;
  CONST UINT8             *Data;
  INT32                   DataSize;
  CM_ARM_GIC_REDIST_INFO  GicRInfo;
  VOID                    *Fdt;

  if (FdtParserHandle == NULL) {
    ASSERT (0);
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
    ASSERT (0);
    return Status;
  }

  // Don't support more than 64 bits and less than 32 bits addresses.
  if ((AddressCells < 1)  ||
      (AddressCells > 2)  ||
      (SizeCells < 1)     ||
      (SizeCells > 2))
  {
    ASSERT (0);
    return EFI_ABORTED;
  }

  // The "#redistributor-regions" property is optional.
  // It indicates the number of GicR.
  Data = fdt_getprop (Fdt, GicIntcNode, "#redistributor-regions", &DataSize);
  if ((Data != NULL) && (DataSize == sizeof (UINT32))) {
    // If available, must be on one cell.
    RedistReg = fdt32_to_cpu (*(UINT32 *)Data);
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
    ASSERT (0);
    return EFI_ABORTED;
  }

  Data += GET_DT_REG_ADDRESS_OFFSET (1, AddressCells, SizeCells)
          * sizeof (UINT32);
  for (Index = 0; Index < RedistReg; Index++) {
    ZeroMem (&GicRInfo, sizeof (CM_ARM_GIC_REDIST_INFO));

    if (AddressCells == 2) {
      GicRInfo.DiscoveryRangeBaseAddress = fdt64_to_cpu (*(UINT64 *)Data);
    } else {
      GicRInfo.DiscoveryRangeBaseAddress = fdt32_to_cpu (*(UINT32 *)Data);
    }

    Data += sizeof (UINT32) * AddressCells;

    if (SizeCells == 2) {
      GicRInfo.DiscoveryRangeLength = (UINT32)fdt64_to_cpu (*(UINT64 *)Data);
    } else {
      GicRInfo.DiscoveryRangeLength = fdt32_to_cpu (*(UINT32 *)Data);
    }

    // Add the CmObj to the Configuration Manager.
    Status = AddSingleCmObj (
               FdtParserHandle,
               CREATE_CM_ARM_OBJECT_ID (EArmObjGicRedistributorInfo),
               &GicRInfo,
               sizeof (CM_ARM_GIC_REDIST_INFO),
               NULL
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }

    Data += sizeof (UINT32) * SizeCells;
  } // for

  return Status;
}

/** CM_ARM_GIC_REDIST_INFO parser function.

  This parser expects FdtBranch to be a Gic interrupt-controller node.
  Gic version must be v3 or higher.
  typedef struct CmArmGicRedistInfo {
    UINT64  DiscoveryRangeBaseAddress;        // {Populated}
    UINT32  DiscoveryRangeLength;             // {Populated}
  } CM_ARM_GIC_REDIST_INFO;

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
ArmGicRInfoParser (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        INT32                      FdtBranch
  )
{
  EFI_STATUS  Status;
  UINT32      GicVersion;
  VOID        *Fdt;

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

  Status = GicRIntcNodeParser (FdtParserHandle, FdtBranch);
  ASSERT_EFI_ERROR (Status);
  return Status;
}
