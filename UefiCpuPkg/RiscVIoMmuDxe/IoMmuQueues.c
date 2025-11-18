/** @file
  RISC-V IOMMU driver.

  Copyright (c) 2025, 9elements GmbH. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include "RiscVIoMmu.h"

/**
  Probe hardware driven queues for problems.

**/
EFI_STATUS
ProbeHardwareQueuesForFaults (
  IN RISCV_IOMMU_CONTEXT  *IoMmuContext
  )
{
  EFI_STATUS                 Status;
  RISCV_IOMMU_QUEUE_POINTER  QueueHead;
  RISCV_IOMMU_QUEUE_POINTER  QueueTail;

  Status = EFI_SUCCESS;

  //
  // The fault queue is the most obvious indicator of problems.
  //
  QueueHead.Uint32 = IoMmuRead32 (IoMmuContext, R_RISCV_IOMMU_FQH);
  QueueTail.Uint32 = IoMmuRead32 (IoMmuContext, R_RISCV_IOMMU_FQT);
  if (QueueHead.Bits.index != QueueTail.Bits.index) {
    DEBUG ((DEBUG_WARN, "Faults have been reported!\n"));
    Status = EFI_DEVICE_ERROR;
  }

  //
  // We don't expect to handle page requests.
  //
  QueueHead.Uint32 = IoMmuRead32 (IoMmuContext, R_RISCV_IOMMU_PQH);
  QueueTail.Uint32 = IoMmuRead32 (IoMmuContext, R_RISCV_IOMMU_PQT);
  if (QueueHead.Bits.index != QueueTail.Bits.index) {
    DEBUG ((DEBUG_WARN, "A page request has been made\n"));
    Status = EFI_DEVICE_ERROR;
  }

  return Status;
}