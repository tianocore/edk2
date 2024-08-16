/** @file
  Arm Gic cpu parser.

  Copyright (c) 2021 - 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - linux/Documentation/devicetree/bindings/interrupt-controller/arm,gic.yaml
  - linux/Documentation/devicetree/bindings/interrupt-controller/arm,gic-v3.yaml
**/

#ifndef ARM_GICC_PARSER_H_
#define ARM_GICC_PARSER_H_

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
  );

#endif // ARM_GICC_PARSER_H_
