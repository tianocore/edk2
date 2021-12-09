/** @file
  Arm Gic dispatcher.

  Copyright (c) 2021, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - linux/Documentation/devicetree/bindings/interrupt-controller/arm,gic.yaml
  - linux/Documentation/devicetree/bindings/interrupt-controller/arm,gic-v3.yaml
**/

#ifndef ARM_GIC_DISPATCHER_H_
#define ARM_GIC_DISPATCHER_H_

#include <FdtHwInfoParserInclude.h>
#include "FdtUtility.h"

/** COMPATIBILITY_INFO structure for the GICv2.
*/
extern CONST COMPATIBILITY_INFO  GicV2CompatibleInfo;

/** Get the Gic version of the interrupt-controller node.

  @param [in]  Fdt          Pointer to a Flattened Device Tree (Fdt).
  @param [in]  IntcNode     Interrupt-controller node.
  @param [out] GicVersion   If success, contains the Gic version of the
                            interrupt-controller node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_UNSUPPORTED         Unsupported.
**/
EFI_STATUS
EFIAPI
GetGicVersion (
  IN  CONST VOID    *Fdt,
  IN        INT32   IntcNode,
  OUT       UINT32  *GicVersion
  );

/** Gic dispatcher.

  This disptacher populates the following structures:
   - CM_ARM_GICC_INFO
   - CM_ARM_GICD_INFO
   - CM_ARM_GIC_MSI_FRAME_INFO

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
ArmGicDispatcher (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        INT32                      FdtBranch
  );

#endif // ARM_GIC_DISPATCHER_H_
