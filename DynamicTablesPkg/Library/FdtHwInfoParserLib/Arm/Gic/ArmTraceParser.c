/** @file
  Arm trace parser helpers.

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - linux/Documentation/devicetree/bindings/arm/arm,trace-buffer-extension.yaml
**/

#include <Library/ArmLib.h>
#include <Library/BaseLib.h>
#include <Library/FdtLib.h>
#include <Library/DebugLib.h>
#include "FdtUtility.h"
#include "Arm/Gic/ArmTraceParser.h"

/** TRBE compatible strings.

  Any other "compatible" value is not supported by this module.
*/
STATIC CONST COMPATIBILITY_STR  TrbeCompatibleStr[] = {
  { "arm,trace-buffer-extension" }
};

/** COMPATIBILITY_INFO structure for TRBE nodes.
*/
STATIC CONST COMPATIBILITY_INFO  TrbeCompatibleInfo = {
  ARRAY_SIZE (TrbeCompatibleStr),
  TrbeCompatibleStr
};

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
  )
{
  EFI_STATUS    Status;
  INT32         PropSize;
  INT32         TrbeNode;
  INT32         DecodedInterruptCells;
  UINT32        Interrupt;
  CONST UINT32  *DecodedInterruptData;
  CONST VOID    *Prop;

  if ((Fdt == NULL) || (TrbeInterrupt == NULL)) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  TrbeNode = 0;
  while (TRUE) {
    Status = FdtGetNextCompatNodeInBranch (
               Fdt,
               0,
               &TrbeCompatibleInfo,
               &TrbeNode
               );
    if (EFI_ERROR (Status)) {
      if (Status == EFI_NOT_FOUND) {
        return EFI_NOT_FOUND;
      }

      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    /*
     * Ignore disabled TRBE nodes.
     */
    Prop = FdtGetProp (Fdt, TrbeNode, "status", &PropSize);
    if ((Prop != NULL) &&
        (AsciiStrnCmp ((CONST CHAR8 *)Prop, "okay", PropSize) != 0))
    {
      continue;
    }

    Status = FdtResolveInterrupt (
               Fdt,
               TrbeNode,
               0,
               &DecodedInterruptData,
               &DecodedInterruptCells
               );
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    Interrupt = FdtGetInterruptId (DecodedInterruptData, DecodedInterruptCells);
    if (Interrupt > MAX_UINT16) {
      ASSERT (FALSE);
      return EFI_ABORTED;
    }

    if (!ArmHasTrbe ()) {
      DEBUG ((DEBUG_WARN, "TRBE parser: Parsed TRBE info while Trbe not supported.\n"));
    }

    *TrbeInterrupt = (UINT16)Interrupt;
    return EFI_SUCCESS;
  }
}
