/** @file
  RISC-V IOMMU driver.

  Copyright (c) 2025, 9elements GmbH. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseRiscVMmuLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Register/RiscV64/RiscVImpl.h>
#include "RiscVIoMmu.h"

/**
  Allocate a level of the DDT.

**/
STATIC
VOID
AllocateDdtLevel (
  IN OUT RISCV_IOMMU_DDT_NON_LEAF  *DdtParent
  )
{
  VOID  *Buffer;

  //
  // Any combination of base/extended format and leaf/non-leaf level
  // fits in one page.
  //
  Buffer = AllocatePages (1);
  ASSERT (Buffer != NULL);

  ZeroMem (Buffer, SIZE_4KB);

  DdtParent->Bits.PPN = ((UINT64)Buffer) >> RISCV_MMU_PAGE_SHIFT;
  DdtParent->Bits.V   = 1;

  DEBUG ((RISCV_IOMMU_DEBUG_LEVEL, "%a: Inserted a level at 0x%x to the level at 0x%x\n", __func__, Buffer, DdtParent));
}

/**
  Find the device context for the requested device_id, creating it if necessary.

  @param[in]  IoMmuDeviceId

  @retval DeviceContext

**/
RISCV_IOMMU_DEVICE_CONTEXT_BASE *
LocateDeviceContext (
  IN RISCV_IOMMU_CONTEXT    *IoMmuContext,
  IN RISCV_IOMMU_DEVICE_ID  *IoMmuDeviceId
  )
{
  CONTEXT_WRAPPER                  *ContextWrapper;
  RISCV_IOMMU_DDTP                 Ddtp;
  RISCV_IOMMU_DDT_NON_LEAF         *DdtEntry;
  UINT16                           Levels;
  UINT16                           Index;
  RISCV_IOMMU_DEVICE_CONTEXT_BASE  *DeviceContext;

  ContextWrapper = &IoMmuContext->DeviceContext;

  DEBUG ((RISCV_IOMMU_DEBUG_LEVEL, "%a: IoMmuDeviceId=0x%x\n", __func__, IoMmuDeviceId->Uint32));

  //
  // Initialise non-leaf data.
  //
  Ddtp.Uint64 = IoMmuRead64 (IoMmuContext, R_RISCV_IOMMU_DDTP);
  DdtEntry = (VOID *)((UINT64)Ddtp.Bits.PPN << RISCV_MMU_PAGE_SHIFT);

  Levels = Ddtp.Bits.iommu_mode - V_RISCV_IOMMU_DDTP_IOMMU_MODE_BARE;
  ASSERT ((Levels >= 1) && (Levels <= 3));

  //
  // Unrolling was the easiest approach with a union-of-structs.
  //
  if (Levels == 3) {
    Index = ContextWrapper->IsExtended ? IoMmuDeviceId->ExtendedFormat.Ddi2 : IoMmuDeviceId->BaseFormat.Ddi2;
    DdtEntry += Index;

    DEBUG ((DEBUG_INFO, "ATTN: DDI2 is 0x%x, this DDT entry is at 0x%x\n", Index, DdtEntry));

    //
    // Connect the next level.
    //
    if (!DdtEntry->Bits.V) {
      DEBUG ((DEBUG_INFO, "ATTN: It's not valid\n"));
      AllocateDdtLevel (DdtEntry);
    }

    //
    // Continue exploring with the next level.
    //
    DdtEntry = (VOID *)((UINT64)DdtEntry->Bits.PPN << RISCV_MMU_PAGE_SHIFT);
    DEBUG ((DEBUG_INFO, "ATTN: continuing search with DDT entry at 0x%x\n", DdtEntry));
  }

  if (Levels >= 2) {
    Index = ContextWrapper->IsExtended ? IoMmuDeviceId->ExtendedFormat.Ddi1 : IoMmuDeviceId->BaseFormat.Ddi1;
    DdtEntry += Index;

    DEBUG ((DEBUG_INFO, "ATTN: DDI1 is 0x%x, this DDT entry is at 0x%x\n", Index, DdtEntry));

    //
    // Connect the next level.
    //
    if (!DdtEntry->Bits.V) {
      DEBUG ((DEBUG_INFO, "ATTN: It's not valid\n"));
      AllocateDdtLevel (DdtEntry);
    }

    //
    // Continue exploring with the next level.
    //
    DdtEntry = (VOID *)((UINT64)DdtEntry->Bits.PPN << RISCV_MMU_PAGE_SHIFT);
    DEBUG ((DEBUG_INFO, "ATTN: continuing search with DDT entry at 0x%x\n", DdtEntry));
  }

  //
  // Initialise leaf data.
  //
  DeviceContext = (VOID *)DdtEntry;

  //
  // This pointer math operates on the base format, while
  // the extended format is twice as large, and therefore, twice as far.
  //
  Index = ContextWrapper->IsExtended ? IoMmuDeviceId->ExtendedFormat.Ddi0 * 2 : IoMmuDeviceId->BaseFormat.Ddi0;
  DeviceContext += Index;
  DEBUG ((DEBUG_INFO, "ATTN: DDI0 is 0x%x, this DDT entry is at 0x%x\n", ContextWrapper->IsExtended ? IoMmuDeviceId->ExtendedFormat.Ddi0 * 2 : IoMmuDeviceId->BaseFormat.Ddi0, DeviceContext));

  return DeviceContext;
}

// TODO: Migrate into an MMU 'operating context' structure.
extern UINTN mMaxRootTableLevel, mBitPerLevel, mTableEntryCount;

/**
  Set SATP mode.

  @param  SatpMode  The SATP mode to be set.

  @retval EFI_INVALID_PARAMETER   The SATP mode was not valid.
  @retval EFI_OUT_OF_RESOURCES    Not enough resource.
  @retval EFI_DEVICE_ERROR        The SATP mode not supported by HW.
  @retval EFI_SUCCESS             The operation succesfully.

**/
STATIC
EFI_STATUS
RiscVIoMmuSetSatpMode  (
  IN RISCV_IOMMU_DEVICE_CONTEXT_BASE  *DeviceContext,
  IN EFI_PHYSICAL_ADDRESS             DeviceAddress,
  IN UINTN                            Length,
  IN UINT64                           IoMmuAccess
  )
{
  UINTN    SatpMode;
  VOID     *TranslationTable;
  BOOLEAN  InterruptState;
  UINT64   Ppn;

  SatpMode = (RiscVGetSupervisorAddressTranslationRegister () & SATP64_MODE) >> SATP64_MODE_SHIFT;
  if ((DeviceContext->TranslationControl.Bits.SXL) && (SatpMode > SATP_MODE_SV32)) {
    SatpMode = SATP_MODE_SV32;
  }

  switch (SatpMode) {
    case SATP_MODE_OFF:
      return EFI_SUCCESS;
    case SATP_MODE_SV39:
      mMaxRootTableLevel = 3;
      mBitPerLevel       = 9;
      mTableEntryCount   = 512;
      break;
    case SATP_MODE_SV48:
      mMaxRootTableLevel = 4;
      mBitPerLevel       = 9;
      mTableEntryCount   = 512;
      break;
    case SATP_MODE_SV57:
      mMaxRootTableLevel = 5;
      mBitPerLevel       = 9;
      mTableEntryCount   = 512;
      break;
    default:
      return EFI_INVALID_PARAMETER;
  }

  // Allocate pages for translation table
  TranslationTable = AllocatePages (1);
  if (TranslationTable == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (TranslationTable, mTableEntryCount * sizeof (UINT64));

  // Default Read/Write attribute for memory mapped IO
  UpdateRegionMapping (
    DeviceAddress,
    Length,
    IoMmuAccess,
    PTE_ATTRIBUTES_MASK,
    TranslationTable,
    FALSE
    );

  InterruptState = SaveAndDisableInterrupts ();

  Ppn = (UINT64)TranslationTable >> RISCV_MMU_PAGE_SHIFT;
  ASSERT (!(Ppn & ~(SATP64_PPN)));

  DeviceContext->FirstStageContext.Bits.PPN  = Ppn;
  DeviceContext->FirstStageContext.Bits.MODE = SatpMode;

  //RiscVLocalTlbFlushAll ();

  SetInterruptState (InterruptState);

  return EFI_SUCCESS;
}

/**
  Set IOMMU attributes for accessing system memory.

  TODO: We also need the reverse of this.

  @param[in]  DeviceHandle      The device who initiates the DMA access request.
  @param[in]  Mapping           The mapping value returned from Map().
  @param[in]  IoMmuAccess       The IOMMU access.

  @retval EFI_SUCCESS            The IoMmuAccess is set for the memory range specified by DeviceAddress and Length.
  @retval EFI_DEVICE_ERROR       The IOMMU device reported an error while attempting the operation.

**/
EFI_STATUS
RiscVIoMmuSetAttributeWorker (
  IN RISCV_IOMMU_CONTEXT    *IoMmuContext,
  IN RISCV_IOMMU_DEVICE_ID  *IoMmuDeviceId,
  IN EFI_PHYSICAL_ADDRESS   DeviceAddress,
  IN UINTN                  Length,
  IN UINT64                 IoMmuAccess
  )
{
  UINT64                           RiscVMmuAccessBits;
  RISCV_IOMMU_DEVICE_CONTEXT_BASE  *DeviceContext;
  RISCV_IOMMU_FCTL                 FeatureControl;
  RISCV_IOMMU_CAPABILITIES         Capabilities;
  EFI_STATUS                       Status;

  //
  // The IOMMU protocol's access bits map to the RISC-V Privileged spec (MMU),
  // but first, the RISC-V spec defines BIT0 as 'valid.'
  //
  RiscVMmuAccessBits = IoMmuAccess << 1;

  DeviceContext = LocateDeviceContext (IoMmuContext, IoMmuDeviceId);
  if (DeviceContext->TranslationControl.Bits.V) {
    UpdateRegionMapping (
      DeviceAddress,
      Length,
      RiscVMmuAccessBits,
      PTE_ATTRIBUTES_MASK,
      (VOID *)((UINT64)DeviceContext->FirstStageContext.Bits.PPN << RISCV_MMU_PAGE_SHIFT),
      TRUE
      );
    return EFI_SUCCESS;
  }

  //
  // Copy global feature support/enablement across.
  //
  FeatureControl.Uint32 = IoMmuRead32 (IoMmuContext, R_RISCV_IOMMU_FCTL);
  DeviceContext->TranslationControl.Bits.SBE = FeatureControl.Bits.BE;
  DeviceContext->TranslationControl.Bits.SXL = FeatureControl.Bits.GXL;

  //
  // General IOMMU controls: update dependent bits together.
  //
  Capabilities.Uint64 = IoMmuRead64 (IoMmuContext, R_RISCV_IOMMU_CAPABILITIES);
  DeviceContext->TranslationControl.Bits.SADE = Capabilities.Bits.AMO_HWAD;
  //DeviceContext->TranslationControl.Bits.DTF  = 1;

  //
  // If the IOMMU supports ATS, enable those device-generated requests.
  // This can improve performance via the DevATC.
  //
  if (Capabilities.Bits.ATS) {
    DeviceContext->TranslationControl.Bits.EN_ATS = 1;
    // TODO: Unlike on an OS kernel, device-driven operation is not expected (such as to VRAM).
    DeviceContext->TranslationControl.Bits.EN_PRI = 0;
    //DeviceContext->TranslationControl.Bits.PRPR   = 1;
    // Without a hypervisor involved, they may be translated directly to an SPA.
    DeviceContext->TranslationControl.Bits.T2GPA  = 0;
  }

  //
  // As a non-virtualised OS, neither process contexts nor second-level translation are in use.
  //
  DeviceContext->TranslationControl.Bits.PDTV = 0; // This reserves DPE. TODO: We don't write the requests. If there's a process_id - could there be? - then this is wrong.
  DeviceContext->IoHypervisorGuestAddressTranslationAndProtection.Bits.MODE = V_RISCV_IOMMU_DC_IOMMU_MODE_BARE;

  //
  // TODO: IOMMU QoS IDs RCID and MCID.
  //

  Status = RiscVIoMmuSetSatpMode (DeviceContext, DeviceAddress, Length, RiscVMmuAccessBits);
  ASSERT_EFI_ERROR (Status);

  //
  // Activate this device context.
  // - IOMMU interrupts aren't in use, so neither is the extended part of the DC.
  //
  DeviceContext->TranslationControl.Bits.V = 1;

  ASSERT_EFI_ERROR (ProbeHardwareQueuesForFaults (IoMmuContext));

  return EFI_SUCCESS;
}
