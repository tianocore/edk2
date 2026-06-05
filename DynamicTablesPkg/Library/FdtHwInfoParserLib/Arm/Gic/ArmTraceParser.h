/** @file
  Arm trace parser helpers.

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - linux/Documentation/devicetree/bindings/arm/arm,trace-buffer-extension.yaml
**/

#pragma once

/** Get the TRBE interrupt.

  Search the Device Tree for a TRBE node and return the decoded interrupt
  GSIV from its "interrupts" property.

  @param [in]  Fdt            Pointer to a Flattened Device Tree (Fdt).
  @param [out] TrbeInterrupt  If success, contains the TRBE interrupt GSIV.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           No TRBE node was found.
**/
EFI_STATUS
EFIAPI
ArmGetTrbeInterrupt (
  IN  CONST VOID    *Fdt,
  OUT       UINT16  *TrbeInterrupt
  );

/** Parse ETE nodes and attach the generated ET objects to their CPUs.

  This function modifies the input CM_ARM_GICC_INFO array to set:
    - EtToken

  For each discovered ETE node, add one CM_ARM_ET_INFO object to the
  Configuration Manager and store its generated token in the matching GICC.

  @param [in]      Fdt               Pointer to a Flattened Device Tree (Fdt).
  @param [in]      FdtParserHandle   A handle to the parser instance.
  @param [in]      CpuNodeOffsetMap  Array mapping each GICC index to the
                                     corresponding CPU DT node offset.
  @param [in, out] GicCCmObjDesc     The CM_ARM_GICC_INFO array to patch.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           No ETE node was found.
  @retval EFI_OUT_OF_RESOURCES    Memory allocation failed.
**/
EFI_STATUS
EFIAPI
ArmTraceAddEtInfo (
  IN      CONST VOID                 *Fdt,
  IN      FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN      CONST INT32                *CpuNodeOffsetMap,
  IN  OUT       CM_OBJ_DESCRIPTOR    *GicCCmObjDesc
  );
