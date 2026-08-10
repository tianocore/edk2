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
