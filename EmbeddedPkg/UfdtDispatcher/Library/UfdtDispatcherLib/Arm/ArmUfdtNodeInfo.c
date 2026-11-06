/** @file
  Arm-specific unflattened device tree utilities.

  Copyright (c) 2026, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include "../UfdtDispatcherLibInternal.h"

// Layout and encodings of a three-cell GIC interrupt specifier.
#define FDT_GIC_INTERRUPT_CELLS  3
#define FDT_GIC_TYPE_CELL        0
#define FDT_GIC_NUMBER_CELL      1
#define FDT_GIC_FLAGS_CELL       2

/** Resolve a node's interrupt and decode its architectural GIC interrupt ID.

  Follow interrupt parents and interrupt-map entries internally. Require a
  three-cell, level-sensitive GIC SPI/PPI specifier.
  Extended SPI and PPI ranges are unsupported. The controller's compatible
  property is not checked.

  @param[in]  Node       Node whose interrupt is requested.
  @param[in]  Index      Zero-based interrupt specifier index.
  @param[out] Interrupt  Architectural interrupt ID, valid on success.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_NOT_FOUND          Not Found.
  @retval EFI_COMPROMISED_DATA   Invalid DTB or GIC interrupt encoding.
  @retval EFI_UNSUPPORTED        Unsupported GIC cell count or trigger mode.
**/
EFI_STATUS
EFIAPI
UfdtGetInterruptId (
  IN CONST UFDT_DEVICE_NODE  *Node,
  IN UINT32                  Index,
  OUT UINT32                 *Interrupt
  )
{
  EFI_STATUS    Status;
  CONST UINT32  *Data;
  INT32         Cells;
  UINT32        IrqType;
  UINT32        IrqId;
  UINT32        Flags;

  if (Interrupt == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = UfdtResolveInterrupt (Node, Index, &Data, &Cells);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Cells != FDT_GIC_INTERRUPT_CELLS) {
    return EFI_UNSUPPORTED;
  }

  IrqType = SwapBytes32 (ReadUnaligned32 (Data + FDT_GIC_TYPE_CELL));
  IrqId   = SwapBytes32 (ReadUnaligned32 (Data + FDT_GIC_NUMBER_CELL));

  switch (IrqType) {
    case FDT_GIC_SPI:
      if (IrqId >= FDT_GIC_SPI_COUNT) {
        return EFI_COMPROMISED_DATA;
      }

      IrqId += FDT_GIC_SPI_BASE;
      break;

    case FDT_GIC_PPI:
      if (IrqId >= FDT_GIC_PPI_COUNT) {
        return EFI_COMPROMISED_DATA;
      }

      IrqId += FDT_GIC_PPI_BASE;
      break;

    default:
      return EFI_COMPROMISED_DATA;
  }

  // The timer and watchdog consumers require level-sensitive interrupts.
  Flags = SwapBytes32 (ReadUnaligned32 (Data + FDT_GIC_FLAGS_CELL)) & 0xF;
  if ((Flags != BIT2) && (Flags != BIT3)) {
    return EFI_UNSUPPORTED;
  }

  *Interrupt = IrqId;
  return EFI_SUCCESS;
}
