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
#include "HartInfo/RiscVTimerDispatcher.h"

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
  INT32                Len;
  INT32                TimerNode;
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
    DEBUG ((DEBUG_ERROR, "Failed to parse cpu node:timebase-frequency\n"));
    return EFI_NOT_FOUND;
  }

  TimerNode = -1;
  Status    = FdtGetNextCompatNodeInBranch (Fdt, -1, &TimerCompatibleInfo, &TimerNode);
  if (!EFI_ERROR (Status)) {
    Prop = FdtGetProp (Fdt, TimerNode, "riscv,timer-cannot-wake-cpu", &Len);
    if (Prop) {
      TimerInfo.Flags = EFI_ACPI_6_6_RHCT_FLAG_TIMER_CANNOT_WAKEUP_CPU;
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

/** Timer dispatcher.

  A parser parses a Device Tree to populate a specific CmObj type. None,
  one or many CmObj can be created by the parser.
  The created CmObj are then handed to the parser's caller through the
  HW_INFO_ADD_OBJECT interface.

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
RiscVTimerDispatcher (
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

  Status = RiscVTimerInfoParser (FdtParserHandle, CpusNode);
  if (EFI_ERROR (Status)) {
    // EFI_NOT_FOUND is not tolerated at this point.
    ASSERT (0);
    return Status;
  }

  return EFI_SUCCESS;
}
