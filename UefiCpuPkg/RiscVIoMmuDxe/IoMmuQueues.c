/** @file
  RISC-V IOMMU driver.

  Copyright (c) 2025, 9elements GmbH. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include "RiscVIoMmu.h"

/**
  IOMMU queue-command worker.

**/
STATIC
EFI_STATUS
IoMmuQueueCommand (
  IN RISCV_IOMMU_CONTEXT  *IoMmuContext,
  IN VOID                 *CommandBuffer
  )
{
  RISCV_IOMMU_QUEUE_BASE     QueueBaseReg;
  VOID                       *QueueBuffer;
  RISCV_IOMMU_QUEUE_POINTER  QueueHead;
  RISCV_IOMMU_QUEUE_POINTER  QueueTail;

  QueueBaseReg.Uint64 = MmioRead64 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_CQB);

  QueueBuffer = (VOID *)((UINT64)QueueBaseReg.Bits.PPN << RISCV_MMU_PAGE_SHIFT);
  ASSERT (QueueBuffer != NULL);

  // TODO: Improve this handling later? The queue is fairly large.
  QueueHead.Uint32 = MmioRead32 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_CQH);
  QueueTail.Uint32 = MmioRead32 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_CQT);
  if (QueueTail.Bits.index == (QueueHead.Bits.index - 1)) {
    ASSERT (FALSE);
    return EFI_DEVICE_ERROR;
  }

  // TODO: If we type commands, change this.
  CopyMem ((UINT8 *)QueueBuffer + (QueueTail.Bits.index * COMMAND_QUEUE_ENTRY_SIZE), CommandBuffer, COMMAND_QUEUE_ENTRY_SIZE);
  MemoryFence ();

  if (QueueTail.Bits.index == (QUEUE_NUMBER_OF_ENTRIES - 1)) {
    MmioWrite32 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_CQT, 0);
  } else {
    MmioWrite32 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_CQT, QueueTail.Bits.index + 1);
  }

  return EFI_SUCCESS;
}

/**
  Invalidate the IOMMU's page table cache.
  Call this after updating device page tables.

**/
EFI_STATUS
IoMmuInvalidatePageTableCache (
  IN RISCV_IOMMU_CONTEXT  *IoMmuContext,
  IN EFI_PHYSICAL_ADDRESS  DeviceAddress
  )
{
  RISCV_IOMMU_IOTINVAL  Command;
  EFI_STATUS            Status;

  ZeroMem ((VOID *)&Command, sizeof (RISCV_IOMMU_IOTINVAL));

  Command.opcode = OP_RISCV_IOMMU_IOTINVAL;
  Command.func3  = FUNC_RISCV_IOMMU_IOTINVAL_VMA;

  //
  // As a non-virtualised OS, neither process contexts nor second-level translation are in use.
  //
  Command.PSCV = 0;
  Command.GV   = 0;

  //
  // As a likely performance optimisation, only command invalidation of these entries.
  //
  Command.AV   = 1;
  Command.ADDR = DeviceAddress;

  // TODO: NL and S bits.
  Status = IoMmuQueueCommand (IoMmuContext, (VOID *)&Command);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  A fence operation for the IOMMU's command queue.
  Call this after completing one full operation.

**/
EFI_STATUS
IoMmuCommandQueueFence (
  IN RISCV_IOMMU_CONTEXT  *IoMmuContext
  )
{
  RISCV_IOMMU_IOFENCE                     Command;
  BOOLEAN                                 Timeout;
  RISCV_IOMMU_QUEUE_POINTER               QueueIndex;
  UINTN                                   FenceCommandIndex;
  EFI_STATUS                              Status;
  UINTN                                   CompletionIndex;
  RISCV_IOMMU_SOFTWARE_REQUEST_QUEUE_CSR  CqCsr;

  ZeroMem ((VOID *)&Command, sizeof (RISCV_IOMMU_IOFENCE));

  Command.opcode = OP_RISCV_IOMMU_IOFENCE;
  Command.func3  = FUNC_RISCV_IOMMU_IOFENCE_C;

  //
  // Request memory fencing for the sake of the caller.
  //
  Command.PR = 1;
  Command.PW = 1;

  //
  // Use polling, not interrupts.
  //
  Command.AV  = 0;
  Command.WSI = 0;

  Timeout = FALSE;
  QueueIndex.Uint32 = MmioRead32 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_CQT);
  FenceCommandIndex = QueueIndex.Bits.index;

  Status = IoMmuQueueCommand (IoMmuContext, (VOID *)&Command);
  ASSERT_EFI_ERROR (Status);

  //
  // Poll for fence completion.
  //
  QueueIndex.Uint32 = MmioRead32 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_CQH);
  CompletionIndex = QueueIndex.Bits.index;
  while ((CompletionIndex < FenceCommandIndex) && !Timeout) {
    QueueIndex.Uint32 = MmioRead32 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_CQH);
    CompletionIndex = QueueIndex.Bits.index;

    CqCsr.Uint32 = MmioRead32 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_CQCSR);
    Timeout = CqCsr.Bits.cmd_to;
  }

  if (Timeout) {
    DEBUG ((DEBUG_ERROR, "IOMMU command execution timed out!\n"));
    Status = EFI_TIMEOUT;
  }

  return Status;
}

/**
  Invalidate the IOMMU's device directory cache.
  Call this after updating any level of the DDT.

**/
EFI_STATUS
IoMmuInvalidateDeviceDirectoryCache (
  IN RISCV_IOMMU_CONTEXT    *IoMmuContext,
  IN RISCV_IOMMU_DEVICE_ID  *IoMmuDeviceId
  )
{
  RISCV_IOMMU_IODIR  Command;
  EFI_STATUS         Status;

  ZeroMem ((VOID *)&Command, sizeof (RISCV_IOMMU_IODIR));

  Command.opcode = OP_RISCV_IOMMU_IODIR;
  Command.func3  = FUNC_RISCV_IOMMU_IODIR_INVAL_DDT;

  //
  // As a likely performance optimisation, only command invalidation of these entries.
  //
  Command.DV  = 1;
  Command.DID = IoMmuDeviceId->Uint32;

  Status = IoMmuQueueCommand (IoMmuContext, (VOID *)&Command);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Probe fault queue for errors.

**/
STATIC
EFI_STATUS
ProbeFaultQueue (
  IN RISCV_IOMMU_CONTEXT  *IoMmuContext
  )
{
  RISCV_IOMMU_QUEUE_POINTER  QueueHead;
  RISCV_IOMMU_QUEUE_POINTER  QueueTail;
  RISCV_IOMMU_QUEUE_BASE     QueueBaseReg;
  VOID                       *QueueBuffer;
  RISCV_IOMMU_FAULT_RECORD   *FaultRecord;
  UINTN                      Index;

  QueueBaseReg.Uint64 = MmioRead64 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_FQB);

  QueueBuffer = (VOID *)((UINT64)QueueBaseReg.Bits.PPN << RISCV_MMU_PAGE_SHIFT);
  ASSERT (QueueBuffer != NULL);

  QueueHead.Uint32 = MmioRead32 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_FQH);
  QueueTail.Uint32 = MmioRead32 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_FQT);
  if (QueueHead.Bits.index == QueueTail.Bits.index) {
    return EFI_SUCCESS;
  }

  //
  // TODOs:
  // - Handle cqmf, cmd_to and cmd_ill. fence_w_ip is not relevant.
  // - Handle fqmf/pqmf and fqof/pqof.
  // - Others.
  //
  DEBUG ((DEBUG_WARN, "Faults (or events) have been reported!\n"));

  Index = 0;
  while (QueueHead.Bits.index != QueueTail.Bits.index) {
    DEBUG ((DEBUG_INFO, "Fault record %d:\n", Index++));
    FaultRecord = (VOID *)((UINT8 *)QueueBuffer + (QueueHead.Bits.index * FAULT_QUEUE_ENTRY_SIZE));

    DEBUG ((DEBUG_INFO, "  Cause: %d\n", FaultRecord->CAUSE));
    DEBUG ((DEBUG_INFO, "  PID: 0x%x\n", FaultRecord->PID));
    DEBUG ((DEBUG_INFO, "  PV: %d\n", FaultRecord->PV));
    DEBUG ((DEBUG_INFO, "  PRIV: %d\n", FaultRecord->PRIV));
    DEBUG ((DEBUG_INFO, "  Transaction Type: %d\n", FaultRecord->TTYP));
    DEBUG ((DEBUG_INFO, "  DID: 0x%x\n", FaultRecord->DID));
    DEBUG ((DEBUG_INFO, "  Custom: 0x%x\n", FaultRecord->Custom));
    DEBUG ((DEBUG_INFO, "  Reserved: 0x%x\n", FaultRecord->Reserved));
    DEBUG ((DEBUG_INFO, "  iotval: 0x%x\n", FaultRecord->iotval));
    DEBUG ((DEBUG_INFO, "  iotval2: 0x%x\n", FaultRecord->iotval2));

    if (QueueHead.Bits.index == (QUEUE_NUMBER_OF_ENTRIES - 1)) {
      QueueHead.Bits.index = 0;
    } else {
      QueueHead.Bits.index++;
    }

    MmioWrite32 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_FQH, QueueHead.Bits.index);
  }

  return EFI_DEVICE_ERROR;
}

/**
  Probe hardware driven queues for problems.

**/
EFI_STATUS
ProbeHardwareQueuesForProblems (
  IN RISCV_IOMMU_CONTEXT  *IoMmuContext
  )
{
  EFI_STATUS                 Status;
  RISCV_IOMMU_QUEUE_POINTER  QueueHead;
  RISCV_IOMMU_QUEUE_POINTER  QueueTail;

  //
  // The fault queue is the most obvious indicator of problems.
  //
  Status = ProbeFaultQueue (IoMmuContext);

  //
  // We don't expect to handle page requests.
  //
  QueueHead.Uint32 = MmioRead32 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_PQH);
  QueueTail.Uint32 = MmioRead32 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_PQT);
  if (QueueHead.Bits.index != QueueTail.Bits.index) {
    DEBUG ((DEBUG_WARN, "A page request has been received\n"));
    Status = EFI_DEVICE_ERROR;
  }

  return Status;
}
