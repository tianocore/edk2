/** @file
  RISC-V IOMMU device context construction.

  Copyright (c) 2025, 9elements GmbH. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseRiscVMmuLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/IoLib.h>
#include <Protocol/PciIo.h>
#include <Register/RiscV64/RiscVImpl.h>
#include "RiscVIoMmu.h"

#define RISCV_PG_R  BIT1
#define RISCV_PG_W  BIT2

//
// Detecting ATS capability on downstream PCI devices requires PciBus internals.
//
#include <Bus/Pci/PciBusDxe/PciBus.h>

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
  // Any combination of base/extended format and leaf/non-leaf level fits in one page.
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
STATIC
RISCV_IOMMU_DEVICE_CONTEXT_BASE *
LocateDeviceContext (
  IN RISCV_IOMMU_CONTEXT    *IoMmuContext,
  IN RISCV_IOMMU_DEVICE_ID  *IoMmuDeviceId
  )
{
  RISCV_IOMMU_CAPABILITIES         Capabilities;
  BOOLEAN                          ExtendedContext;
  RISCV_IOMMU_DDTP                 Ddtp;
  RISCV_IOMMU_DDT_NON_LEAF         *DdtEntry;
  UINT8                            Levels;
  UINTN                            Index;
  RISCV_IOMMU_DEVICE_CONTEXT_BASE  *DeviceContext;

  DEBUG ((RISCV_IOMMU_DEBUG_LEVEL, "%a: IoMmuDeviceId=0x%x on the IOMMU at 0x%lx\n", __func__, IoMmuDeviceId->Uint32, IoMmuContext->BaseAddress));

  //
  // Determine the format of the context struct.
  //
  Capabilities.Uint64 = MmioRead64 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_CAPABILITIES);
  ExtendedContext     = Capabilities.Bits.MSI_FLAT;

  //
  // Initialise non-leaf data.
  //
  Ddtp.Uint64 = MmioRead64 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_DDTP);
  DdtEntry    = (VOID *)((UINT64)Ddtp.Bits.PPN << RISCV_MMU_PAGE_SHIFT);

  Levels = Ddtp.Bits.iommu_mode - V_RISCV_IOMMU_DDTP_IOMMU_MODE_BARE;
  ASSERT ((Levels >= 1) && (Levels <= 3));

  //
  // Traverse the tree until a leaf.
  //
  while (Levels > 1) {
    if (Levels == 3) {
      Index = ExtendedContext ? IoMmuDeviceId->ExtendedFormat.Ddi2 : IoMmuDeviceId->BaseFormat.Ddi2;
    } else if (Levels == 2) {
      Index = ExtendedContext ? IoMmuDeviceId->ExtendedFormat.Ddi1 : IoMmuDeviceId->BaseFormat.Ddi1;
    }

    //
    // Connect the next level.
    //
    DdtEntry += Index;
    if (!DdtEntry->Bits.V) {
      AllocateDdtLevel (DdtEntry);
    }

    //
    // Continue exploring with the next level.
    //
    DdtEntry = (VOID *)((UINT64)DdtEntry->Bits.PPN << RISCV_MMU_PAGE_SHIFT);
    Levels--;
  }

  //
  // Initialise leaf data.
  //
  DeviceContext = (VOID *)DdtEntry;

  //
  // This pointer math operates on the base format, while the extended format
  // is twice as large, and therefore, twice as far.
  //
  Index          = ExtendedContext ? IoMmuDeviceId->ExtendedFormat.Ddi0 * 2 : IoMmuDeviceId->BaseFormat.Ddi0;
  DeviceContext += Index;

  DEBUG ((RISCV_IOMMU_DEBUG_LEVEL, "%a: DeviceContext=0x%lx\n", __func__, DeviceContext));

  return DeviceContext;
}

// TODO: Migrate into an MMU 'operating context' structure. Would we have to set this up for each call?
extern UINTN  mMaxRootTableLevel, mBitPerLevel, mTableEntryCount;

/**
  Update a device-specific page table attached to a device context.

  @retval EFI_INVALID_PARAMETER   The SATP mode was not valid.
  @retval EFI_OUT_OF_RESOURCES    Not enough resource.
  @retval EFI_DEVICE_ERROR        The SATP mode not supported by HW.
  @retval EFI_SUCCESS             The operation succesfully.

**/
STATIC
EFI_STATUS
RiscVIoMmuUpdateDevicePageTable (
  IN RISCV_IOMMU_CONTEXT              *IoMmuContext,
  IN RISCV_IOMMU_DEVICE_ID            *IoMmuDeviceId,
  IN RISCV_IOMMU_DEVICE_CONTEXT_BASE  *DeviceContext,
  IN EFI_PHYSICAL_ADDRESS             DeviceAddress,
  IN UINTN                            Length,
  IN UINT64                           IoMmuAccess
  )
{
  EFI_STATUS  Status;

  UpdateRegionMapping (
    DeviceAddress,
    ALIGN_VALUE (Length, SIZE_4KB),
    IoMmuAccess,
    PTE_ATTRIBUTES_MASK,
    (VOID *)((UINT64)DeviceContext->FirstStageContext.Bits.PPN << RISCV_MMU_PAGE_SHIFT),
    FALSE /* These page tables aren't live on the MMU */
    );

  Status = IoMmuInvalidatePageTableCache (IoMmuContext, DeviceAddress);
  ASSERT_EFI_ERROR (Status);

  if (DeviceContext->TranslationControl.Bits.EN_ATS) {
    Status = IoMmuInvalidateDownstreamDevAtc (IoMmuContext, IoMmuDeviceId, DeviceAddress);
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}

/**
  Create a device-specific page table and attach it to the device context.

  @retval EFI_INVALID_PARAMETER   The SATP mode was not valid.
  @retval EFI_OUT_OF_RESOURCES    Not enough resource.
  @retval EFI_DEVICE_ERROR        The SATP mode not supported by HW.
  @retval EFI_SUCCESS             The operation succesfully.

**/
STATIC
EFI_STATUS
RiscVIoMmuInitialiseDevicePageTable (
  IN RISCV_IOMMU_CONTEXT              *IoMmuContext,
  IN RISCV_IOMMU_DEVICE_ID            *IoMmuDeviceId,
  IN RISCV_IOMMU_DEVICE_CONTEXT_BASE  *DeviceContext,
  IN EFI_PHYSICAL_ADDRESS             DeviceAddress,
  IN UINTN                            Length,
  IN UINT64                           IoMmuAccess
  )
{
  UINTN       SatpMode;
  VOID        *TranslationTable;
  BOOLEAN     InterruptState;
  UINT64      Ppn;
  EFI_STATUS  Status;

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
    ALIGN_VALUE (Length, SIZE_4KB),
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

  SetInterruptState (InterruptState);

  Status = IoMmuInvalidatePageTableCache (IoMmuContext, DeviceAddress);
  ASSERT_EFI_ERROR (Status);

  if (DeviceContext->TranslationControl.Bits.EN_ATS) {
    Status = IoMmuInvalidateDownstreamDevAtc (IoMmuContext, IoMmuDeviceId, DeviceAddress);
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}

/**
  Locate PciExpress capability register block per capability ID.

  @param PciIoDevice       A pointer to the PCI_IO_DEVICE.
  @param CapId             The capability ID.
  @param Offset            A pointer to the offset returned.
  @param NextRegBlock      A pointer to the next block returned.

  @retval EFI_SUCCESS      Successfully located capability register block.
  @retval EFI_UNSUPPORTED  Pci device does not support capability.
  @retval EFI_NOT_FOUND    Pci device support but can not find register block.

**/
EFI_STATUS
LocatePciExpressCapabilityRegBlock (
  IN     PCI_IO_DEVICE  *PciIoDevice,
  IN     UINT16         CapId,
  IN OUT UINT32         *Offset,
  OUT UINT32            *NextRegBlock OPTIONAL
  )
{
  EFI_STATUS  Status;
  UINT32      CapabilityPtr;
  UINT32      CapabilityEntry;
  UINT16      CapabilityID;

  //
  // To check the capability of this device supports
  //
  if (!PciIoDevice->IsPciExp) {
    return EFI_UNSUPPORTED;
  }

  if (*Offset != 0) {
    CapabilityPtr = *Offset;
  } else {
    CapabilityPtr = EFI_PCIE_CAPABILITY_BASE_OFFSET;
  }

  while (CapabilityPtr != 0) {
    //
    // Mask it to DWORD alignment per PCI spec
    //
    CapabilityPtr &= 0xFFC;
    Status         = PciIoDevice->PciIo.Pci.Read (
                                              &PciIoDevice->PciIo,
                                              EfiPciIoWidthUint32,
                                              CapabilityPtr,
                                              1,
                                              &CapabilityEntry
                                              );
    if (EFI_ERROR (Status)) {
      break;
    }

    if (CapabilityEntry == MAX_UINT32) {
      DEBUG ((
        DEBUG_WARN,
        "%a: [%02x|%02x|%02x] failed to access config space at offset 0x%x\n",
        __func__,
        PciIoDevice->BusNumber,
        PciIoDevice->DeviceNumber,
        PciIoDevice->FunctionNumber,
        CapabilityPtr
        ));
      break;
    }

    CapabilityID = (UINT16)CapabilityEntry;

    if (CapabilityID == CapId) {
      *Offset = CapabilityPtr;
      if (NextRegBlock != NULL) {
        *NextRegBlock = (CapabilityEntry >> 20) & 0xFFF;
      }

      return EFI_SUCCESS;
    }

    CapabilityPtr = (CapabilityEntry >> 20) & 0xFFF;
  }

  return EFI_NOT_FOUND;
}

/**
  Set IOMMU attributes for accessing system memory.

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
  IN UINT64                 IoMmuAccess,
  IN EFI_PCI_IO_PROTOCOL    *PciIo
  )
{
  UINT64                           RiscVMmuAccessBits;
  RISCV_IOMMU_DEVICE_CONTEXT_BASE  *DeviceContext;
  RISCV_IOMMU_FCTL                 FeatureControl;
  RISCV_IOMMU_CAPABILITIES         Capabilities;
  UINT32                           PciCapOffset;
  EFI_STATUS                       Status;

  //
  // The IOMMU protocol's access bits map to the RISC-V Privileged spec (MMU),
  // but first, the RISC-V spec defines BIT0 as the 'valid' bit.
  //
  RiscVMmuAccessBits = IoMmuAccess << 1;

  //
  // Per the RISC-V Privileged spec (MMU), a writeable entry must be readable.
  //
  if (RiscVMmuAccessBits & RISCV_PG_W) {
    RiscVMmuAccessBits |= RISCV_PG_R;
  }

  DeviceContext = LocateDeviceContext (IoMmuContext, IoMmuDeviceId);
  if (DeviceContext->TranslationControl.Bits.V) {
    Status = RiscVIoMmuUpdateDevicePageTable (IoMmuContext, IoMmuDeviceId, DeviceContext, DeviceAddress, Length, RiscVMmuAccessBits);
    ASSERT_EFI_ERROR (Status);

    Status = ProbeFaultQueueForErrors (IoMmuContext);
    ASSERT_EFI_ERROR (Status);

    return Status;
  }

  //
  // Copy global feature support/enablement across.
  //
  FeatureControl.Uint32                      = MmioRead32 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_FCTL);
  DeviceContext->TranslationControl.Bits.SBE = FeatureControl.Bits.BE;
  DeviceContext->TranslationControl.Bits.SXL = FeatureControl.Bits.GXL;

  //
  // General IOMMU controls: update dependent bits together.
  //
  Capabilities.Uint64                         = MmioRead64 (IoMmuContext->BaseAddress + R_RISCV_IOMMU_CAPABILITIES);
  DeviceContext->TranslationControl.Bits.SADE = Capabilities.Bits.AMO_HWAD;

  //
  // If the IOMMU and device support ATS, enable it.
  // This can improve performance via the DevATC.
  //
  if (Capabilities.Bits.ATS && (PciIo != NULL)) {
    PciCapOffset = 0;
    Status       = LocatePciExpressCapabilityRegBlock (
                     PCI_IO_DEVICE_FROM_PCI_IO_THIS (PciIo),
                     EFI_PCIE_CAPABILITY_ID_ATS,
                     &PciCapOffset,
                     NULL
                     );
    if (!EFI_ERROR (Status)) {
      DeviceContext->TranslationControl.Bits.EN_ATS = 1;
      // Unlike in an OS environment, device-driven operation is not expected (such as to VRAM).
      DeviceContext->TranslationControl.Bits.EN_PRI = 0;
      //DeviceContext->TranslationControl.Bits.PRPR   = 1;
      // Without a hypervisor involved, they may be translated directly to an SPA.
      DeviceContext->TranslationControl.Bits.T2GPA  = 0;

      DEBUG ((RISCV_IOMMU_DEBUG_LEVEL, "Enabled ATS\n"));
    }
  }

  //
  // As a non-virtualised OS, neither process contexts nor second-level translation are in use.
  //
  DeviceContext->TranslationControl.Bits.PDTV                               = 0; // This reserves DPE. TODO: We don't write the requests. If there's a process_id - could there be? - then this is wrong.
  DeviceContext->IoHypervisorGuestAddressTranslationAndProtection.Bits.MODE = V_RISCV_IOMMU_DC_IOMMU_MODE_BARE;

  Status = RiscVIoMmuInitialiseDevicePageTable (IoMmuContext, IoMmuDeviceId, DeviceContext, DeviceAddress, Length, RiscVMmuAccessBits);
  ASSERT_EFI_ERROR (Status);

  //
  // Activate this device context.
  // - IOMMU interrupts aren't in use, so neither is the extended part of the DC.
  //
  DeviceContext->TranslationControl.Bits.V = 1;

  Status = IoMmuInvalidateDeviceDirectoryCache (IoMmuContext, IoMmuDeviceId);
  ASSERT_EFI_ERROR (Status);

  Status = ProbeFaultQueueForErrors (IoMmuContext);
  ASSERT_EFI_ERROR (Status);

  return Status;
}
