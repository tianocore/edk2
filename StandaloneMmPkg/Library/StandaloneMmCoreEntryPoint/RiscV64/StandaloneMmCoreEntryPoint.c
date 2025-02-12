/** @file
  Entry point to the Standalone MM Foundation on RISCV platforms

Copyright (c) 2025, Rivos Inc<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiMm.h>

#include <StandaloneMmCpu.h>
#include <StandaloneMmCoreEntryPoint.h>

/**
  Retrieve and print boot information provided by privileged firmware.

  This function extracts and logs the RISC-V SMM boot info structure passed by
  the secure firmware. It prints memory layout, communication buffer, and CPU topology.

  @param[in] PayloadInfoAddress  Address of the EFI_RISCV_SMM_PAYLOAD_INFO structure.

  @retval EFI_RISCV_SMM_PAYLOAD_INFO*   Pointer to parsed boot information if valid.
  @retval NULL                          If the input is NULL.
**/
EFI_RISCV_SMM_PAYLOAD_INFO *
GetAndPrintBootinformation (
  IN VOID  *PayloadInfoAddress
  )
{
  EFI_RISCV_SMM_PAYLOAD_INFO  *BootInfo;
  EFI_RISCV_SMM_CPU_INFO      *CpuInfo;
  UINTN                        Index;

  BootInfo = (EFI_RISCV_SMM_PAYLOAD_INFO *)PayloadInfoAddress;

  if (BootInfo == NULL) {
    DEBUG ((DEBUG_ERROR, "GetAndPrintBootinformation: PayloadInfoAddress is NULL\n"));
    return NULL;
  }

  DEBUG ((DEBUG_INFO, "=== SMM Boot Info ===\n"));
  DEBUG ((DEBUG_INFO, "  NumMmMemRegions : 0x%x\n", BootInfo->NumMmMemRegions));
  DEBUG ((DEBUG_INFO, "  MmMemBase       : 0x%lx\n", BootInfo->MmMemBase));
  DEBUG ((DEBUG_INFO, "  MmMemLimit      : 0x%lx\n", BootInfo->MmMemLimit));
  DEBUG ((DEBUG_INFO, "  MmImageBase     : 0x%lx\n", BootInfo->MmImageBase));
  DEBUG ((DEBUG_INFO, "  MmStackBase     : 0x%lx\n", BootInfo->MmStackBase));
  DEBUG ((DEBUG_INFO, "  MmHeapBase      : 0x%lx\n", BootInfo->MmHeapBase));
  DEBUG ((DEBUG_INFO, "  MmNsCommBufBase : 0x%lx\n", BootInfo->MmNsCommBufBase));
  DEBUG ((DEBUG_INFO, "  MmImageSize     : 0x%x\n", BootInfo->MmImageSize));
  DEBUG ((DEBUG_INFO, "  MmPcpuStackSize : 0x%x\n", BootInfo->MmPcpuStackSize));
  DEBUG ((DEBUG_INFO, "  MmHeapSize      : 0x%x\n", BootInfo->MmHeapSize));
  DEBUG ((DEBUG_INFO, "  MmNsCommBufSize : 0x%x\n", BootInfo->MmNsCommBufSize));
  DEBUG ((DEBUG_INFO, "  NumCpus         : 0x%x\n", BootInfo->NumCpus));

  CpuInfo = (EFI_RISCV_SMM_CPU_INFO *)&BootInfo->CpuInfo;

  for (Index = 0; Index < BootInfo->NumCpus; Index++) {
    DEBUG ((DEBUG_INFO, "  CPU[%u] ProcessorId: 0x%lx, Package: %u, Core: %u\n",
            Index,
            CpuInfo[Index].ProcessorId,
            CpuInfo[Index].Package,
            CpuInfo[Index].Core));
  }

  return BootInfo;
}
/**
  The entry point of Standalone MM Foundation.

  @param  [in]  CpuId             The Id assigned to this running CPU
  @param  [in]  PayloadInfoAddress   The address of boot info

**/
VOID
EFIAPI
CModuleEntryPoint (
  IN UINT64  CpuId,
  IN VOID    *PayloadInfoAddress
  )
{
  EFI_RISCV_SMM_PAYLOAD_INFO  *BootInfo;
  VOID                        *HobStart;

  BootInfo = GetAndPrintBootinformation (PayloadInfoAddress);
  if (BootInfo == NULL) {
    return;
  }

  ASSERT (0);
}
