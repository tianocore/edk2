/** @file
  RISC-V IOMMU command and fault queue handling.

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

  //
  // At the minimum it can be (a page), the queue is quite large when considering the fencing.
  //
  QueueHead.Uint32 = MmioRead32 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_CQH);
  QueueTail.Uint32 = MmioRead32 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_CQT);
  if (QueueTail.Bits.index == (QueueHead.Bits.index - 1)) {
    ASSERT (FALSE);
    return EFI_DEVICE_ERROR;
  }

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
  IN RISCV_IOMMU_CONTEXT   *IoMmuContext,
  IN EFI_PHYSICAL_ADDRESS  DeviceAddress
  )
{
  RISCV_IOMMU_IOTINVAL  Command;
  EFI_STATUS            Status;

  ZeroMem ((VOID *)&Command, sizeof (RISCV_IOMMU_IOTINVAL));

  Command.opcode = OP_RISCV_IOMMU_IOTINVAL;
  Command.func3  = FUNC_RISCV_IOMMU_IOTINVAL_VMA;

  //
  // As a non-virtualised firmware, neither process contexts nor second-level translation are in use.
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
  Invalidate a downstream's DevATC.
  Call this after updating device page tables.

**/
EFI_STATUS
IoMmuInvalidateDownstreamDevAtc (
  IN RISCV_IOMMU_CONTEXT    *IoMmuContext,
  IN RISCV_IOMMU_DEVICE_ID  *IoMmuDeviceId,
  IN EFI_PHYSICAL_ADDRESS   DeviceAddress
  )
{
  RISCV_IOMMU_ATS_INVAL  Command;
  EFI_STATUS             Status;

  ZeroMem ((VOID *)&Command, sizeof (RISCV_IOMMU_ATS_INVAL));

  Command.opcode = OP_RISCV_IOMMU_ATS;
  Command.func3  = FUNC_RISCV_IOMMU_ATS_INVAL;

  //
  // As a non-virtualised firmware, process contexts are not in use, so there is no PASID.
  //
  Command.PV = 0;

  //
  // If the needed width is sufficiently high, since this is a PCI device,
  // it needs the segment number passed across too.
  //
  if (NeededIoMmuDeviceIdWidth () > 16) {
    Command.DSV  = 1;
    Command.DSEG = IoMmuDeviceId->PciBdf.Segment;
  }

  Command.RID = IoMmuDeviceId->Uint32 & 0xFFFF;

  // TODO: G and S bits.
  Command.PAYLOAD.Bits.Address = DeviceAddress;

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
  RISCV_IOMMU_QUEUE_POINTER               QueueIndex;
  UINTN                                   FenceCommandIndex;
  BOOLEAN                                 Timeout;
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

  QueueIndex.Uint32 = MmioRead32 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_CQT);
  FenceCommandIndex = QueueIndex.Bits.index;

  Timeout = FALSE;
  Status  = IoMmuQueueCommand (IoMmuContext, (VOID *)&Command);
  ASSERT_EFI_ERROR (Status);

  //
  // Poll for fence completion.
  //
  QueueIndex.Uint32 = MmioRead32 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_CQH);
  CompletionIndex   = QueueIndex.Bits.index;
  while ((CompletionIndex < FenceCommandIndex) && !Timeout) {
    QueueIndex.Uint32 = MmioRead32 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_CQH);
    CompletionIndex   = QueueIndex.Bits.index;

    CqCsr.Uint32 = MmioRead32 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_CQCSR);
    Timeout      = CqCsr.Bits.cmd_to;
  }

  if (Timeout) {
    DEBUG ((DEBUG_WARN, "IOMMU command execution timed out!\n"));
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
  Handle the queue CSRs at the start of probing the fault queue.

**/
STATIC
VOID
PreFaultQueueCsrHandling (
  IN RISCV_IOMMU_CONTEXT  *IoMmuContext
  )
{
  RISCV_IOMMU_SOFTWARE_REQUEST_QUEUE_CSR  CqCsr;
  RISCV_IOMMU_QUEUE_BASE                  CommandQueueBaseReg;
  VOID                                    *CommandQueueBuffer;
  RISCV_IOMMU_QUEUE_POINTER               CommandQueueTail;
  RISCV_IOMMU_IOFENCE                     *CommandRecord;

  //
  // We do not expect to encounter illegal commands.
  // Therefore, if the IOMMU reports one, print it, handle it, and halt.
  //
  CqCsr.Uint32 = MmioRead32 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_CQCSR);
  if (CqCsr.Bits.cmd_ill) {
    CommandQueueBaseReg.Uint64 = MmioRead64 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_CQB);

    CommandQueueBuffer = (VOID *)((UINT64)CommandQueueBaseReg.Bits.PPN << RISCV_MMU_PAGE_SHIFT);
    ASSERT (CommandQueueBuffer != NULL);

    CommandQueueTail.Uint32 = MmioRead32 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_CQT);

    CommandRecord = (VOID *)((UINT8 *)CommandQueueBuffer + (CommandQueueTail.Bits.index * COMMAND_QUEUE_ENTRY_SIZE));
    DEBUG ((DEBUG_ERROR, "Command with (opcode 0x%x, func3 0x%x) was rejected as illegal\n", CommandRecord->opcode, CommandRecord->func3));

    CqCsr.Bits.cmd_ill = 0;
    MmioWrite32 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_CQCSR, CqCsr.Uint32);
    ASSERT (FALSE);
  }
}

/**
  Dump some debug information about the fault.

**/
STATIC
VOID
DumpFaultInformation (
  IN RISCV_IOMMU_FAULT_RECORD  *FaultRecord
  )
{
  switch (FaultRecord->CAUSE) {
    case RISCV_IOMMU_FAULT_CAUSE_INSTR_ACCESS_FAULT:
    case RISCV_IOMMU_FAULT_CAUSE_READ_ACCESS_FAULT:
    case RISCV_IOMMU_FAULT_CAUSE_WRITE_ACCESS_FAULT:
      DEBUG ((DEBUG_INFO, "Transaction triggered an access fault, please report this!\n"));
      break;
    case RISCV_IOMMU_FAULT_CAUSE_READ_ADDR_MISALIGNED:
    case RISCV_IOMMU_FAULT_CAUSE_WRITE_ADDR_MISALIGNED:
      DEBUG ((DEBUG_INFO, "Address is misaligned. Please check your device's driver.\n"));
      break;
    case RISCV_IOMMU_FAULT_CAUSE_DMA_DISABLED:
      DEBUG ((DEBUG_INFO, "DMA is disabled. Please report this!\n"));
      break;
    case RISCV_IOMMU_FAULT_CAUSE_DDT_LOAD_FAULT:
    case RISCV_IOMMU_FAULT_CAUSE_DDT_INVALID:
    case RISCV_IOMMU_FAULT_CAUSE_DDT_MISCONFIGURED:
    case RISCV_IOMMU_FAULT_CAUSE_DDT_CORRUPTED:
      DEBUG ((DEBUG_INFO, "Device Directory Table cannot be read/parsed, please report this!\n"));
      break;
    case RISCV_IOMMU_FAULT_CAUSE_TTYP_DISALLOWED:
      DEBUG ((DEBUG_INFO, "Transaction type disallowed, please report this!\n"));
      break;
    case RISCV_IOMMU_FAULT_CAUSE_INTERNAL_PATH_ERROR:
      DEBUG ((DEBUG_INFO, "Internal IOMMU error, please report this!\n"));
      break;
    case RISCV_IOMMU_FAULT_CAUSE_PT_CORRUPTED:
      DEBUG ((DEBUG_INFO, "Device page table corrupted, please report this!\n"));
      break;
    case RISCV_IOMMU_FAULT_CAUSE_MSI_LOAD_FAULT:
    case RISCV_IOMMU_FAULT_CAUSE_MSI_INVALID:
    case RISCV_IOMMU_FAULT_CAUSE_MSI_MISCONFIGURED:
    case RISCV_IOMMU_FAULT_CAUSE_MRIF_ACCESS_FAULT:
    case RISCV_IOMMU_FAULT_CAUSE_MSI_PT_CORRUPTED:
    case RISCV_IOMMU_FAULT_CAUSE_MRIF_CORRUPTED:
    case RISCV_IOMMU_FAULT_CAUSE_MSI_WRITE_FAULT:
      DEBUG ((DEBUG_WARN, "(unhandled; MSIs weren't configured)\n"));
      break;
    case RISCV_IOMMU_FAULT_CAUSE_PDT_LOAD_FAULT:
    case RISCV_IOMMU_FAULT_CAUSE_PDT_INVALID:
    case RISCV_IOMMU_FAULT_CAUSE_PDT_MISCONFIGURED:
    case RISCV_IOMMU_FAULT_CAUSE_PDT_CORRUPTED:
      DEBUG ((DEBUG_WARN, "(unhandled; process contexts aren't needed by firmware)\n"));
      break;
    case RISCV_IOMMU_FAULT_CAUSE_INSTR_PAGE_FAULT_1:
    case RISCV_IOMMU_FAULT_CAUSE_READ_PAGE_FAULT_1:
    case RISCV_IOMMU_FAULT_CAUSE_WRITE_PAGE_FAULT_1:
    case RISCV_IOMMU_FAULT_CAUSE_INSTR_PAGE_FAULT_2:
    case RISCV_IOMMU_FAULT_CAUSE_READ_PAGE_FAULT_2:
    case RISCV_IOMMU_FAULT_CAUSE_WRITE_PAGE_FAULT_2:
      DEBUG ((DEBUG_WARN, "(unhandled; page tables aren't needed for ATS)\n"));
      break;
    default:
      DEBUG ((DEBUG_WARN, "(unknown)\n"));
      break;
  }
}

/**
  Handle the queue CSRs at the end of probing the fault queue.

**/
STATIC
VOID
PostFaultQueueCsrHandling (
  IN RISCV_IOMMU_CONTEXT  *IoMmuContext
  )
{
  RISCV_IOMMU_HARDWARE_REQUEST_QUEUE_CSR  FqCsr;

  //
  // Implement end-of-queue error handling.
  //
  FqCsr.Uint32 = MmioRead32 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_FQCSR);
  if (FqCsr.Bits.qof) {
    DEBUG ((DEBUG_WARN, "Fault queue overflowed!\n"));
    FqCsr.Bits.qof = 0;
    MmioWrite32 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_FQCSR, FqCsr.Uint32);
  }
}

/**
  Probe fault queue for errors.

**/
EFI_STATUS
ProbeFaultQueueForErrors (
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
  // - Handle cqmf/fqmf and cmd_to.
  // - Handle fault causes?
  //
  DEBUG ((DEBUG_WARN, "Faults (or events) have been reported!\n"));

  PreFaultQueueCsrHandling (IoMmuContext);

  Index = 0;
  while (QueueHead.Bits.index != QueueTail.Bits.index) {
    DEBUG ((DEBUG_INFO, "\nFault record %d:\n", Index++));
    FaultRecord = (VOID *)((UINT8 *)QueueBuffer + (QueueHead.Bits.index * FAULT_QUEUE_ENTRY_SIZE));

    DumpFaultInformation (FaultRecord);

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

  PostFaultQueueCsrHandling (IoMmuContext);

  return EFI_DEVICE_ERROR;
}
