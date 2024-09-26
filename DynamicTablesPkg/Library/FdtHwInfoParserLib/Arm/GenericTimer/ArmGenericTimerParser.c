/** @file
  Arm generic timer parser.

  Copyright (c) 2021, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - linux/Documentation/devicetree/bindings/timer/arm,arch_timer.yaml
**/

#include <Library/BaseMemoryLib.h>
#include "FdtHwInfoParser.h"
#include "CmObjectDescUtility.h"
#include "Arm/GenericTimer/ArmGenericTimerParser.h"
#include "Arm/Gic/ArmGicDispatcher.h"

/** List of "compatible" property values for timer nodes.

  Other "compatible" values are not supported by this module.
*/
STATIC CONST COMPATIBILITY_STR  TimerCompatibleStr[] = {
  { "arm,armv7-timer" },
  { "arm,armv8-timer" }
};

/** Timer compatiblity information.
*/
STATIC CONST COMPATIBILITY_INFO  TimerCompatibleInfo = {
  ARRAY_SIZE (TimerCompatibleStr),
  TimerCompatibleStr
};

/** Parse a timer node.

  @param [in]  Fdt                Pointer to a Flattened Device Tree (Fdt).
  @param [in]  TimerNode          Offset of a timer node.
  @param [in]  GenericTimerInfo   The CM_ARM_BOOT_ARCH_INFO to populate.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
TimerNodeParser (
  IN  CONST VOID                       *Fdt,
  IN        INT32                      TimerNode,
  IN        CM_ARM_GENERIC_TIMER_INFO  *GenericTimerInfo
  )
{
  EFI_STATUS    Status;
  CONST UINT32  *Data;
  INT32         IntcNode;
  UINT32        GicVersion;
  INT32         DataSize;
  INT32         IntCells;
  BOOLEAN       AlwaysOnTimer;

  if ((Fdt == NULL) ||
      (GenericTimerInfo == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Data = fdt_getprop (Fdt, TimerNode, "always-on", &DataSize);
  if ((Data == NULL) || (DataSize < 0)) {
    AlwaysOnTimer = FALSE;
  } else {
    AlwaysOnTimer = TRUE;
  }

  // Get the associated interrupt-controller.
  Status = FdtGetIntcParentNode (Fdt, TimerNode, &IntcNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Check that the interrupt-controller node is a Gic.
  Status = GetGicVersion (Fdt, IntcNode, &GicVersion);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Get the number of cells used to encode an interrupt.
  Status = FdtGetInterruptCellsInfo (Fdt, IntcNode, &IntCells);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    if (Status == EFI_NOT_FOUND) {
      // Should have found the node.
      Status = EFI_ABORTED;
    }

    return Status;
  }

  Data = fdt_getprop (Fdt, TimerNode, "interrupts", &DataSize);
  if ((Data == NULL) ||
      (DataSize != (FdtMaxTimerItem * IntCells * sizeof (UINT32))))
  {
    // If error or not FdtMaxTimerItem interrupts.
    ASSERT (0);
    return EFI_ABORTED;
  }

  GenericTimerInfo->SecurePL1TimerGSIV =
    FdtGetInterruptId (&Data[FdtSecureTimerIrq * IntCells]);
  GenericTimerInfo->SecurePL1TimerFlags =
    FdtGetInterruptFlags (&Data[FdtSecureTimerIrq * IntCells]);
  GenericTimerInfo->NonSecurePL1TimerGSIV =
    FdtGetInterruptId (&Data[FdtNonSecureTimerIrq * IntCells]);
  GenericTimerInfo->NonSecurePL1TimerFlags =
    FdtGetInterruptFlags (&Data[FdtNonSecureTimerIrq * IntCells]);
  GenericTimerInfo->VirtualTimerGSIV =
    FdtGetInterruptId (&Data[FdtVirtualTimerIrq * IntCells]);
  GenericTimerInfo->VirtualTimerFlags =
    FdtGetInterruptFlags (&Data[FdtVirtualTimerIrq * IntCells]);
  GenericTimerInfo->NonSecurePL2TimerGSIV =
    FdtGetInterruptId (&Data[FdtHypervisorTimerIrq * IntCells]);
  GenericTimerInfo->NonSecurePL2TimerFlags =
    FdtGetInterruptFlags (&Data[FdtHypervisorTimerIrq * IntCells]);

  if (AlwaysOnTimer) {
    GenericTimerInfo->SecurePL1TimerFlags    |= BIT2;
    GenericTimerInfo->NonSecurePL1TimerFlags |= BIT2;
    GenericTimerInfo->VirtualTimerFlags      |= BIT2;
    GenericTimerInfo->NonSecurePL2TimerFlags |= BIT2;
  }

  // Setup default values
  // The CntControlBase & CntReadBase Physical Address are optional if
  // the system implements EL3 (Security Extensions). So, initialise
  // these to their default value.
  GenericTimerInfo->CounterControlBaseAddress = 0xFFFFFFFFFFFFFFFF;
  GenericTimerInfo->CounterReadBaseAddress    = 0xFFFFFFFFFFFFFFFF;

  // For systems not implementing ARMv8.1 VHE, this field is 0.
  GenericTimerInfo->VirtualPL2TimerGSIV  = 0;
  GenericTimerInfo->VirtualPL2TimerFlags = 0;

  return EFI_SUCCESS;
}

/** CM_ARM_GENERIC_TIMER_INFO parser function.

  The following structure is populated:
  typedef struct CmArmGenericTimerInfo {
    UINT64  CounterControlBaseAddress;        // {default}
    UINT64  CounterReadBaseAddress;           // {default}
    UINT32  SecurePL1TimerGSIV;               // {Populated}
    UINT32  SecurePL1TimerFlags;              // {Populated}
    UINT32  NonSecurePL1TimerGSIV;            // {Populated}
    UINT32  NonSecurePL1TimerFlags;           // {Populated}
    UINT32  VirtualTimerGSIV;                 // {Populated}
    UINT32  VirtualTimerFlags;                // {Populated}
    UINT32  NonSecurePL2TimerGSIV;            // {Populated}
    UINT32  NonSecurePL2TimerFlags;           // {Populated}
    UINT32  VirtualPL2TimerGSIV;              // {default}
    UINT32  VirtualPL2TimerFlags;             // {default}
  } CM_ARM_GENERIC_TIMER_INFO;

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
ArmGenericTimerInfoParser (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        INT32                      FdtBranch
  )
{
  EFI_STATUS                 Status;
  UINT32                     Index;
  INT32                      TimerNode;
  UINT32                     TimerNodeCount;
  CM_ARM_GENERIC_TIMER_INFO  GenericTimerInfo;
  VOID                       *Fdt;

  if (FdtParserHandle == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Fdt    = FdtParserHandle->Fdt;
  Status = FdtCountCompatNodeInBranch (
             Fdt,
             FdtBranch,
             &TimerCompatibleInfo,
             &TimerNodeCount
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  if (TimerNodeCount == 0) {
    return EFI_NOT_FOUND;
  }

  // Parse each timer node in the branch.
  TimerNode = FdtBranch;
  for (Index = 0; Index < TimerNodeCount; Index++) {
    ZeroMem (&GenericTimerInfo, sizeof (CM_ARM_GENERIC_TIMER_INFO));

    Status = FdtGetNextCompatNodeInBranch (
               Fdt,
               FdtBranch,
               &TimerCompatibleInfo,
               &TimerNode
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      if (Status == EFI_NOT_FOUND) {
        // Should have found the node.
        Status = EFI_ABORTED;
      }

      return Status;
    }

    Status = TimerNodeParser (Fdt, TimerNode, &GenericTimerInfo);
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }

    // Add the CmObj to the Configuration Manager.
    Status = AddSingleCmObj (
               FdtParserHandle,
               CREATE_CM_ARM_OBJECT_ID (EArmObjGenericTimerInfo),
               &GenericTimerInfo,
               sizeof (CM_ARM_GENERIC_TIMER_INFO),
               NULL
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }
  } // for

  return Status;
}
