/** @file
  RISC-V Hart Information dispatcher.

  Copyright (c) 2024, Ventana Micro Systems Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/FdtLib.h>
#include "FdtHwInfoParser.h"
#include "CmObjectDescUtility.h"
#include "HartInfo/RiscVHartInfoDispatcher.h"

/** List of "compatible" property values for CPU nodes.

  Any other "compatible" value is not supported by this module.
*/
STATIC CONST COMPATIBILITY_STR  CpuCompatibleStr[] = {
  { "riscv" }
};

/** COMPATIBILITY_INFO structure for CPU nodes.
*/
STATIC CONST COMPATIBILITY_INFO  CpuCompatibleInfo = {
  ARRAY_SIZE (CpuCompatibleStr),
  CpuCompatibleStr
};

/** List of "compatible" property values for timer node.

  Any other "compatible" value is not supported by this module.
*/
STATIC CONST COMPATIBILITY_STR  TimerCompatibleStr[] = {
  { "riscv,timer" }
};

/** COMPATIBILITY_INFO structure for timer node.
*/
STATIC CONST COMPATIBILITY_INFO  TimerCompatibleInfo = {
  ARRAY_SIZE (TimerCompatibleStr),
  TimerCompatibleStr
};

/** Get compatible node in FDT

  @param [in]  FdtParserHandle Pointer to device tree.
  @param [in]  CompatInfo      Pointer to compatibility info.
  @param [out] TargetNode      Pointer to target Node.

  @retval TRUE                 The AplicNode is S-mode APLIC
  @retval FALSE                The AplicNode is not S-mode APLIC
**/
STATIC
EFI_STATUS
FdtGetCompatNode (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN  CONST VOID                       *CompatInfo,
  OUT INT32                            *TargetNode
  )
{
  INT32  Prev;
  INT32  Node;
  VOID   *Fdt;

  Fdt = FdtParserHandle->Fdt;

  for (Prev = 0; ; Prev = Node) {
    Node = FdtNextNode (Fdt, Prev, NULL);
    if (Node < 0) {
      return EFI_NOT_FOUND;
    }

    if (FdtNodeIsCompatible (Fdt, Node, CompatInfo)) {
      *TargetNode = Node;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/** Get CMO block size

  CMO block size in ACPI table is power of 2 value.

  @param [in]  Val       CBO size.

  @retval Ret            Exponent value when Val is represented in power of 2.
**/
STATIC
UINT32
CmoGetBlockSize (
  UINT32  Val
  )
{
  UINT32  Ret;

  Ret = 0;
  while (Val > 1) {
    Ret++;
    Val >>= 1;
  }

  return Ret;
}

/** Create CMO info structure if CMO extension present

  Create CMO structure with CBOM, CBOP and CBOZ sizes.

  @param [in]  FdtParserHandle    A handle to the parser instance.
  @param [in]  CpusNode           cpus node.

**/
STATIC
EFI_STATUS
EFIAPI
RiscVCmoInfoParser (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        INT32                      CpusNode
  )
{
  CM_RISCV_CMO_NODE  CmoInfo;
  CONST UINT32       *Prop;
  EFI_STATUS         Status;
  INT32              CpuNode, Len;
  VOID               *Fdt;

  Fdt     = FdtParserHandle->Fdt;
  CpuNode = CpusNode;
  Status  = FdtGetNextNamedNodeInBranch (Fdt, CpusNode, "cpu", &CpuNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Parse the "cpu" node.
  if (!FdtNodeIsCompatible (Fdt, CpuNode, &CpuCompatibleInfo)) {
    ASSERT (0);
    Status = EFI_ABORTED;
    return Status;
  }

  ZeroMem (&CmoInfo, sizeof (CM_RISCV_CMO_NODE));
  Prop = FdtGetProp (Fdt, CpuNode, "riscv,cbom-block-size", &Len);
  if (Prop) {
    CmoInfo.CbomBlockSize = CmoGetBlockSize (Fdt32ToCpu (*(const UINT32 *)Prop));
  } else {
    DEBUG (
      (
       DEBUG_VERBOSE,
       "%a: Failed to parse cpu node: riscv,cbom-block-size\n",
       __func__
      )
      );
    return EFI_NOT_FOUND;
  }

  Prop = FdtGetProp (Fdt, CpuNode, "riscv,cboz-block-size", &Len);
  if (Prop) {
    CmoInfo.CbozBlockSize = CmoGetBlockSize (Fdt32ToCpu (*(const UINT32 *)Prop));
  } else {
    DEBUG (
      (
       DEBUG_VERBOSE,
       "%a: Failed to parse cpu node: riscv,cboz-block-size\n",
       __func__
      )
      );
  }

  Prop = FdtGetProp (Fdt, CpuNode, "riscv,cbop-block-size", &Len);
  if (Prop) {
    CmoInfo.CbopBlockSize = CmoGetBlockSize (Fdt32ToCpu (*(const UINT32 *)Prop));
  } else {
    DEBUG (
      (
       DEBUG_VERBOSE,
       "%a: Failed to parse cpu node: riscv,cbop-block-size\n",
       __func__
      )
      );
  }

  // Add the CmObj to the Configuration Manager.
  Status = AddSingleCmObj (
             FdtParserHandle,
             CREATE_CM_RISCV_OBJECT_ID (ERiscVObjCmoInfo),
             &CmoInfo,
             sizeof (CM_RISCV_CMO_NODE),
             NULL
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
  }

  return Status;
}

/** Create ISA string Info structure

  @param [in]  FdtParserHandle    A handle to the parser instance.
  @param [in]  CpusNode           cpus node.
**/
STATIC
EFI_STATUS
EFIAPI
RiscVIsaStringInfoParser (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        INT32                      CpusNode
  )
{
  CM_RISCV_ISA_STRING_NODE  IsaStringInfo;
  CONST UINT32              *Prop;
  EFI_STATUS                Status;
  INT32                     Len, CpuNode;
  INT32                     AlignedLen;
  VOID                      *Fdt;

  Fdt     = FdtParserHandle->Fdt;
  CpuNode = CpusNode;
  Status  = FdtGetNextNamedNodeInBranch (Fdt, CpusNode, "cpu", &CpuNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Parse the "cpu" node.
  if (!FdtNodeIsCompatible (Fdt, CpuNode, &CpuCompatibleInfo)) {
    ASSERT (0);
    return Status;
  }

  ZeroMem (&IsaStringInfo, sizeof (CM_RISCV_ISA_STRING_NODE));
  Prop = FdtGetProp (Fdt, CpuNode, "riscv,isa", &Len);
  if (!Prop) {
    DEBUG (
      (
       DEBUG_ERROR,
       "%a: Failed to parse cpu node: riscv,isa\n",
       __func__
      )
      );
    ASSERT (0);
    return EFI_ABORTED;
  }

  IsaStringInfo.Length    = Len;
  AlignedLen              = ALIGN_VALUE (Len, 2);
  IsaStringInfo.IsaString = AllocateZeroPool (AlignedLen);
  if (IsaStringInfo.IsaString == NULL) {
    ASSERT (0);
    return EFI_ABORTED;
  }

  Status = AsciiStrCpyS (
             IsaStringInfo.IsaString,
             Len,
             (CHAR8 *)Prop
             );
  // Add the CmObj to the Configuration Manager.
  Status = AddSingleCmObj (
             FdtParserHandle,
             CREATE_CM_RISCV_OBJECT_ID (ERiscVObjIsaStringInfo),
             &IsaStringInfo,
             sizeof (CM_RISCV_ISA_STRING_NODE) + AlignedLen,
             NULL
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
  }

  return Status;
}

/** Create Timer Info structure.

  Create Timer info structure with time base frequency and flag.

  @param [in]  FdtParserHandle     A handle to the parser instance.
  @param [in]  CpusNode            cpus node.
**/
STATIC
EFI_STATUS
EFIAPI
RiscVTimerInfoParser (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        INT32                      CpusNode
  )
{
  CM_RISCV_TIMER_INFO  TimerInfo;
  CONST UINT64         *Prop;
  EFI_STATUS           Status;
  INT32                Len, TimerNode;
  VOID                 *Fdt;

  Fdt = FdtParserHandle->Fdt;

  ZeroMem (&TimerInfo, sizeof (CM_RISCV_TIMER_INFO));
  if (CpusNode < 0) {
    return EFI_INVALID_PARAMETER;
  }

  Prop = FdtGetProp (Fdt, CpusNode, "timebase-frequency", &Len);
  if (Prop) {
    TimerInfo.TimeBaseFrequency = Fdt32ToCpu (*(const UINT32 *)Prop);
  } else {
    DEBUG (
      (
       DEBUG_ERROR,
       "%a: Failed to parse cpu node:timebase-frequency\n",
       __func__
      )
      );
    return EFI_NOT_FOUND;
  }

  Status = FdtGetCompatNode (FdtParserHandle, &TimerCompatibleInfo, &TimerNode);
  if (!EFI_ERROR (Status)) {
    Prop = FdtGetProp (Fdt, TimerNode, "riscv,timer-cannot-wake-cpu", &Len);
    if (Prop) {
      TimerInfo.TimerCannotWakeCpu = RISCV_TIMER_CANNOT_WAKE_CPU;
    }
  }

  Status = AddSingleCmObj (
             FdtParserHandle,
             CREATE_CM_RISCV_OBJECT_ID (ERiscVObjTimerInfo),
             &TimerInfo,
             sizeof (CM_RISCV_TIMER_INFO),
             NULL
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
  }

  return Status;
}

/** Hart Info dispatcher.

  A parser parses a Device Tree to populate a specific CmObj type. None,
  one or many CmObj can be created by the parser.
  The created CmObj are then handed to the parser's caller through the
  HW_INFO_ADD_OBJECT interface.
  This can also be a dispatcher. I.e. a function that not parsing a
  Device Tree but calling other parsers.

  @param [in]  FdtParserHandle    A handle to the parser instance.
  @param [in]  FdtBranch          When searching for DT node name, restrict
                                  the search to this Device Tree branch.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           Not found.
  @retval EFI_UNSUPPORTED         Unsupported.
**/
EFI_STATUS
EFIAPI
RiscVHartInfoDispatcher (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        INT32                      FdtBranch
  )
{
  EFI_STATUS  Status;
  INT32       CpusNode;
  VOID        *Fdt;

  if (FdtParserHandle == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Fdt = FdtParserHandle->Fdt;

  // The "cpus" node resides at the root of the DT. Fetch it.
  CpusNode = FdtPathOffset (Fdt, "/cpus");
  if (CpusNode < 0) {
    return EFI_NOT_FOUND;
  }

  Status = RiscVCmoInfoParser (FdtParserHandle, CpusNode);
  if (EFI_ERROR (Status)) {
    // EFI_NOT_FOUND is not tolerated at this point.
    ASSERT (0);
    return Status;
  }

  Status = RiscVIsaStringInfoParser (FdtParserHandle, CpusNode);
  if (EFI_ERROR (Status)) {
    // EFI_NOT_FOUND is not tolerated at this point.
    ASSERT (0);
    return Status;
  }

  Status = RiscVTimerInfoParser (FdtParserHandle, CpusNode);
  if (EFI_ERROR (Status)) {
    // EFI_NOT_FOUND is not tolerated at this point.
    ASSERT (0);
    return Status;
  }

  return EFI_SUCCESS;
}
