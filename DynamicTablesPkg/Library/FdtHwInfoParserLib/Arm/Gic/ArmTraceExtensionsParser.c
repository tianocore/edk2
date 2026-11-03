/** @file
  Arm Trace Extensions parser helpers.

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - linux/Documentation/devicetree/bindings/arm/arm,trace-buffer-extension.yaml
**/

#include <Library/ArmLib.h>
#include <Library/BaseLib.h>
#include <Library/FdtLib.h>
#include <Library/DebugLib.h>
#include "FdtHwInfoParser.h"
#include "CmObjectDescUtility.h"
#include "FdtUtility.h"
#include "Arm/Gic/ArmTraceExtensionsParser.h"

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

/** ETE compatible strings.

  Any other "compatible" value is not supported by this module.
*/
STATIC CONST COMPATIBILITY_STR  EteCompatibleStr[] = {
  { "arm,embedded-trace-extension" }
};

/** COMPATIBILITY_INFO structure for the EteCompatibleStr.
*/
STATIC CONST COMPATIBILITY_INFO  EteCompatibleInfo = {
  ARRAY_SIZE (EteCompatibleStr),
  EteCompatibleStr
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
        (AsciiStrnCmp ((CONST CHAR8 *)Prop, "okay", PropSize) != 0) &&
        (AsciiStrnCmp ((CONST CHAR8 *)Prop, "ok", PropSize) != 0))
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
  )
{
  EFI_STATUS        Status;
  INT32             EteNode;
  UINT32            GicCIndex;
  UINT32            CpuIndex;
  CM_ARM_GICC_INFO  *GicCInfo;
  CM_ARM_ET_INFO    EtInfo;
  CONST UINT32      *CpuProp;
  INT32             CpuPropSize;
  INT32             CpuNode;
  CONST VOID        *Prop;
  INT32             PropSize;
  BOOLEAN           EteFound;

  if ((Fdt == NULL)             ||
      (FdtParserHandle == NULL) ||
      (CpuNodeOffsetMap == NULL) ||
      (GicCCmObjDesc == NULL)   ||
      (GicCCmObjDesc->Data == NULL))
  {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  GicCInfo = (CM_ARM_GICC_INFO *)GicCCmObjDesc->Data;
  EteNode  = 0;
  EteFound = FALSE;
  while (TRUE) {
    Status = FdtGetNextCompatNodeInBranch (
               Fdt,
               0,
               &EteCompatibleInfo,
               &EteNode
               );
    if (EFI_ERROR (Status)) {
      if (Status == EFI_NOT_FOUND) {
        // No ETE nodes left.
        break;
      }

      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    //
    // Ignore disabled ETE nodes.
    //
    Prop = FdtGetProp (Fdt, EteNode, "status", &PropSize);
    if ((Prop != NULL) &&
        (AsciiStrnCmp ((CONST CHAR8 *)Prop, "okay", PropSize) != 0) &&
        (AsciiStrnCmp ((CONST CHAR8 *)Prop, "ok", PropSize) != 0))
    {
      continue;
    }

    CpuProp = FdtGetProp (Fdt, EteNode, "cpu", &CpuPropSize);
    if ((CpuProp == NULL) || (CpuPropSize != sizeof (UINT32))) {
      ASSERT (FALSE);
      return EFI_ABORTED;
    }

    CpuNode = FdtNodeOffsetByPhandle (Fdt, Fdt32ToCpu (*CpuProp));
    if (CpuNode < 0) {
      ASSERT (FALSE);
      return EFI_ABORTED;
    }

    GicCIndex = MAX_UINT32;
    for (CpuIndex = 0; CpuIndex < GicCCmObjDesc->Count; CpuIndex++) {
      if (CpuNodeOffsetMap[CpuIndex] == CpuNode) {
        GicCIndex = CpuIndex;
        break;
      }
    }

    if (GicCIndex == MAX_UINT32) {
      ASSERT (FALSE);
      return EFI_NOT_FOUND;
    }

    if (GicCInfo[GicCIndex].EtToken != CM_NULL_TOKEN) {
      ASSERT (FALSE);
      return EFI_ABORTED;
    }

    EtInfo.EtType = ArmEtTypeEte;
    Status        = AddSingleCmObj (
                      FdtParserHandle,
                      CREATE_CM_ARM_OBJECT_ID (EArmObjEtInfo),
                      &EtInfo,
                      sizeof (EtInfo),
                      &GicCInfo[GicCIndex].EtToken
                      );
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    EteFound = TRUE;
  }

  if (!EteFound) {
    return EFI_NOT_FOUND;
  }

  if (!ArmHasEte ()) {
    DEBUG ((DEBUG_WARN, "ETE parser: Parsed ETE info while Ete not supported.\n"));
  }

  return EFI_SUCCESS;
}
