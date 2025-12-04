/** @file
  RISC-V IOMMU driver.

  Copyright (c) 2025, 9elements GmbH. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/FdtLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/IoLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/Cpu.h>
#include <Register/RiscV64/RiscVImpl.h>
#include "RiscVIoMmu.h"

EFI_HANDLE  mHandle = NULL;
LIST_ENTRY  mRiscVIoMmuContexts = INITIALIZE_LIST_HEAD_VARIABLE (mRiscVIoMmuContexts);

/**
  Determine if the IOMMU is in a reset state.

  @retval TRUE   The IOMMU is reset.
  @retval FALSE  The IOMMU is active.

**/
STATIC
BOOLEAN
IoMmuIsReset (
  IN RISCV_IOMMU_CONTEXT  *IoMmuContext
  )
{
  RISCV_IOMMU_SOFTWARE_REQUEST_QUEUE_CSR  SoftwareReqQueueCsr;
  RISCV_IOMMU_HARDWARE_REQUEST_QUEUE_CSR  HardwareReqQueueCsr;
  RISCV_IOMMU_DDTP                        Ddtp;
  RISCV_IOMMU_IPSR                        Ipsr;

  SoftwareReqQueueCsr.Uint32 = MmioRead32 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_CQCSR);
  if (SoftwareReqQueueCsr.Bits.qen || SoftwareReqQueueCsr.Bits.ie || SoftwareReqQueueCsr.Bits.qon ||
      SoftwareReqQueueCsr.Bits.busy) {
    return FALSE;
  }

  HardwareReqQueueCsr.Uint32 = MmioRead32 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_FQCSR);
  if (HardwareReqQueueCsr.Bits.qen || HardwareReqQueueCsr.Bits.ie || HardwareReqQueueCsr.Bits.qon ||
      HardwareReqQueueCsr.Bits.busy) {
    return FALSE;
  }

  HardwareReqQueueCsr.Uint32 = MmioRead32 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_PQCSR);
  if (HardwareReqQueueCsr.Bits.qen || HardwareReqQueueCsr.Bits.ie || HardwareReqQueueCsr.Bits.qon ||
      HardwareReqQueueCsr.Bits.busy) {
    return FALSE;
  }

  //
  // Translation request is a debug feature, and might not be present. Skipping.
  //
  Ddtp.Uint64 = MmioRead64 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_DDTP);
  if (Ddtp.Bits.busy) {
    return FALSE;
  }

  Ipsr.Uint32 = MmioRead32 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_IPSR);
  if (Ipsr.Uint32) {
    return FALSE;
  }

  // Since feature control register may be modified during initialisation, forbid BARE mode too.
  if (Ddtp.Bits.iommu_mode != V_RISCV_IOMMU_DDTP_IOMMU_MODE_OFF) {
    return FALSE;
  }

  //
  // The caches must be empty/invalid on reset. Since the command queue is disabled, we can't invalidate them though.
  // Assume it's a requirement for the hardware; we handle invalidations after basic initialisation.
  //
  return TRUE;
}

/**
  Allocate a queue.

  @param[in]  QueueType   The type of queue to be allocated.

**/
STATIC
VOID
AllocateQueue (
  IN RISCV_IOMMU_CONTEXT  *IoMmuContext,
  IN UINTN                QueueType
  )
{
  UINTN                                   QueueBaseReg;
  UINTN                                   QueueHeadTailReg;
  UINTN                                   QueueCsrReg;
  UINTN                                   EntrySize;
  UINTN                                   Log2Size;
  UINTN                                   NumberOfPages;
  VOID                                    *QueueBuffer;
  RISCV_IOMMU_QUEUE_BASE                  QueueBase;
  RISCV_IOMMU_HARDWARE_REQUEST_QUEUE_CSR  HardwareReqQueueCsr;
  RISCV_IOMMU_SOFTWARE_REQUEST_QUEUE_CSR  SoftwareReqQueueCsr;

  switch (QueueType) {
    case QUEUE_COMMAND:
      QueueBaseReg     = R_RISCV_IOMMU_CQB;
      // Unlike the others, this is one we write requests into.
      QueueHeadTailReg = R_RISCV_IOMMU_CQT;
      QueueCsrReg      = R_RISCV_IOMMU_CQCSR;
      EntrySize        = COMMAND_QUEUE_ENTRY_SIZE;
      break;
    case QUEUE_FAULT:
      QueueBaseReg     = R_RISCV_IOMMU_FQB;
      QueueHeadTailReg = R_RISCV_IOMMU_FQH;
      QueueCsrReg      = R_RISCV_IOMMU_FQCSR;
      EntrySize        = FAULT_QUEUE_ENTRY_SIZE;
      break;
    case QUEUE_PAGE_REQUEST:
      QueueBaseReg     = R_RISCV_IOMMU_PQB;
      QueueHeadTailReg = R_RISCV_IOMMU_PQH;
      QueueCsrReg      = R_RISCV_IOMMU_PQCSR;
      EntrySize        = PAGE_REQUEST_QUEUE_ENTRY_SIZE;
      break;
    default:
      ASSERT (FALSE);
      return;
  }

  //
  // The specification defines that the 'buffer size' register is `LOG2SZ-1`.
  //
  ASSERT ((QUEUE_NUMBER_OF_ENTRIES != 0) && (QUEUE_NUMBER_OF_ENTRIES % 2 == 0));
  Log2Size = HighBitSet32 (QUEUE_NUMBER_OF_ENTRIES);
  ASSERT (Log2Size - 1 < QUEUE_MAX_LOG_SIZE);

  //
  // Align the buffer to the specification's requirement.
  //
  NumberOfPages       = EFI_SIZE_TO_PAGES (QUEUE_NUMBER_OF_ENTRIES * EntrySize);
  QueueBuffer = AllocateAlignedPages (NumberOfPages, MAX (SIZE_4KB, EFI_PAGES_TO_SIZE (NumberOfPages)));
  ASSERT (QueueBuffer != NULL);

  QueueBase.Bits.PPN      = ((UINT64)QueueBuffer) >> RISCV_MMU_PAGE_SHIFT;
  QueueBase.Bits.LOG2SZ_1 = Log2Size - 1;
  MmioWrite64 (IoMmuContext->BaseAddress + QueueBaseReg, QueueBase.Uint64);
  MmioWrite32 (IoMmuContext->BaseAddress + QueueHeadTailReg, 0);
  MemoryFence ();

  //
  // Enable the queue.
  //
  if (QueueType == QUEUE_COMMAND) {
    SoftwareReqQueueCsr.Bits.qen = 1;
    IoMmuWriteAndWait32 (IoMmuContext->BaseAddress + QueueCsrReg, SoftwareReqQueueCsr.Uint32, 1 << N_RISCV_IOMMU_QUEUE_CSR_QON, TRUE);
  } else {
    HardwareReqQueueCsr.Bits.qen = 1;
    IoMmuWriteAndWait32 (IoMmuContext->BaseAddress + QueueCsrReg, HardwareReqQueueCsr.Uint32, 1 << N_RISCV_IOMMU_QUEUE_CSR_QON, TRUE);
  }
}

/**
  Program the root of a context table into the IOMMU.

  @param[in]  ContextStruct  Pointer to a context table's wrapping struct.

**/
STATIC
BOOLEAN
ProgramContextRoot (
  IN RISCV_IOMMU_CONTEXT  *IoMmuContext
  )
{
  UINT8                     DeviceIdSupportedWidth;
  RISCV_IOMMU_CAPABILITIES  Capabilities;
  BOOLEAN                   ExtendedContext;
  UINT8                     OneLevelMaxWidth;
  UINT8                     TwoLevelMaxWidth;
  UINT8                     IoMmuMode;
  RISCV_IOMMU_DDTP          Ddtp;
  VOID                      *ContextBuffer;

  //
  // Initialise the variable to silence a warning.
  //
  IoMmuMode = V_RISCV_IOMMU_DDTP_IOMMU_MODE_BARE;

  //
  // Determine the supported device_id width.
  // - FIXME: Do they mean *needed* width? Probably not (ours here it's the PCI routing ID with one bus). Consider it?
  //
  DeviceIdSupportedWidth = 16;

  //
  // Determine the format of the context struct.
  //
  Capabilities.Uint64 = MmioRead64 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_CAPABILITIES);
  ExtendedContext = Capabilities.Bits.MSI_FLAT;

  //
  // Determine the needed IOMMU mode.
  //
  OneLevelMaxWidth = ExtendedContext ? N_RISCV_IOMMU_DEVICE_ID_EXTENDED_I1 : N_RISCV_IOMMU_DEVICE_ID_BASE_I1;
  TwoLevelMaxWidth = ExtendedContext ? N_RISCV_IOMMU_DEVICE_ID_EXTENDED_I2 : N_RISCV_IOMMU_DEVICE_ID_BASE_I2;

  if (DeviceIdSupportedWidth <= OneLevelMaxWidth) {
    IoMmuMode = V_RISCV_IOMMU_DDTP_IOMMU_MODE_1LVL;
  } else if (DeviceIdSupportedWidth <= TwoLevelMaxWidth) {
    IoMmuMode = V_RISCV_IOMMU_DDTP_IOMMU_MODE_2LVL;
  } else if (DeviceIdSupportedWidth <= 24) {
    IoMmuMode = V_RISCV_IOMMU_DDTP_IOMMU_MODE_3LVL;
  }

  //
  // Attempt to set the needed mode. NOTE: Could we attempt to upgrade if this mode fails?
  //
  Ddtp.Bits.iommu_mode = IoMmuMode;
  IoMmuWriteAndWait64 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_DDTP, Ddtp.Uint64, 1 << N_RISCV_IOMMU_DDTP_BUSY, FALSE);

  Ddtp.Uint64 = MmioRead64 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_DDTP);
  if (Ddtp.Bits.iommu_mode != IoMmuMode) {
    DEBUG ((DEBUG_ERROR, "Needed IOMMU mode 0x%x is not supported!\n", IoMmuMode));
    return FALSE;
  }

  //
  // Allocate the root of the context table.
  //
  ContextBuffer = AllocatePages (1);
  ASSERT (ContextBuffer != NULL);

  ZeroMem (ContextBuffer, SIZE_4KB);

  Ddtp.Bits.PPN = ((UINT64)ContextBuffer) >> RISCV_MMU_PAGE_SHIFT;
  IoMmuWriteAndWait64 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_DDTP, Ddtp.Uint64, 1 << N_RISCV_IOMMU_DDTP_BUSY, FALSE);

  DEBUG ((
    RISCV_IOMMU_DEBUG_LEVEL,
    "%a: Configured a %d level device table at 0x%x\n",
    __func__,
    IoMmuMode - V_RISCV_IOMMU_DDTP_IOMMU_MODE_BARE,
    ContextBuffer
    ));

  return TRUE;
}

/**
  Initialise the IOMMU hardware.

  @retval  EFI_SUCCESS      The hardware is initialised.
  @retval  EFI_UNSUPPORTED  The detected IOMMU is not supported by this driver.

**/
STATIC
EFI_STATUS
InitialiseRiscVIoMmu (
  IN RISCV_IOMMU_CONTEXT  *IoMmuContext
  )
{
  RISCV_IOMMU_CAPABILITIES  Capabilities;
  BOOLEAN                   HartIsBigEndian;
  RISCV_IOMMU_FCTL          FeatureControl;
  UINTN                     HartSatpMode;

  // TODO: Handle this instead.
  ASSERT (IoMmuIsReset (IoMmuContext));

  //
  // 1. Discover the capabilities of the IOMMU, and:
  // 2. Ensure its architectural version is supported.
  //
  Capabilities.Uint64 = MmioRead64 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_CAPABILITIES);
  if (Capabilities.Bits.version != V_RISCV_IOMMU_CAPABILITIES_VERSION_1_0) {
    DEBUG ((DEBUG_ERROR, "IOMMU version 0x%x is not supported by this driver!\n", Capabilities.Bits.version));
    return EFI_UNSUPPORTED;
  }

  // It'd be preferable to read MSTATUS_SBE, but this isn't present in the SSTATUS_CSR.
  HartIsBigEndian = (RiscVGetSupervisorStatusRegister () & MSTATUS_UBE) != 0;

  //
  // 3. Read the feature control register, and:
  // 4. If changing the IOMMU's endianness is required, ensure that it's possible.
  //
  FeatureControl.Uint32 = MmioRead32 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_FCTL);
  if (HartIsBigEndian && !FeatureControl.Bits.BE && !Capabilities.Bits.END) {
    DEBUG ((DEBUG_ERROR, "HART is big-endian, which is not supported by the IOMMU!\n"));
    return EFI_UNSUPPORTED;
  }

  //
  // 5. If required, change the IOMMU's endianness.
  //
  if (HartIsBigEndian && !FeatureControl.Bits.BE && Capabilities.Bits.END) {
    FeatureControl.Bits.BE = 1;
    MmioWrite32 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_FCTL, FeatureControl.Uint32);
  }

  //
  // 6-7. Firmware is largely synchronous, so skip WSI enablement.
  //

  //
  // 8. Ensure other required capabilities (e.g. virtual-addressing modes, MSI translation, etc.) are supported.
  // - MSI translation is a virtualisation-specific feature.
  //
  HartSatpMode = (RiscVGetSupervisorAddressTranslationRegister () & SATP64_MODE) >> SATP64_MODE_SHIFT;
  if ((HartSatpMode == SATP_MODE_SV64) ||
      ((HartSatpMode == SATP_MODE_SV57) && !Capabilities.Bits.Sv57) ||
      ((HartSatpMode == SATP_MODE_SV48) && !Capabilities.Bits.Sv48) ||
      ((HartSatpMode == SATP_MODE_SV39) && !Capabilities.Bits.Sv39) ||
      ((HartSatpMode == SATP_MODE_SV32) && !Capabilities.Bits.Sv32)) {
    DEBUG ((DEBUG_ERROR, "HART virtual-addressing mode (SATP: 0x%x) is not supported by the IOMMU!\n", HartSatpMode));
    return EFI_UNSUPPORTED;
  }

  // Attempt to enable the needed group of paging modes.
  FeatureControl.Bits.GXL = HartSatpMode == SATP_MODE_SV32;
  MmioWrite32 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_FCTL, FeatureControl.Uint32);

  //
  // 9-11. Firmware is largely synchronous, so skip mapping interrupt causes to vectors.
  //

  //
  // 12-14. Program the three queues.
  //
  AllocateQueue (IoMmuContext, QUEUE_COMMAND);
  AllocateQueue (IoMmuContext, QUEUE_FAULT);
  // TODO: Unlike on an OS kernel, device-driven operation is not expected (such as to VRAM).
  if (Capabilities.Bits.ATS) {
    AllocateQueue (IoMmuContext, QUEUE_PAGE_REQUEST);
  }

  //
  // 15. Program the DDT pointer.
  //
  if (!ProgramContextRoot (IoMmuContext)) {
    DEBUG ((DEBUG_ERROR, "Failed to program the DDT root pointer!\n"));
    return EFI_UNSUPPORTED;
  }

  DEBUG ((
    DEBUG_INFO,
    "Initialised the RISC-V IOMMU %a device at 0x%lx\n",
    IoMmuContext->IoMmuIsPciDevice ? "PCI" : "system",
    IoMmuContext->BaseAddress
    ));

  return EFI_SUCCESS;
}

/**
  Initialisation worker function.

**/
EFI_STATUS
IoMmuCommonInitialise (
  IN RISCV_IOMMU_CONTEXT  *IoMmuContext
  )
{
  EFI_CPU_ARCH_PROTOCOL  *CpuArch;
  EFI_STATUS             Status;

  if (IoMmuContext->DriverState >= STATE_INITIALISED) {
    return EFI_SUCCESS;
  }

  IoMmuContext->DriverState = STATE_INITIALISED;

  DEBUG ((
    DEBUG_INFO,
    "Detected a RISC-V IOMMU %a device at 0x%lx\n",
    IoMmuContext->IoMmuIsPciDevice ? "PCI" : "system",
    IoMmuContext->BaseAddress
    ));

  //
  // Ensure the IOMMU is mapped and accessible to the HART.
  //
  Status = gBS->LocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (VOID **)&CpuArch);
  ASSERT_EFI_ERROR (Status);

  Status = CpuArch->SetMemoryAttributes (
                      CpuArch,
                      IoMmuContext->BaseAddress,
                      SIZE_4KB,
                      EFI_MEMORY_UC | EFI_MEMORY_XP
                      );
  ASSERT_EFI_ERROR (Status);

  //
  // Now, run the initialisation worker.
  //
  Status = InitialiseRiscVIoMmu (IoMmuContext);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to initialise the IOMMU\n"));
    return Status;
  }

  // TODO: This means that the services weren't called for PCI devices in the case of a PCI IOMMU?
  if (mHandle == NULL) {
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &mHandle,
                    &gEdkiiIoMmuProtocolGuid,
                    &mRiscVIoMmuProtocol,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}

/**
  Initialise the RISC-V IOMMU driver.

  @param[in]  ImageHandle  ImageHandle of the loaded driver
  @param[in]  SystemTable  Pointer to the System Table

  @retval  EFI_SUCCESS           The hardware is initialised and the protocol is installed.
  @retval  EFI_UNSUPPORTED       The detected IOMMU is not supported by this driver.

**/
EFI_STATUS
EFIAPI
RiscVIoMmuDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  LIST_ENTRY           *Link;
  RISCV_IOMMU_CONTEXT  *IoMmuContext;
  EFI_STATUS           Status;

  DetectRiscVIoMmus ();

  for (Link = GetFirstNode (&mRiscVIoMmuContexts)
       ; !IsNull (&mRiscVIoMmuContexts, Link)
       ; Link = GetNextNode (&mRiscVIoMmuContexts, Link)
       ) {
    IoMmuContext = RISCV_IOMMU_CONTEXT_FROM_LINK (Link);
    if (IoMmuContext->DriverState < STATE_AVAILABLE) {
      DEBUG ((
        DEBUG_INFO,
        "The RISC-V IOMMU %a device at 0x%lx will be initialised once available\n",
        IoMmuContext->IoMmuIsPciDevice ? "PCI" : "system",
        IoMmuContext->BaseAddress
        ));
      continue;
    }

    Status = IoMmuCommonInitialise (IoMmuContext);
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}
