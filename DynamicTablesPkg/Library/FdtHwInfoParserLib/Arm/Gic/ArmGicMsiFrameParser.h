/** @file
  Arm Gic Msi frame Parser.

  Copyright (c) 2021, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - linux/Documentation/devicetree/bindings/interrupt-controller/arm,gic.yaml
  - linux/Documentation/devicetree/bindings/interrupt-controller/arm,gic-v3.yaml
**/

#ifndef ARM_GIC_MSI_FRAME_PARSER_H_
#define ARM_GIC_MSI_FRAME_PARSER_H_

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
  );

#endif // ARM_GIC_MSI_FRAME_PARSER_H_
