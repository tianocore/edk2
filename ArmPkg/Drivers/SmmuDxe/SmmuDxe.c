/** @file SmmuDxe.c

    This file contains functions for the SMMU driver.

    This driver consumes a SMMU_CONFIG Hob structure defined by the platform to configure the SMMU hardware.
    Initializes the SmmuV3 hardware to enable stage 2 translation and dma remapping.
    Installs the IORT to describe the SMMU configuration to the OS.
    Implements the IoMmu protocol to provide a generic interface for mapping host memory to device memory.

    Copyright (c) Microsoft Corporation.
    SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/ArmLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Protocol/AcpiTable.h>
#include <Guid/SmmuConfig.h>
#include "IoMmu.h"
#include "SmmuV3.h"

// Global IOMMU/SMMU instance
IOMMU_CONFIG  *mIoMmu;

// GIC interrupt protocol used to register the SMMU EVTQ / GERR ISRs.
EFI_HARDWARE_INTERRUPT2_PROTOCOL  *mGicInterrupt = NULL;

// Global IORT data pointer - saved for lookups during runtime
VOID  *mIortData = NULL;

/**
  Add the IORT ACPI table.

  @param [in]  AcpiTableProtocol    Pointer to the ACPI Table Protocol.
  @param [in]  IortData             Pointer to the IORT.
  @param [in]  IortSize             Size of the IORT table.

  @retval EFI_SUCCESS               Success.
  @retval EFI_OUT_OF_RESOURCES      Out of resources.
  @retval EFI_INVALID_PARAMETER     Invalid parameter.
**/
STATIC
EFI_STATUS
AddIortTable (
  IN EFI_ACPI_TABLE_PROTOCOL  *AcpiTable,
  IN VOID                     *IortData,
  IN UINT32                   IortSize
  )
{
  EFI_STATUS  Status;
  UINTN       TableHandle;

  if ((AcpiTable == NULL) || (IortData == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Parameters\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  Status = AcpiTable->InstallAcpiTable (
                        AcpiTable,
                        (EFI_ACPI_COMMON_HEADER *)(UINTN)IortData,
                        IortSize,
                        &TableHandle
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to install IORT table\n", __func__));
  }

  return Status;
}

/**
  Initialize a page table. Only initializes the root page table.
  UpdateMapping() will allocate entries on the fly as needed.

  To support concatenated page tables, allocate the max amount of pages we are allowed to concatenate, 16.

  @retval A pointer to the initialized page table, or NULL on failure.
**/
PAGE_TABLE *
SmmuV3AllocatePageTableRoot (
  VOID
  )
{
  PAGE_TABLE  *PageTable;

  // To support concatenated page tables, allocate the max amount of pages we are allowed to concatenate, 16.
  // Arm DDI0487L_a_a-profile_architecture_reference_manual section D8.2.2 states:
  // Align the base address of the first translation table to the sum of the size of the memory occupied by the concatenated translation tables.
  PageTable = (PAGE_TABLE *)AllocateAlignedPages (
                              PAGE_TABLE_ROOT_CONCATENATED_PAGES_MAX,
                              EFI_PAGES_TO_SIZE (PAGE_TABLE_ROOT_CONCATENATED_PAGES_MAX)
                              );

  if (PageTable == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate page table\n", __func__));
    return NULL;
  }

  ZeroMem (PageTable, EFI_PAGES_TO_SIZE (PAGE_TABLE_ROOT_CONCATENATED_PAGES_MAX));

  return PageTable;
}

/**
  Recursivley deinitialize and free a page table for all previously
  allocated entries, given its level and pointer.

  @param [in]  Level      The level of the page table to deinitialize.
  @param [in]  PageTable  The page table to deinitialize.
**/
VOID
SmmuV3FreePageTableTree (
  IN UINT8       Level,
  IN PAGE_TABLE  *PageTable
  )
{
  UINTN             Index;
  PAGE_TABLE_ENTRY  Entry;
  PAGE_TABLE        *PageTableAddress;

  if ((Level >= PAGE_TABLE_DEPTH) || (PageTable == NULL)) {
    return;
  }

  for (Index = 0; Index < PAGE_TABLE_SIZE; Index++) {
    Entry            = PageTable->Entries[Index];
    PageTableAddress = (PAGE_TABLE *)((UINTN)Entry & ~PAGE_TABLE_BLOCK_MASK);

    if (Entry != 0) {
      SmmuV3FreePageTableTree (Level + 1, PageTableAddress);
    }
  }

  // For root level, use larger size to free for supporting a concatenated page table root.
  if (Level == 0) {
    FreePages (PageTable, PAGE_TABLE_ROOT_CONCATENATED_PAGES_MAX);
  } else {
    FreePages (PageTable, 1);
  }
}

/**
  Allocate an event queue for SMMUv3.

  @param [in]   SmmuInfo       Pointer to the SMMU_INFO structure.
  @param [out]  QueueLog2Size  Pointer to store the log2 size of the queue.
  @param [out]  EventQueueBase Pointer to store the base address of the allocated event queue.

  @retval EFI_SUCCESS          The event queue was allocated successfully.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  Allocation failed due to insufficient resources.
**/
STATIC
EFI_STATUS
SmmuV3AllocateEventQueue (
  IN  SMMU_INFO  *SmmuInfo,
  OUT UINT32     *QueueLog2Size,
  OUT VOID       **EventQueueBase
  )
{
  UINT32       QueueSize;
  SMMUV3_IDR1  Idr1;
  UINT32       Pages;

  if ((SmmuInfo == NULL) || (QueueLog2Size == NULL) || (EventQueueBase == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Parameters\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  Idr1.AsUINT32 = SmmuV3ReadRegister32 (SmmuInfo->SmmuBase, SMMU_IDR1);

  *QueueLog2Size  = MIN (Idr1.Bits.EventQs, SMMUV3_EVENT_QUEUE_LOG2ENTRIES);
  QueueSize       = SMMUV3_EVENT_QUEUE_SIZE_FROM_LOG2 (*QueueLog2Size);
  Pages           = EFI_SIZE_TO_PAGES (QueueSize);
  *EventQueueBase = AllocatePages (Pages);

  if (*EventQueueBase == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Allocation failed\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (*EventQueueBase, EFI_PAGES_TO_SIZE (Pages));
  return EFI_SUCCESS;
}

/**
  Allocate a command queue for SMMUv3.

  @param [in]   SmmuInfo       Pointer to the SMMU_INFO structure.
  @param [out]  QueueLog2Size  Pointer to store the log2 size of the queue.
  @param [out]  CmdQueueBase   Pointer to store the base address of the allocated command queue.

  @retval EFI_SUCCESS          The command queue was allocated successfully.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  Allocation failed due to insufficient resources.
**/
STATIC
EFI_STATUS
SmmuV3AllocateCommandQueue (
  IN  SMMU_INFO  *SmmuInfo,
  OUT UINT32     *QueueLog2Size,
  OUT VOID       **CmdQueueBase
  )
{
  UINT32       QueueSize;
  SMMUV3_IDR1  Idr1;
  UINT32       Pages;

  if ((SmmuInfo == NULL) || (QueueLog2Size == NULL) || (CmdQueueBase == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Parameters\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  Idr1.AsUINT32 = SmmuV3ReadRegister32 (SmmuInfo->SmmuBase, SMMU_IDR1);

  *QueueLog2Size = MIN (Idr1.Bits.CmdQs, SMMUV3_COMMAND_QUEUE_LOG2ENTRIES);
  QueueSize      = SMMUV3_COMMAND_QUEUE_SIZE_FROM_LOG2 (*QueueLog2Size);
  Pages          = EFI_SIZE_TO_PAGES (QueueSize);
  *CmdQueueBase  = AllocatePages (Pages);

  if (*CmdQueueBase == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Allocation failed\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (*CmdQueueBase, EFI_PAGES_TO_SIZE (Pages));
  return EFI_SUCCESS;
}

/**
  Free a previously allocated event queue.

  @param [in]  QueuePtr    Pointer to the queue to free.
  @param [in]  Log2Size    Log2 of the queue entry count, used to recover the
                           original allocation size.
**/
STATIC
VOID
SmmuV3FreeEventQueue (
  IN VOID    *QueuePtr,
  IN UINT32  Log2Size
  )
{
  UINT32  Size;

  if (QueuePtr == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid parameters. QueuePtr == NULL\n", __func__));
  } else {
    Size = SMMUV3_EVENT_QUEUE_SIZE_FROM_LOG2 (Log2Size);
    FreePages ((VOID *)QueuePtr, EFI_SIZE_TO_PAGES (Size));
  }
}

/**
  Free a previously allocated command queue.

  @param [in]  QueuePtr    Pointer to the queue to free.
  @param [in]  Log2Size    Log2 of the queue entry count, used to recover the
                           original allocation size.
**/
STATIC
VOID
SmmuV3FreeCommandQueue (
  IN VOID    *QueuePtr,
  IN UINT32  Log2Size
  )
{
  UINT32  Size;

  if (QueuePtr == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid parameters. QueuePtr == NULL\n", __func__));
  } else {
    Size = SMMUV3_COMMAND_QUEUE_SIZE_FROM_LOG2 (Log2Size);
    FreePages ((VOID *)QueuePtr, EFI_SIZE_TO_PAGES (Size));
  }
}

/**
  Build an invalid stream-table entry used at init for every STE
  slot before any device has been mapped.

  Implemented by reusing SmmuV3BuildTranslateStreamTableEntry with
  PageTableRoot == NULL VMID=0 and VALID = 0.

  @param [in]  SmmuInfo     SMMU instance (needed for IDR-derived fields).
  @param [out] StreamEntry  STE buffer to populate.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameters.
  @retval Other                  Failure from SmmuV3BuildTranslateStreamTableEntry.
**/
EFI_STATUS
SmmuV3BuildInvalidStreamTableEntry (
  IN  SMMU_INFO                  *SmmuInfo,
  OUT SMMUV3_STREAM_TABLE_ENTRY  *StreamEntry
  )
{
  if ((SmmuInfo == NULL) || (StreamEntry == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Parameters\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  // PageTableRoot==NULL VMID==0 and VALID==0.
  return SmmuV3BuildTranslateStreamTableEntry (SmmuInfo, NULL, 0, StreamEntry);
}

/**
  Build a Valid STAGE_2_TRANSLATE stream-table entry using the
  given page-table root.

  @param [in]  SmmuInfo        Pointer to the SMMU_INFO structure.
  @param [in]  PageTableRoot   Page-table root the STE should point at.
  @param [in]  Vmid            VMID tag to install in the STE's S2VMID field.
  @param [out] StreamEntry     STE buffer to populate.

  @retval EFI_SUCCESS         Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter.
**/
EFI_STATUS
SmmuV3BuildTranslateStreamTableEntry (
  IN  SMMU_INFO                  *SmmuInfo,
  IN  PAGE_TABLE                 *PageTableRoot,
  IN  UINT16                     Vmid,
  OUT SMMUV3_STREAM_TABLE_ENTRY  *StreamEntry
  )
{
  EFI_STATUS   Status;
  SMMUV3_IDR1  Idr1;
  SMMUV3_IDR5  Idr5;
  UINT32       CCA;
  UINT8        CPM;
  UINT8        DACS;
  UINT64       S2Sl0;

  if ((SmmuInfo == NULL) || (StreamEntry == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Parameters\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  CCA  = SMMUV3_STREAM_TABLE_ENTRY_CCA;
  CPM  = SMMUV3_STREAM_TABLE_ENTRY_CPM;
  DACS = SMMUV3_STREAM_TABLE_ENTRY_DACS;

  ZeroMem ((VOID *)StreamEntry, sizeof (*StreamEntry));

  Idr1.AsUINT32 = SmmuV3ReadRegister32 (SmmuInfo->SmmuBase, SMMU_IDR1);
  Idr5.AsUINT32 = SmmuV3ReadRegister32 (SmmuInfo->SmmuBase, SMMU_IDR5);

  StreamEntry->Bits.Config = SMMUV3_STREAM_TABLE_ENTRY_CONFIG_STAGE_2_TRANSLATE_STAGE_1_BYPASS;
  // ATS is not supported in this implementation, so set the ATS field to 0.
  StreamEntry->Bits.Eats   = SMMUV3_STREAM_TABLE_ENTRY_EATS_NOT_SUPPORTED;
  StreamEntry->Bits.S2Vmid = Vmid;                                // Per-stream VMID (allocated by SmmuV3StreamGetOrCreate).
  StreamEntry->Bits.S2Tg   = SMMUV3_STREAM_TABLE_ENTRY_S2TG_4KB;
  StreamEntry->Bits.S2Aa64 = 1;                                   // AArch64 S2 translation tables
  if (PageTableRoot != NULL) {
    StreamEntry->Bits.S2Ttb = (UINT64)(UINTN)PageTableRoot >> SMMUV3_STREAM_TABLE_ENTRY_S2TTB_OFFSET;
  } else {
    StreamEntry->Bits.S2Ttb = 0;  // For abort STEs, S2Ttb is set to 0 so any access will fault since it is not a valid page-table root.
  }

  // Set the maximum output address width.
  // SmmuDxe does not support output address widths greater than 48 bits.
  SmmuInfo->OutputAddressWidth = SmmuV3DecodeAddressWidth (Idr5.Bits.Oas);

  if (SmmuInfo->OutputAddressWidth < SMMUV3_STREAM_TABLE_ENTRY_OUTPUT_ADDRESS_MAX) {
    StreamEntry->Bits.S2Ps = SmmuV3EncodeAddressWidth (SmmuInfo->OutputAddressWidth);
  } else {
    DEBUG ((DEBUG_INFO, "%a: Advertised OutputAddressWidth >= 48. Capping the width to 48 per the SMMU spec.\n", __func__));
    StreamEntry->Bits.S2Ps       = SmmuV3EncodeAddressWidth (SMMUV3_STREAM_TABLE_ENTRY_OUTPUT_ADDRESS_MAX);
    SmmuInfo->OutputAddressWidth = SMMUV3_STREAM_TABLE_ENTRY_OUTPUT_ADDRESS_MAX;
  }

  Status = SmmuV3SetTranslationStartingLevel (SmmuInfo, SmmuInfo->OutputAddressWidth, &S2Sl0);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to set translation starting level\n", __func__));
    return Status;
  }

  // S2SL0      Meaning
  // <https://developer.arm.com/documentation/ddi0601/2026-06/AArch64-Registers/VTCR-EL2--Virtualization-Translation-Control-Register>
  // Starting level of the stage 2 translation lookup, controlled by VTCR_EL2. The meaning of this field depends on the value of VTCR_EL2.TG0.
  // 0x2:
  // If VTCR_EL2.TG0 is 0b00 (4KB granule):
  // If FEAT_LPA2 is not implemented, start at level 0.
  // If FEAT_LPA2 is implemented and VTCR_EL2.SL2 is 0b0, start at level 0.
  // If FEAT_LPA2 is implemented, the combination of VTCR_EL2.SL0 == 10 and VTCR_EL2.SL2 == 1 is reserved.
  // If VTCR_EL2.TG0 is 0b10 (16KB granule) or 0b01 (64KB granule), start at level 1.
  //
  StreamEntry->Bits.S2Sl0  = S2Sl0;
  StreamEntry->Bits.S2T0Sz = 64 - SmmuInfo->OutputAddressWidth;

  // Set cache attributes as:
  // - Inner/Outer cacheability -> Write-back-cacheable (WBC),
  //           Read-Allocate (RA), Write-Allocate (WA)
  // - Shareability -> Inner-shareable.
  StreamEntry->Bits.S2Ir0 = ARM64_RGNCACHEATTR_WRITEBACK_WRITEALLOCATE;
  StreamEntry->Bits.S2Or0 = ARM64_RGNCACHEATTR_WRITEBACK_WRITEALLOCATE;
  StreamEntry->Bits.S2Sh0 = ARM64_SHATTR_INNER_SHAREABLE;

  StreamEntry->Bits.S2Rs = SMMUV3_STREAM_TABLE_ENTRY_S2RS_RECORD_FAULTS;

  if (Idr1.Bits.AttrTypesOvr != 0) {
    StreamEntry->Bits.ShCfg = SMMUV3_STREAM_TABLE_ENTRY_SHCFG_INCOMING_SHAREABILITY;
  }

  // If the device requires memory attribute overrides, then hard-code it to
  // Inner+Outer write-back cached and Inner-shareable (IWB-OWB-ISH) as
  // given by the IORT spec.
  if ((Idr1.Bits.AttrTypesOvr != 0) && ((CCA == 1) && (CPM == 1) && (DACS == 0))) {
    StreamEntry->Bits.Mtcfg   = SMMUV3_STREAM_TABLE_ENTRY_MTCFG;
    StreamEntry->Bits.MemAttr = SMMUV3_STREAM_TABLE_ENTRY_MEMATTR_INNER_OUTTER_WRITEBACK_CACHED;
    StreamEntry->Bits.ShCfg   = SMMUV3_STREAM_TABLE_ENTRY_SHCFG_INNER_SHAREABLE;
  }

  if (PageTableRoot != NULL) {
    StreamEntry->Bits.Valid = SMMUV3_STREAM_TABLE_ENTRY_VALID;
  } else {
    StreamEntry->Bits.Valid = 0;
  }

  return Status;
}

/**
  2-level stream tables only: if the L1 descriptor covering StreamId still
  points at the shared-ABORT L2 page (SmmuInfo->SharedAbortL2), seed the
  caller-provided L2 page (NewL2) with copies of the shared ABORT STE and
  rewrite the L1 descriptor to point at it.
  The caller owns the lifetime of NewL2; this function does not allocate or free it.

  Each L1 index that sees at least one promotion gets its own private L2.
  Other L1 indices keep aliasing the shared page until they too see
  a promotion.

  Must be issued with break-before-make for every STE in the L1's coverage
  because the L1 descriptor change invalidates the SMMU's cached STE
  pointers for that whole range.

  @param [in]   SmmuInfo       Pointer to the SMMU_INFO structure.
  @param [in]   StreamId       StreamID whose L1 slot may need splitting.
  @param [in]   NewL2          Caller-provided L2 page (page-sized, writable)
                               that the L1 descriptor will be swung to point at.
                               Used only if a split is actually needed.
  @param [out]  NewL2Consumed  Set to TRUE if NewL2 was installed into the L1
                               descriptor (ownership has transferred to the
                               SMMU and the caller must NOT free it), FALSE
                               if no split was performed (caller is free to
                               release NewL2).

  @retval EFI_SUCCESS            No split needed, or split succeeded.
  @retval EFI_INVALID_PARAMETER  Invalid parameters.
  @retval Other                  Command-queue failure.
**/
STATIC
EFI_STATUS
SmmuV3SplitL1IfShared (
  IN  SMMU_INFO                  *SmmuInfo,
  IN  UINT32                     StreamId,
  IN  SMMUV3_STREAM_TABLE_ENTRY  *NewL2,
  OUT BOOLEAN                    *NewL2Consumed
  )
{
  EFI_STATUS                         Status;
  SMMUV3_L1_STREAM_TABLE_DESCRIPTOR  NewDesc;
  SMMUV3_L1_STREAM_TABLE_DESCRIPTOR  *L1Table;
  SMMUV3_L1_STREAM_TABLE_DESCRIPTOR  *L1Desc;
  UINT64                             SharedAbortL2Encoded;
  UINT32                             L1Index;
  UINT32                             BaseStreamId;
  SMMUV3_CMD_GENERIC                 Command;

  if ((SmmuInfo == NULL) || (SmmuInfo->StreamTable == NULL) || (NewL2Consumed == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // Assume no split until we actually publish NewL2 into the L1 descriptor.
  *NewL2Consumed = FALSE;

  // Linear stream-table mode: no L1 indirection to split.
  if (SmmuInfo->SharedAbortL2 == NULL) {
    return EFI_SUCCESS;
  }

  L1Index      = StreamId >> SMMUV3_STR_TAB_BASE_CFG_SPLIT;
  BaseStreamId = L1Index << SMMUV3_STR_TAB_BASE_CFG_SPLIT;

  L1Table = (SMMUV3_L1_STREAM_TABLE_DESCRIPTOR *)SmmuInfo->StreamTable;
  L1Desc  = &L1Table[L1Index];

  // If this L1 descriptor has already been split onto a private L2 page
  // (i.e. it no longer points at the shared-ABORT L2), there is nothing
  // more to do: the caller can safely write its STE in place because
  // sibling slots in this L2 either still hold the abort STE or are
  // already-promoted translating STEs that belong to this same caller's
  // sequence.
  SharedAbortL2Encoded = ((UINT64)(UINTN)SmmuInfo->SharedAbortL2) >> SMMUV3_STR_TAB_BASE_L2_PTR_OFFSET;
  if (L1Desc->Bits.L2Ptr != SharedAbortL2Encoded) {
    return EFI_SUCCESS;
  }

  // Seed the new L2 from the shared-ABORT L2 so every unpromoted slot in
  // this L1 range starts out faulting.
  CopyMem (NewL2, SmmuInfo->SharedAbortL2, EFI_PAGE_SIZE);

  // Ensure all STE writes in the new L2 page are visible to the SMMU
  ArmDataSynchronizationBarrier ();

  // Atomically swing the L1 descriptor onto the private L2
  // so the SMMU cannot observe a torn L1STD.
  NewDesc.AsUINT64   = 0;
  NewDesc.Bits.L2Ptr = (UINT64)(UINTN)NewL2 >> SMMUV3_STR_TAB_BASE_L2_PTR_OFFSET;
  NewDesc.Bits.Span  = SMMUV3_STR_TAB_BASE_CFG_SPLIT + 1;

  L1Desc->AsUINT64 = NewDesc.AsUINT64;

  *NewL2Consumed = TRUE;

  // Ensure the L1STD write is visible to the SMMU before issuing the CFGI_STE_RANGE command.
  ArmDataSynchronizationBarrier ();

  //
  // The old L1STD was the shared-ABORT entry with full Span = SPLIT+1, so
  // its L2 page and STEs could already be cached. Invalidate the full
  // L1 range (2^SPLIT STEs anchored at BaseStreamId) using CFGI_STE_RANGE.
  // CFGI_STE_RANGE invalidates 2^(Range+1) STEs, so Range = SPLIT - 1.
  SMMUV3_BUILD_CMD_CFGI_STE_RANGE (&Command, BaseStreamId, SMMUV3_STR_TAB_BASE_CFG_SPLIT - 1);
  Status = SmmuV3SendCommand (SmmuInfo, &Command);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: CFGI_STE_RANGE failed: %r\n", __func__, Status));
    return Status;
  }

  SMMUV3_BUILD_CMD_SYNC_NO_INTERRUPT (&Command);
  Status = SmmuV3SendCommand (SmmuInfo, &Command);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: CMD_SYNC failed: %r\n", __func__, Status));
    return Status;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: Split L1[0x%x] on SmmuBase=0x%llx for StreamId=0x%x (range base 0x%x, %u entries) onto private L2 0x%p\n",
    __func__,
    L1Index,
    SmmuInfo->SmmuBase,
    StreamId,
    BaseStreamId,
    (1 << SMMUV3_STR_TAB_BASE_CFG_SPLIT),
    NewL2
    ));

  return EFI_SUCCESS;
}

/**
  Locate the STE slot in the SMMU's stream table for a given StreamID.

  Supports both linear and 2-level stream tables. For 2-level tables, walks
  the L1 descriptor array by (StreamId >> SPLIT), follows the L2Ptr, then
  indexes the L2 table by (StreamId & ((1 << SPLIT) - 1)). The L2 table is
  shared across all L1 entries (allocated once in SmmuV3Configure) so any
  StreamID within range resolves to a real slot.

  @param [in]  SmmuInfo  Pointer to the SMMU_INFO structure.
  @param [in]  StreamId  The StreamID.

  @retval Pointer to the STE slot, or NULL on out-of-range.
**/
SMMUV3_STREAM_TABLE_ENTRY *
SmmuV3GetSteSlot (
  IN SMMU_INFO  *SmmuInfo,
  IN UINT32     StreamId
  )
{
  BOOLEAN                            TwoLevel;
  SMMUV3_L1_STREAM_TABLE_DESCRIPTOR  *L1Table;
  SMMUV3_L1_STREAM_TABLE_DESCRIPTOR  *L1Desc;
  SMMUV3_STREAM_TABLE_ENTRY          *L2Table;
  UINT32                             L1Index;
  UINT32                             L2Index;
  UINT32                             L2EntriesPerTable;

  if ((SmmuInfo == NULL) || (SmmuInfo->StreamTable == NULL)) {
    return NULL;
  }

  if (StreamId > SmmuInfo->StreamTableEntryMax) {
    DEBUG ((DEBUG_ERROR, "%a: StreamId 0x%x out of range (max 0x%x)\n", __func__, StreamId, SmmuInfo->StreamTableEntryMax));
    return NULL;
  }

  TwoLevel = (SmmuInfo->StreamTableEntryMax >= (EFI_PAGE_SIZE / sizeof (SMMUV3_STREAM_TABLE_ENTRY)));
  if (!SmmuInfo->TwoLevelStreamTableSupported) {
    TwoLevel = FALSE;
    DEBUG ((DEBUG_VERBOSE, "%a: SMMU does not support 2-level stream tables. Falling back to linear stream table.\n", __func__));
  }

  if (!TwoLevel) {
    return &((SMMUV3_STREAM_TABLE_ENTRY *)SmmuInfo->StreamTable)[StreamId];
  }

  // 2-level: L1 index = top bits above SPLIT, L2 index = low SPLIT bits.
  L2EntriesPerTable = 1u << SMMUV3_STR_TAB_BASE_CFG_SPLIT;
  L1Index           = StreamId >> SMMUV3_STR_TAB_BASE_CFG_SPLIT;
  L2Index           = StreamId & (L2EntriesPerTable - 1);

  L1Table = (SMMUV3_L1_STREAM_TABLE_DESCRIPTOR *)SmmuInfo->StreamTable;
  L1Desc  = &L1Table[L1Index];
  if (L1Desc->Bits.L2Ptr == 0) {
    DEBUG ((DEBUG_ERROR, "%a: L1[%u] has no L2 table for StreamId 0x%x\n", __func__, L1Index, StreamId));
    return NULL;
  }

  L2Table = (SMMUV3_STREAM_TABLE_ENTRY *)(UINTN)((UINT64)L1Desc->Bits.L2Ptr << SMMUV3_STR_TAB_BASE_L2_PTR_OFFSET);
  return &L2Table[L2Index];
}

/**
  Promote the STE for StreamId from Invalid to Valid with the given
  page-table root, using the SMMU break-before-make sequence required by the
  SMMUv3 spec for STE Config changes:

    1. Write the STE with V=0.
    2. DSB + CFGI_STE(StreamId) + CMD_SYNC.
    3. Write the full new STE contents (S2Ttb etc., Config=S2_TRANSLATE, V=1).
    4. DSB + CFGI_STE(StreamId) + CMD_SYNC.

  @param [in]  SmmuInfo        Pointer to the SMMU_INFO structure.
  @param [in]  StreamId        The StreamID whose STE is being promoted.
  @param [in]  Vmid            VMID tag to install in the STE's S2VMID field.
  @param [in]  PageTableRoot   Page-table root to install in the STE.
  @param [in]  NewL2           Caller-provided L2 page used by
                               SmmuV3SplitL1IfShared() when the covering L1
                               descriptor still points at the shared-ABORT
                               L2. Because the caller owns the PageTableRoot and NewL2,
                               this function does not allocate or free them.
  @param [out] NewL2Consumed   Set to TRUE if NewL2 was installed into an L1
                               descriptor by the split step (the caller must
                               NOT free it), FALSE otherwise (the caller
                               should free NewL2).

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameters.
  @retval Other                  Command-queue / sync failure.
**/
EFI_STATUS
SmmuV3PromoteSteToTranslate (
  IN  SMMU_INFO                  *SmmuInfo,
  IN  UINT32                     StreamId,
  IN  UINT16                     Vmid,
  IN  PAGE_TABLE                 *PageTableRoot,
  IN  SMMUV3_STREAM_TABLE_ENTRY  *NewL2,
  OUT BOOLEAN                    *NewL2Consumed
  )
{
  EFI_STATUS                 Status;
  SMMUV3_STREAM_TABLE_ENTRY  *SteSlot;
  SMMUV3_STREAM_TABLE_ENTRY  NewEntry;
  SMMUV3_CMD_GENERIC         Command;
  UINTN                      Index;

  if ((SmmuInfo == NULL) || (PageTableRoot == NULL) || (NewL2 == NULL) || (NewL2Consumed == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // Assume NewL2 is not consumed until SplitL1IfShared reports otherwise.
  *NewL2Consumed = FALSE;

  // 2-level only: if this L1 index still points at the shared-ABORT L2,
  // copy-on-write it onto a private L2 page before mutating any STE. This
  // is what prevents StreamID collisions across L1 indices that originally
  // shared one L2. For linear stream tables this is a no-op (SharedAbortL2
  // is NULL) and NewL2 stays unconsumed.
  if (SmmuInfo->SharedAbortL2 != NULL) {
    Status = SmmuV3SplitL1IfShared (SmmuInfo, StreamId, NewL2, NewL2Consumed);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: SplitL1IfShared failed for StreamId 0x%x: %r\n", __func__, StreamId, Status));
      return Status;
    }
  }

  SteSlot = SmmuV3GetSteSlot (SmmuInfo, StreamId);
  if (SteSlot == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Build the full STAGE_2_TRANSLATE STE (V=1, Config=S2_TRANSLATE,
  // S2Ttb, attrs, etc.) into a local. We then publish it into the live
  // slot following the invalid -> valid sequence from the SMMU spec.
  //
  // The init-time STE template installed by SmmuV3Configure has Valid=0,
  // so every promotion is an invalid -> valid transition.
  Status = SmmuV3BuildTranslateStreamTableEntry (SmmuInfo, PageTableRoot, Vmid, &NewEntry);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Build translate STE failed for StreamId 0x%x: %r\n", __func__, StreamId, Status));
    return Status;
  }

  //
  // 1. Write all STE UINT64's except Index 0 (which holds Valid + Config).
  //
  for (Index = 1; Index < (sizeof (SMMUV3_STREAM_TABLE_ENTRY) / sizeof (UINT64)); Index++) {
    SteSlot->AsUINT64[Index] = NewEntry.AsUINT64[Index];
  }

  //
  // 2. DSB so the SteSlot[1..7] writes are observable, then CFGI_STE + SYNC
  //    so the SMMU drops any cached STE state derived from the old
  //    contents before we publish Valid=1.
  //
  ArmDataSynchronizationBarrier ();

  SMMUV3_BUILD_CMD_CFGI_STE (&Command, StreamId, 1);     // Leaf = 1
  Status = SmmuV3SendCommand (SmmuInfo, &Command);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: CFGI_STE (pre-valid) failed for StreamId 0x%x: %r\n", __func__, StreamId, Status));
    return Status;
  }

  SMMUV3_BUILD_CMD_SYNC_NO_INTERRUPT (&Command);
  Status = SmmuV3SendCommand (SmmuInfo, &Command);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: CMD_SYNC (pre-valid) failed for StreamId 0x%x: %r\n", __func__, StreamId, Status));
    return Status;
  }

  //
  // 3. Publish SteSlot[0] (Valid + Config) last with a single
  //    atomic 64-bit write.
  //
  SteSlot->AsUINT64[0] = NewEntry.AsUINT64[0];

  //
  // 4. Final DSB + CFGI_STE + SYNC so the SMMU re-fetches the now-valid
  //    STE and picks up Config=S2_TRANSLATE for this StreamID.
  //
  ArmDataSynchronizationBarrier ();

  SMMUV3_BUILD_CMD_CFGI_STE (&Command, StreamId, 1);     // Leaf = 1
  Status = SmmuV3SendCommand (SmmuInfo, &Command);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: CFGI_STE (post-valid) failed for StreamId 0x%x: %r\n", __func__, StreamId, Status));
    return Status;
  }

  SMMUV3_BUILD_CMD_SYNC_NO_INTERRUPT (&Command);
  Status = SmmuV3SendCommand (SmmuInfo, &Command);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: CMD_SYNC (post-valid) failed for StreamId 0x%x: %r\n", __func__, StreamId, Status));
    return Status;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: Promoted STE StreamId=0x%x VMID=0x%x on SmmuBase=0x%llx to STAGE_2_TRANSLATE, root=0x%p\n",
    __func__,
    StreamId,
    Vmid,
    SmmuInfo->SmmuBase,
    PageTableRoot
    ));

  return EFI_SUCCESS;
}

/**
  Allocate a linear or 2-Level stream table for SMMUv3.

  For allocating a 2-level or linear stream table, the stream table alignment
  requirements per SMMUv3 spec:
  - For 2-level table, the table needs to be aligned to the larger of L1
    table size or 64 bytes.
  - For linear table, the table needs to be aligned to its size.

  @param [in]  SmmuInfo             Pointer to the SMMU_INFO structure.
  @param [in]  TwoLevelStreamTable  Flag to indicate if a two-level stream table is used.
  @param [out] Log2Size             Pointer to store the log2 size of the stream table.
  @param [out] Size                 Pointer to store the size of the stream table.

  @retval Pointer to the allocated stream table, or NULL on failure.
**/
STATIC
VOID *
SmmuV3AllocateStreamTable (
  IN SMMU_INFO  *SmmuInfo,
  IN BOOLEAN    TwoLevelStreamTable,
  OUT UINT32    *Log2Size,
  OUT UINT32    *Size
  )
{
  UINT32  MaxStreamId;
  UINT32  SidMsb;
  UINT32  L1Bits;
  UINT32  Alignment;
  UINTN   Pages;
  VOID    *AllocatedAddress;

  if ((SmmuInfo == NULL) || (Log2Size == NULL) || (Size == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Parameters\n", __func__));
    return NULL;
  }

  // The max stream id is calculated as the output base + the number of stream ids
  MaxStreamId = SmmuInfo->StreamTableEntryMax;
  if (TwoLevelStreamTable && (MaxStreamId < (EFI_PAGE_SIZE / sizeof (SMMUV3_STREAM_TABLE_ENTRY)))) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid MaxStreamId for 2-Level table%u\n", __func__, MaxStreamId));
    return NULL;
  }

  SidMsb    = HighBitSet32 (MaxStreamId);
  *Log2Size = SidMsb + 1;
  *Size     = SMMUV3_LINEAR_STREAM_TABLE_SIZE_FROM_LOG2 (*Log2Size);
  if (TwoLevelStreamTable) {
    L1Bits = *Log2Size - SMMUV3_STR_TAB_BASE_CFG_SPLIT; // L1 table log2 size
    *Size  = SMMUV3_L1_STREAM_TABLE_SIZE_FROM_LOG2 (L1Bits);
  }

  *Size            = ALIGN_VALUE (*Size, EFI_PAGE_SIZE);
  Alignment        = *Size; // Aligned to the size of the table, linear stream table
  Pages            = EFI_SIZE_TO_PAGES (*Size);
  AllocatedAddress = AllocateAlignedPages (Pages, Alignment);
  if (AllocatedAddress == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Allocation failed for stream table\n", __func__));
    return NULL;
  }

  ZeroMem (AllocatedAddress, *Size);
  return AllocatedAddress;
}

/**
  Free the allocated stream table for SMMUv3.

  @param [in] StreamTablePtr  Pointer to the stream table entry.
  @param [in] Size            Size of the stream table.
**/
STATIC
VOID
SmmuV3FreeStreamTable (
  IN VOID    *StreamTablePtr,
  IN UINT32  Size
  )
{
  UINTN  Pages;

  if ((StreamTablePtr == NULL) || (Size == 0)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Parameters\n", __func__));
    return;
  }

  Pages = EFI_SIZE_TO_PAGES (Size);

  FreeAlignedPages ((VOID *)StreamTablePtr, Pages);
}

/**
  Allocate the stream table for the SMMU and populate every STE slot with
  the init-time "abort-equivalent" STE template. For 2-level stream tables
  this also allocates the shared-ABORT L2 page and points every L1
  descriptor at it.

  @param [in]  SmmuInfo              Pointer to the SMMU_INFO structure.
  @param [out] TwoLevelStreamTable   Set to TRUE if a 2-level stream table
                                     was allocated, FALSE for linear.

  @retval EFI_SUCCESS            Success.
  @retval EFI_OUT_OF_RESOURCES   Allocation failure.
  @retval Other                  Failure building the STE template.
**/
STATIC
EFI_STATUS
SmmuV3InitStreamTable (
  IN  SMMU_INFO  *SmmuInfo,
  OUT BOOLEAN    *TwoLevelStreamTable
  )
{
  EFI_STATUS                         Status;
  UINT32                             Index;
  SMMUV3_STREAM_TABLE_ENTRY          *StreamTableEntryPtr;
  SMMUV3_STREAM_TABLE_ENTRY          *L2StreamTablePtr;
  SMMUV3_L1_STREAM_TABLE_DESCRIPTOR  *L1Table;
  SMMUV3_STREAM_TABLE_ENTRY          TemplateEntry;

  *TwoLevelStreamTable = (SmmuInfo->StreamTableEntryMax >= (EFI_PAGE_SIZE / sizeof (SMMUV3_STREAM_TABLE_ENTRY)));
  if (!SmmuInfo->TwoLevelStreamTableSupported) {
    *TwoLevelStreamTable = FALSE;
    DEBUG ((DEBUG_INFO, "%a: SMMU does not support 2-level stream tables. Falling back to linear stream table.\n", __func__));
  }

  SmmuInfo->StreamTable = SmmuV3AllocateStreamTable (SmmuInfo, *TwoLevelStreamTable, &SmmuInfo->StreamTableLog2Size, &SmmuInfo->StreamTableSize);
  if (SmmuInfo->StreamTable == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Error allocating stream table\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  // Build the init-time STE template. This is a STAGE_2_TRANSLATE STE with
  // S2Ttb = 0 (no page-table root yet) and Valid = 0, so any DMA from a non-promoted
  // StreamID will trigger a SMMU fault and be recorded in
  // the event queue. The first IoMmu Map/SetAttribute for a StreamID
  // publishes a real S2Ttb in-place via SmmuV3PromoteSteToTranslate().
  Status = SmmuV3BuildInvalidStreamTableEntry (SmmuInfo, &TemplateEntry);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Error building init STE template\n", __func__));
    return Status;
  }

  if (*TwoLevelStreamTable) {
    L2StreamTablePtr = (SMMUV3_STREAM_TABLE_ENTRY *)AllocatePages (1);
    if (L2StreamTablePtr == NULL) {
      DEBUG ((DEBUG_ERROR, "%a: Error allocating L2 stream table\n", __func__));
      return EFI_OUT_OF_RESOURCES;
    }

    ZeroMem (L2StreamTablePtr, EFI_PAGE_SIZE);

    for (Index = 0; Index < (EFI_PAGE_SIZE / sizeof (SMMUV3_STREAM_TABLE_ENTRY)); Index++) {
      CopyMem (&L2StreamTablePtr[Index], &TemplateEntry, sizeof (SMMUV3_STREAM_TABLE_ENTRY));
    }

    // Remember this shared-ABORT L2. Every L1 descriptor initially points
    // here. The first IoMmu promotion that targets an L1 index whose L2Ptr
    // still equals SharedAbortL2 will copy-on-write a private L2 page so
    // distinct StreamIDs that happen to share L1 bits (same StreamId >>
    // SPLIT) don't alias to the same STE slot.
    SmmuInfo->SharedAbortL2 = L2StreamTablePtr;

    L1Table = (SMMUV3_L1_STREAM_TABLE_DESCRIPTOR *)SmmuInfo->StreamTable;
    for (Index = 0; Index < (SMMUV3_L1_STREAM_TABLE_SIZE_FROM_LOG2 (SmmuInfo->StreamTableLog2Size - SMMUV3_STR_TAB_BASE_CFG_SPLIT) / sizeof (UINT64)); Index++) {
      L1Table[Index].Bits.L2Ptr = (UINT64)(UINTN)L2StreamTablePtr >> SMMUV3_STR_TAB_BASE_L2_PTR_OFFSET;
      // Per SmmuV3 spec: Span must be within the range of 0 to (SMMU_STRTAB_BASE_CFG.SPLIT + 1)
      // That is it must stay within the bounds of the Stream table split point.
      // Cannot have Span of 0, means invalid L2 table ptr in the L1 table entry.
      L1Table[Index].Bits.Span = SMMUV3_STR_TAB_BASE_CFG_SPLIT + 1;
    }
  } else {
    StreamTableEntryPtr = (SMMUV3_STREAM_TABLE_ENTRY *)SmmuInfo->StreamTable;
    for (Index = 0; Index <= SmmuInfo->StreamTableEntryMax; Index++) {
      CopyMem (&StreamTableEntryPtr[Index], &TemplateEntry, sizeof (SMMUV3_STREAM_TABLE_ENTRY));
    }
  }

  return EFI_SUCCESS;
}

/**
  Program the Stream Table, Command Queue and Event Queue base registers.

  @param [in]  SmmuInfo                 Pointer to the SMMU_INFO structure.
  @param [in]  TwoLevelStreamTable      TRUE if a 2-level stream table is used.
**/
STATIC
VOID
SmmuV3ProgramBaseRegisters (
  IN SMMU_INFO  *SmmuInfo,
  IN BOOLEAN    TwoLevelStreamTable
  )
{
  SMMUV3_STRTAB_BASE      StrTabBase;
  SMMUV3_STRTAB_BASE_CFG  StrTabBaseCfg;
  SMMUV3_CMDQ_BASE        CommandQueueBase;
  SMMUV3_EVENTQ_BASE      EventQueueBase;

  // Configure Stream Table Base
  StrTabBaseCfg.AsUINT32 = 0;
  StrTabBaseCfg.Bits.Fmt = SMMUV3_STR_TAB_BASE_CFG_FMT_LINEAR;
  if (TwoLevelStreamTable) {
    StrTabBaseCfg.Bits.Fmt   = SMMUV3_STR_TAB_BASE_CFG_FMT_2LEVEL;
    StrTabBaseCfg.Bits.Split = SMMUV3_STR_TAB_BASE_CFG_SPLIT;
  }

  StrTabBaseCfg.Bits.Log2Size = SmmuInfo->StreamTableLog2Size;

  SmmuV3WriteRegister32 (SmmuInfo->SmmuBase, SMMU_STRTAB_BASE_CFG, StrTabBaseCfg.AsUINT32);

  StrTabBase.AsUINT64  = 0;
  StrTabBase.Bits.Ra   = 1;
  StrTabBase.Bits.Addr = ((UINT64)(UINTN)SmmuInfo->StreamTable) >> SMMUV3_STR_TAB_BASE_ADDR_OFFSET;
  SmmuV3WriteRegister64 (SmmuInfo->SmmuBase, SMMU_STRTAB_BASE, StrTabBase.AsUINT64);

  // Configure Command Queue Base
  CommandQueueBase.AsUINT64      = 0;
  CommandQueueBase.Bits.Log2Size = SmmuInfo->CommandQueueLog2Size;
  CommandQueueBase.Bits.Addr     = ((UINT64)(UINTN)SmmuInfo->CommandQueue) >> SMMUV3_STR_TAB_BASE_CMDQ_OFFSET;
  CommandQueueBase.Bits.Ra       = 1;
  SmmuV3WriteRegister64 (SmmuInfo->SmmuBase, SMMU_CMDQ_BASE, CommandQueueBase.AsUINT64);
  SmmuV3WriteRegister32 (SmmuInfo->SmmuBase, SMMU_CMDQ_PROD, 0);
  SmmuV3WriteRegister32 (SmmuInfo->SmmuBase, SMMU_CMDQ_CONS, 0);
  SmmuInfo->CachedConsumer = 0;
  SmmuInfo->CachedProducer = 0;

  // Configure Event Queue Base
  EventQueueBase.AsUINT64      = 0;
  EventQueueBase.Bits.Log2Size = SmmuInfo->EventQueueLog2Size;
  EventQueueBase.Bits.Addr     = ((UINT64)(UINTN)SmmuInfo->EventQueue) >> SMMUV3_STR_TAB_BASE_EVENTQ_OFFSET;
  EventQueueBase.Bits.Wa       = 1;
  SmmuV3WriteRegister64 (SmmuInfo->SmmuBase, SMMU_EVENTQ_BASE, EventQueueBase.AsUINT64);
  SmmuV3WriteRegister32 (SmmuInfo->SmmuBase + SMMUV3_PAGE_1_OFFSET, SMMU_EVENTQ_PROD, 0);
  SmmuV3WriteRegister32 (SmmuInfo->SmmuBase + SMMUV3_PAGE_1_OFFSET, SMMU_EVENTQ_CONS, 0);
}

/**
  Configure the SMMU CR1 and CR2 control registers.

  @param [in]  SmmuInfo  Pointer to the SMMU_INFO structure.
**/
STATIC
VOID
SmmuV3ConfigureControlRegisters (
  IN SMMU_INFO  *SmmuInfo
  )
{
  SMMUV3_CR1   Cr1;
  SMMUV3_CR2   Cr2;
  SMMUV3_IDR0  Idr0;

  // Configure CR1
  Cr1.AsUINT32     = SmmuV3ReadRegister32 (SmmuInfo->SmmuBase, SMMU_CR1);
  Cr1.AsUINT32    &= ~SMMUV3_CR1_VALID_MASK;
  Cr1.Bits.QueueIc = ARM64_RGNCACHEATTR_WRITEBACK_WRITEALLOCATE;
  Cr1.Bits.QueueOc = ARM64_RGNCACHEATTR_WRITEBACK_WRITEALLOCATE;
  Cr1.Bits.QueueSh = ARM64_SHATTR_INNER_SHAREABLE;

  Cr1.Bits.TableIc = ARM64_RGNCACHEATTR_WRITEBACK_WRITEALLOCATE;
  Cr1.Bits.TableOc = ARM64_RGNCACHEATTR_WRITEBACK_WRITEALLOCATE;
  Cr1.Bits.TableSh = ARM64_SHATTR_INNER_SHAREABLE;

  SmmuV3WriteRegister32 (SmmuInfo->SmmuBase, SMMU_CR1, Cr1.AsUINT32);

  // Configure CR2
  Cr2.AsUINT32  = SmmuV3ReadRegister32 (SmmuInfo->SmmuBase, SMMU_CR2);
  Cr2.AsUINT32 &= ~SMMUV3_CR2_VALID_MASK;
  // Set E2h to match HCR_EL2.E2H in the host PE
  Cr2.Bits.E2h       = (ArmReadHcr () & ARM_HCR_E2H) ? 1 : 0;
  Cr2.Bits.RecInvSid = SMMUV3_CR2_REC_INV_SID;   // Record C_BAD_STREAMID for invalid input streams.

  //
  // If broadcast TLB maintenance (BTM) is not enabled, then configure
  // private TLB maintenance (PTM). Per SMMU spec (section 6.3.12), the PTM bit is
  // only valid when BTM is indicated as supported.
  //
  Idr0.AsUINT32 = SmmuV3ReadRegister32 (SmmuInfo->SmmuBase, SMMU_IDR0);
  if (Idr0.Bits.Btm == 1) {
    Cr2.Bits.Ptm = SMMUV3_CR2_PTM;     // Private TLB maintenance.
  }

  SmmuV3WriteRegister32 (SmmuInfo->SmmuBase, SMMU_CR2, Cr2.AsUINT32);
}

/**
  Enable the SMMU event and command queues (CR0 part 1) and then invalidate
  all cached configuration and TLB entries.

  @param [in]  SmmuInfo  Pointer to the SMMU_INFO structure.

  @retval EFI_SUCCESS  Success.
  @retval Other        Command-queue / poll failure.
**/
STATIC
EFI_STATUS
SmmuV3EnableQueuesAndInvalidate (
  IN SMMU_INFO  *SmmuInfo
  )
{
  EFI_STATUS          Status;
  SMMUV3_CR0          Cr0;
  SMMUV3_CMD_GENERIC  Command;

  // Issue a DSB to ensure that all previous writes are observable to the SMMU before configuring CR0.
  ArmDataSynchronizationBarrier ();

  // Configure CR0 part1
  Cr0.AsUINT32      = SmmuV3ReadRegister32 (SmmuInfo->SmmuBase, SMMU_CR0);
  Cr0.Bits.EventQEn = SMMUV3_CR0_EVENTQ_EN;
  Cr0.Bits.CmdQEn   = SMMUV3_CR0_CMDQ_EN;

  SmmuV3WriteRegister32 (SmmuInfo->SmmuBase, SMMU_CR0, Cr0.AsUINT32);
  Status = SmmuV3Poll (SmmuInfo->SmmuBase, SMMU_CR0ACK, 0xC, 0xC);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Error polling register: 0x%lx\n", __func__, SmmuInfo->SmmuBase + SMMU_CR0ACK));
    return Status;
  }

  //
  // Invalidate all cached configuration and TLB entries
  //
  SMMUV3_BUILD_CMD_CFGI_ALL (&Command);
  Status = SmmuV3SendCommand (SmmuInfo, &Command);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Error sending command.\n", __func__));
    return Status;
  }

  SMMUV3_BUILD_CMD_TLBI_NSNH_ALL (&Command);
  Status = SmmuV3SendCommand (SmmuInfo, &Command);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Error sending command.\n", __func__));
    return Status;
  }

  SMMUV3_BUILD_CMD_TLBI_EL2_ALL (&Command);
  Status = SmmuV3SendCommand (SmmuInfo, &Command);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Error sending command.\n", __func__));
    return Status;
  }

  // Issue a CMD_SYNC command to guarantee that any previously issued TLB
  // invalidations (CMD_TLBI_*) are completed (SMMUv3.2 spec section 4.6.3).
  SMMUV3_BUILD_CMD_SYNC_NO_INTERRUPT (&Command);
  Status = SmmuV3SendCommand (SmmuInfo, &Command);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Error sending command.\n", __func__));
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Enable SMMU translation (CR0 part 2) and check for any global errors
  reported after enablement.

  @param [in]  SmmuInfo  Pointer to the SMMU_INFO structure.

  @retval EFI_SUCCESS       Success.
  @retval EFI_DEVICE_ERROR  Global SMMU error reported.
  @retval Other             Poll failure.
**/
STATIC
EFI_STATUS
SmmuV3EnableSmmuTranslation (
  IN SMMU_INFO  *SmmuInfo
  )
{
  EFI_STATUS     Status;
  SMMUV3_CR0     Cr0;
  SMMUV3_IDR0    Idr0;
  SMMUV3_GERROR  GError;

  // Configure CR0 part2
  Cr0.AsUINT32 = SmmuV3ReadRegister32 (SmmuInfo->SmmuBase, SMMU_CR0);

  // Issue a DSB to ensure that all previous writes are observable to the SMMU before configuring CR0.
  ArmDataSynchronizationBarrier ();

  Cr0.AsUINT32      = Cr0.AsUINT32 & ~SMMUV3_CR0_VALID_MASK;
  Cr0.Bits.SmmuEn   = SMMUV3_CR0_SMMU_EN;
  Cr0.Bits.EventQEn = SMMUV3_CR0_EVENTQ_EN;
  Cr0.Bits.CmdQEn   = SMMUV3_CR0_CMDQ_EN;
  Cr0.Bits.PriQEn   = SMMUV3_CR0_PRIQ_EN_DISABLED;
  Cr0.Bits.Vmw      = SMMUV3_CR0_VMW_DISABLED;  // Disable VMID wildcard matching.
  Idr0.AsUINT32     = SmmuV3ReadRegister32 (SmmuInfo->SmmuBase, SMMU_IDR0);
  if (Idr0.Bits.Ats != 0) {
    Cr0.Bits.AtsChk = SMMUV3_CR0_ATS_CHK_DISABLE; // Disable bypass for ATS translated traffic.
  }

  SmmuV3WriteRegister32 (SmmuInfo->SmmuBase, SMMU_CR0, Cr0.AsUINT32);
  Status = SmmuV3Poll (SmmuInfo->SmmuBase, SMMU_CR0ACK, SMMUV3_CR0_SMMU_EN_MASK, SMMUV3_CR0_SMMU_EN_MASK);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Error polling register: 0x%lx\n", __func__, SmmuInfo->SmmuBase + SMMU_CR0ACK));
    return Status;
  }

  // Issue a DSB to ensure that all previous writes are observable to the SMMU.
  ArmDataSynchronizationBarrier ();

  GError.AsUINT32 = SmmuV3ReadRegister32 (SmmuInfo->SmmuBase, SMMU_GERROR);
  if (GError.AsUINT32 != 0) {
    DEBUG ((DEBUG_ERROR, "%a: Global SMMU Error detected: 0x%lx\n", __func__, GError.AsUINT32));
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Configure the SMMUv3 based on the provided configuration per the SmmuV3 specification.
  Main configuration function for smmu hardware. Creates and enables a stream table, page table,
  event queue, and command queue. Enables stage 2 translation and dma remapping.

  <https://developer.arm.com/documentation/109242/0100/Programming-the-SMMU/Minimum-configuration>
  <https://developer.arm.com/documentation/ihi0070/latest/>

  At init time every STE is built in "abort-equivalent" mode (S2 translate
  with S2Ttb=0), so no global page-table root is needed; per-StreamID roots
  are allocated lazily on the first IoMmu map.

  @param [in] SmmuInfo        Pointer to the SMMU_INFO structure.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES   Out of resources.
  @retval EFI_TIMEOUT            Timeout.
  @retval EFI_DEVICE_ERROR       Device error.
  @retval Others                 Failure.
**/
STATIC
EFI_STATUS
SmmuV3Configure (
  IN SMMU_INFO  *SmmuInfo
  )
{
  EFI_STATUS   Status;
  UINT32       CommandQueueLog2Size;
  UINT32       EventQueueLog2Size;
  SMMUV3_IDR0  Idr0;
  SMMUV3_IDR3  Idr3;
  VOID         *CommandQueue;
  VOID         *EventQueue;
  BOOLEAN      TwoLevelStreamTable;

  if (SmmuInfo == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Parameters\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  Idr0.AsUINT32 = SmmuV3ReadRegister32 (SmmuInfo->SmmuBase, SMMU_IDR0);
  // This implementation only supports cache coherent SMMUs
  if (Idr0.Bits.Cohacc == 0) {
    DEBUG ((DEBUG_ERROR, "%a: Non-coherent access to translation tables not supported.\n", __func__));
    return EFI_UNSUPPORTED;
  }

  // Check for Stage 2 translation support
  if (Idr0.Bits.S2p == 0) {
    DEBUG ((DEBUG_ERROR, "%a: SMMU does not support stage 2 translation.\n", __func__));
    return EFI_UNSUPPORTED;
  }

  // Check for 2-level stream table support.
  SmmuInfo->TwoLevelStreamTableSupported = (Idr0.Bits.StLevel != 0);

  // Cache VMID width and seed the per-stream VMID allocator. VMID 0 is
  // reserved as "unassigned" so the allocator starts at 1.
  SmmuInfo->Vmid16Supported = (Idr0.Bits.Vmid16 != 0);
  SmmuInfo->NextVmid        = 1;

  // Disable SMMU before configuring
  Status = SmmuV3DisableTranslation (SmmuInfo->SmmuBase);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Error disabling translation\n", __func__));
    goto End;
  }

  Status = SmmuV3DisableInterrupts (SmmuInfo->SmmuBase, TRUE);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Error disabling interrupts\n", __func__));
    goto End;
  }

  Status = SmmuV3InitStreamTable (SmmuInfo, &TwoLevelStreamTable);
  if (EFI_ERROR (Status)) {
    goto End;
  }

  Status = SmmuV3AllocateCommandQueue (SmmuInfo, &CommandQueueLog2Size, &CommandQueue);
  if (EFI_ERROR (Status) || (CommandQueue == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Error allocating SMMU Command Queue\n", __func__));
    goto End;
  }

  Status = SmmuV3AllocateEventQueue (SmmuInfo, &EventQueueLog2Size, &EventQueue);
  if (EFI_ERROR (Status) || (EventQueue == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Error allocating SMMU Event Queue\n", __func__));
    goto End;
  }

  SmmuInfo->CommandQueue         = CommandQueue;
  SmmuInfo->CommandQueueLog2Size = CommandQueueLog2Size;
  SmmuInfo->EventQueue           = EventQueue;
  SmmuInfo->EventQueueLog2Size   = EventQueueLog2Size;

  SmmuV3ProgramBaseRegisters (SmmuInfo, TwoLevelStreamTable);

  // Register EVTQ + GERR ISRs with the GIC so SMMU faults are surfaced.
  if (mGicInterrupt != NULL) {
    Status = SmmuV3RegisterGicIsr (mGicInterrupt, SmmuInfo);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Error registering SMMU GIC ISR\n", __func__));
      goto End;
    }

    DEBUG ((DEBUG_INFO, "%a: Registered SMMU GIC ISR for SmmuBase=0x%llx\n", __func__, SmmuInfo->SmmuBase));
  } else {
    DEBUG ((DEBUG_ERROR, "%a: SMMU GIC ISR for SmmuBase=0x%llx not registered.\n", __func__, SmmuInfo->SmmuBase));
  }

  // Check if Range-based invalidation and level hint are supported.
  Idr3.AsUINT32                        = SmmuV3ReadRegister32 (SmmuInfo->SmmuBase, SMMU_IDR3);
  SmmuInfo->RangeInvalidationSupported = (Idr3.Bits.Ril != 0);

  // Enable GError and event interrupts
  Status = SmmuV3EnableInterrupts (SmmuInfo->SmmuBase);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Error enabling interrupts\n", __func__));
    goto End;
  }

  SmmuV3ConfigureControlRegisters (SmmuInfo);

  Status = SmmuV3EnableQueuesAndInvalidate (SmmuInfo);
  if (EFI_ERROR (Status)) {
    goto End;
  }

  Status = SmmuV3EnableSmmuTranslation (SmmuInfo);
  if (EFI_ERROR (Status)) {
    goto End;
  }

  // Only logs errors if errors are found after SMMU is enabled.
  SmmuV3LogErrors (SmmuInfo);

End:
  return Status;
}

/**
  Retrieve the SMMU configuration data from the HOB.

  @return Pointer to the SMMU_CONFIG structure, or NULL if not found.
**/
STATIC
SMMU_CONFIG *
GetSmmuConfigHobData (
  VOID
  )
{
  VOID  *GuidHob;

  GuidHob = GetFirstGuidHob (&gSmmuConfigHobGuid);

  if (GuidHob != NULL) {
    return (SMMU_CONFIG *)GET_GUID_HOB_DATA (GuidHob);
  }

  return NULL;
}

/**
  Check if the SMMU_CONFIG structure is compatible with the current driver version.
  Backwards compatibility is currently not supported.

  @param [in] SmmuConfig  Pointer to the SMMU_CONFIG structure.

  @retval EFI_SUCCESS               Success.
  @retval EFI_INVALID_PARAMETER     Invalid parameter.
  @retval EFI_INCOMPATIBLE_VERSION  Incompatible version.
**/
STATIC
EFI_STATUS
CheckSmmuConfigStructure (
  IN SMMU_CONFIG  *SmmuConfig
  )
{
  if (SmmuConfig == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: SMMU_CONFIG structure is NULL\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if ((SmmuConfig->VersionMajor == CURRENT_SMMU_CONFIG_VERSION_MAJOR) && (SmmuConfig->VersionMinor == CURRENT_SMMU_CONFIG_VERSION_MINOR)) {
    return EFI_SUCCESS;
  }

  DEBUG ((
    DEBUG_ERROR,
    "%a: SMMU_CONFIG version mismatch. Expected: %u.%u Got: %u.%u\n",
    __func__,
    CURRENT_SMMU_CONFIG_VERSION_MAJOR,
    CURRENT_SMMU_CONFIG_VERSION_MINOR,
    SmmuConfig->VersionMajor,
    SmmuConfig->VersionMinor
    ));
  return EFI_INCOMPATIBLE_VERSION;
}

/**
  Initialize the IOMMU_CONFIG structure.

  @param [out]  IoMmu  Pointer to receive the allocated IOMMU_CONFIG structure.

  @retval EFI_SUCCESS           The IOMMU_CONFIG structure was allocated.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate the IOMMU_CONFIG structure.
**/
EFI_STATUS
IoMmuConfigInit (
  OUT IOMMU_CONFIG  **IoMmu
  )
{
  *IoMmu = (IOMMU_CONFIG *)AllocateZeroPool (sizeof (IOMMU_CONFIG));
  if (*IoMmu == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate IOMMU_CONFIG structure\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

/**
  Free every per-StreamID stage-2 page-table root reachable from this SMMU's
  stream table.

  The STEs are the single source of truth for what roots exist. Multiple
  StreamIDs can alias the same root (a device's secondary StreamIDs share
  its primary's S2Ttb / VMID), so this walks every STE, dedupes the S2Ttb
  addresses it finds, and frees each unique root exactly once via
  SmmuV3FreePageTableTree.

  If the dedupe-tracking buffer cannot be allocated, the function leaks the
  per-stream roots rather than risking a double free.

  Caller must have already disabled translation / driven the SMMU into
  ABORT so no in-flight DMA can still reference these roots.

  @param [in]  SmmuInfo  SMMU instance whose per-stream roots should be
                         freed. No-op if StreamTable is NULL.
**/
STATIC
VOID
SmmuV3FreePerStreamPageTableRoots (
  IN SMMU_INFO  *SmmuInfo
  )
{
  PAGE_TABLE                 **FreedRoots;
  UINTN                      MaxRoots;
  UINTN                      FreedCount;
  UINTN                      MaxStreamId;
  UINTN                      StreamId;
  UINTN                      DupIdx;
  SMMUV3_STREAM_TABLE_ENTRY  *Ste;
  PAGE_TABLE                 *Root;
  BOOLEAN                    IsDup;

  if ((SmmuInfo == NULL) || (SmmuInfo->StreamTable == NULL)) {
    return;
  }

  // Upper bound on unique roots = VMIDs handed out by the allocator
  // (each call hands out one VMID before allocating a fresh root). If
  // NextVmid wrapped to 0, every VMID in the configured width is in
  // use.
  MaxRoots = (SmmuInfo->NextVmid == SMMU_VMID_RESERVED)
             ? (SmmuInfo->Vmid16Supported ? MAX_UINT16 : MAX_UINT8)
             : (SmmuInfo->NextVmid - 1);
  FreedRoots = NULL;
  FreedCount = 0;
  if (MaxRoots > 0) {
    FreedRoots = (PAGE_TABLE **)AllocateZeroPool (MaxRoots * sizeof (PAGE_TABLE *));
  }

  // Walk every STE slot. SmmuV3GetSteSlot transparently handles both
  // linear and 2-level (including the shared-ABORT L2). If we cannot
  // allocate the dedupe buffer, leak rather than risk double-free.
  if ((MaxRoots == 0) || (FreedRoots != NULL)) {
    MaxStreamId = SmmuInfo->StreamTableEntryMax;
    for (StreamId = 0; StreamId <= MaxStreamId; StreamId++) {
      Ste = SmmuV3GetSteSlot (SmmuInfo, (UINT32)StreamId);
      if ((Ste == NULL) || (Ste->Bits.Valid == 0) || (Ste->Bits.S2Ttb == 0)) {
        continue;
      }

      Root  = (PAGE_TABLE *)(UINTN)((UINT64)Ste->Bits.S2Ttb << SMMUV3_STREAM_TABLE_ENTRY_S2TTB_OFFSET);
      IsDup = FALSE;
      for (DupIdx = 0; DupIdx < FreedCount; DupIdx++) {
        if (FreedRoots[DupIdx] == Root) {
          IsDup = TRUE;
          break;
        }
      }

      if (IsDup) {
        continue;
      }

      if ((FreedRoots != NULL) && (FreedCount < MaxRoots)) {
        FreedRoots[FreedCount++] = Root;
      }

      SmmuV3FreePageTableTree (0, Root);
    }
  } else {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate dedupe buffer; leaking per-stream roots on SMMU 0x%llx\n", __func__, SmmuInfo->SmmuBase));
  }

  if (FreedRoots != NULL) {
    FreePool (FreedRoots);
  }
}

/**
  Release the stream table itself plus any auxiliary L2 stream-table pages
  attached to it.

  For 2-level stream tables this frees every private L2 page installed by
  split-on-write (any L1 descriptor whose L2Ptr no longer points at the
  shared-ABORT L2) and then the shared-ABORT L2 page. For linear stream
  tables the L2 handling is a no-op. In both cases the top-level stream
  table allocation is released last via SmmuV3FreeStreamTable.

  Must run after SmmuV3FreePerStreamPageTableRoots so the S2Ttb pointers
  in each STE remain valid while roots are being reclaimed.

  @param [in]  SmmuInfo  SMMU instance whose stream table + L2 pages
                         should be freed. No-op if StreamTable is NULL.
**/
STATIC
VOID
SmmuV3FreeStreamTableAndL2Pages (
  IN SMMU_INFO  *SmmuInfo
  )
{
  SMMUV3_L1_STREAM_TABLE_DESCRIPTOR  *L1Tbl;
  UINTN                              L1Count;
  UINTN                              L1Idx;
  UINT64                             SharedEnc;

  if ((SmmuInfo == NULL) || (SmmuInfo->StreamTable == NULL)) {
    return;
  }

  // Free any private L2 stream-table pages allocated by split-on-write.
  // The shared-ABORT L2 page is freed separately below.
  if (SmmuInfo->SharedAbortL2 != NULL) {
    L1Tbl     = (SMMUV3_L1_STREAM_TABLE_DESCRIPTOR *)SmmuInfo->StreamTable;
    L1Count   = SmmuInfo->StreamTableSize / sizeof (SMMUV3_L1_STREAM_TABLE_DESCRIPTOR);
    SharedEnc = (UINT64)(UINTN)SmmuInfo->SharedAbortL2 >> SMMUV3_STR_TAB_BASE_L2_PTR_OFFSET;
    for (L1Idx = 0; L1Idx < L1Count; L1Idx++) {
      if ((L1Tbl[L1Idx].Bits.L2Ptr != 0) && ((UINT64)L1Tbl[L1Idx].Bits.L2Ptr != SharedEnc)) {
        FreePages (
          (VOID *)(UINTN)((UINT64)L1Tbl[L1Idx].Bits.L2Ptr << SMMUV3_STR_TAB_BASE_L2_PTR_OFFSET),
          1
          );
        L1Tbl[L1Idx].Bits.L2Ptr = 0;
      }
    }

    FreePages (SmmuInfo->SharedAbortL2, 1);
    SmmuInfo->SharedAbortL2 = NULL;
  }

  SmmuV3FreeStreamTable (SmmuInfo->StreamTable, SmmuInfo->StreamTableSize);
  SmmuInfo->StreamTable = NULL;
}

/**
  Deinitialize and free the SMMU_INFO structure and everything inside.
  Also disables SMMU translation and sets global abort.

  @param [in]  IoMmu   Pointer to the IOMMU_CONFIG structure to deinitialize.
**/
STATIC
VOID
IoMmuDeInit (
  IN IOMMU_CONFIG  *IoMmu
  )
{
  EFI_STATUS  Status;
  UINT32      SmmuIndex;

  if (IoMmu == NULL) {
    ASSERT (IoMmu != NULL);
    return;
  }

  for (SmmuIndex = 0; SmmuIndex < IoMmu->SmmuCount; SmmuIndex++) {
    Status = SmmuV3DisableTranslation (IoMmu->SmmuInfo[SmmuIndex].SmmuBase);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to disable SMMUv3 translation 0x%llx\n", __func__, IoMmu->SmmuInfo[SmmuIndex].SmmuBase));
    }

    Status = SmmuV3GlobalAbort (IoMmu->SmmuInfo[SmmuIndex].SmmuBase);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to global abort SMMUv3 0x%llx\n", __func__, IoMmu->SmmuInfo[SmmuIndex].SmmuBase));
    }

    // Free any per-StreamID page-table roots installed by lazy STE
    // promotion, then release the stream table (and its auxiliary L2
    // pages, if 2-level).
    SmmuV3FreePerStreamPageTableRoots (&IoMmu->SmmuInfo[SmmuIndex]);
    SmmuV3FreeStreamTableAndL2Pages (&IoMmu->SmmuInfo[SmmuIndex]);

    if (IoMmu->SmmuInfo[SmmuIndex].CommandQueue != NULL) {
      SmmuV3FreeCommandQueue (IoMmu->SmmuInfo[SmmuIndex].CommandQueue, IoMmu->SmmuInfo[SmmuIndex].CommandQueueLog2Size);
      IoMmu->SmmuInfo[SmmuIndex].CommandQueue = NULL;
    }

    if (IoMmu->SmmuInfo[SmmuIndex].EventQueue != NULL) {
      SmmuV3FreeEventQueue (IoMmu->SmmuInfo[SmmuIndex].EventQueue, IoMmu->SmmuInfo[SmmuIndex].EventQueueLog2Size);
      IoMmu->SmmuInfo[SmmuIndex].EventQueue = NULL;
    }
  }

  FreePool (IoMmu->SmmuInfo);
  FreePool (IoMmu);
}

/**
  Disable SMMU translation and set SMMU to global abort or bypass during ExitBootServices
  depending on the EBSBehaviorAbort flag in the SMMU_INFO structure.

  @param [in] Event    The event that triggered this notification function.
  @param [in] Context  Pointer to the notification function's context.
**/
STATIC
VOID
SmmuV3ExitBootServices (
  IN      EFI_EVENT  Event,
  IN      VOID       *Context
  )
{
  EFI_STATUS  Status;
  UINT32      SmmuIndex;

  if (Event == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Event\n", __func__));
    ASSERT (Event != NULL);
    return;
  }

  if ((mIoMmu == NULL) || (mIoMmu->SmmuInfo == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: IOMMU_CONFIG/SMMU_INFO structure is NULL\n", __func__));
    ASSERT (mIoMmu != NULL);
    ASSERT (mIoMmu->SmmuInfo != NULL);
    return;
  }

  for (SmmuIndex = 0; SmmuIndex < mIoMmu->SmmuCount; SmmuIndex++) {
    if (mIoMmu->SmmuInfo[SmmuIndex].Enabled) {
      if (mIoMmu->SmmuInfo[SmmuIndex].EBSBehaviorAbort) {
        Status = SmmuV3GlobalAbort (mIoMmu->SmmuInfo[SmmuIndex].SmmuBase);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "%a: Failed to global abort smmu 0x%llx.\n", __func__, mIoMmu->SmmuInfo[SmmuIndex].SmmuBase));
          ASSERT_EFI_ERROR (Status);
        }
      } else {
        Status = SmmuV3SetGlobalBypass (mIoMmu->SmmuInfo[SmmuIndex].SmmuBase);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "%a: Failed to set smmu 0x%llx global bypass.\n", __func__, mIoMmu->SmmuInfo[SmmuIndex].SmmuBase));
          ASSERT_EFI_ERROR (Status);
        }
      }

      Status = SmmuV3DisableTranslation (mIoMmu->SmmuInfo[SmmuIndex].SmmuBase);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: Failed to disable smmu 0x%llx translation.\n", __func__, mIoMmu->SmmuInfo[SmmuIndex].SmmuBase));
        ASSERT_EFI_ERROR (Status);
      }
    }
  }

  gBS->CloseEvent (Event);
}

/**
  Entrypoint for SmmuDxe driver.
  Configures IORT, and SMMUv3 hardware based on the configuration data from gSmmuConfigHobGuid HOB.
  Uses a linear stream table and stage 2 translation for dma remapping.
  Initializes IoMmu Protocol.

  @param [in] ImageHandle    The firmware allocated handle for the EFI image.
  @param [in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS               The entry point is executed successfully.
  @retval EFI_OUT_OF_RESOURCES      Not enough resources to initialize the driver.
  @retval EFI_NOT_FOUND             The SMMU configuration data is not found.
  @retval EFI_INVALID_PARAMETER     Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES      Out of resources.
  @retval EFI_TIMEOUT               Timeout.
  @retval EFI_DEVICE_ERROR          Device error.
  @retval EFI_INCOMPATIBLE_VERSION  Incompatible version.
  @retval Others                    Some error occurs when executing this entry point.
**/
EFI_STATUS
InitializeSmmuDxe (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS               Status;
  EFI_EVENT                Event;
  UINT32                   SmmuIndex;
  UINT32                   SmmuStatusIndex;
  UINT32                   SmmuDisabledCount;
  UINT64                   *SmmuDisabledList;
  EFI_ACPI_TABLE_PROTOCOL  *AcpiTable;
  SMMU_CONFIG              *SmmuConfig;
  VOID                     *IortData;

  // Get SMMU configuration data from HOB
  SmmuConfig = GetSmmuConfigHobData ();
  if (SmmuConfig == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get SMMU config data from gSmmuConfigHobGuid\n", __func__));
    return EFI_NOT_FOUND;
  }

  // Check SMMU_CONFIG version, return error if incompatible. Backwards compatibility not supported.
  Status = CheckSmmuConfigStructure (SmmuConfig);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: SMMU_CONFIG version check failed\n", __func__));
    return Status;
  }

  // Check if ACPI Table Protocol has been installed
  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **)&AcpiTable
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to locate ACPI Table Protocol\n", __func__));
    return Status;
  }

  // Retrieve GIC interrupt registration interface so we can hook SMMU
  // EVTQ / GERR interrupts later in SmmuV3Configure. Treat absence as
  // non-fatal so SMMU configuration still proceeds without ISRs.
  Status = gBS->LocateProtocol (
                  &gHardwareInterrupt2ProtocolGuid,
                  NULL,
                  (VOID **)&mGicInterrupt
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "%a: HardwareInterrupt2 protocol not found (%r); SMMU IRQs will not be hooked\n", __func__, Status));
    mGicInterrupt = NULL;
  }

  // Create an event callback to disable SMMUv3 translation and set global abort during ExitBootServices
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  SmmuV3ExitBootServices,
                  NULL,
                  &gEfiEventExitBootServicesGuid,
                  &Event
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to create ExitBootServices event\n", __func__));
    return Status;
  }

  Status = IoMmuConfigInit (&mIoMmu);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to initialize IoMmu Config\n", __func__));
    return Status;
  }

  IortData = (VOID *)((UINTN)SmmuConfig + (UINTN)SmmuConfig->IortOffset);

  Status = SmmuV3ParseIort (IortData, &mIoMmu->SmmuInfo, &mIoMmu->SmmuCount);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to parse IORT for SMMU\n", __func__));
    return Status;
  }

  DEBUG ((DEBUG_VERBOSE, "%a: Found %u SMMUs\n", __func__, mIoMmu->SmmuCount));

  // Save IORT data pointer for StreamID lookups after PCI enumeration.
  mIortData = IortData;

  // Pick up the platform's NonDiscoverable {UniqueId -> Named Component Obj Name}
  // lookup table, if provided. SmmuDxe uses this in IoMmuSetAttribute to
  // resolve NC DeviceHandles reliably.
  if ((SmmuConfig->NcDeviceListSize >= sizeof (SMMU_NC_DEVICE_ENTRY)) &&
      (SmmuConfig->NcDeviceListOffset != 0))
  {
    mIoMmu->NcDeviceList  = (SMMU_NC_DEVICE_ENTRY *)((UINTN)SmmuConfig + (UINTN)SmmuConfig->NcDeviceListOffset);
    mIoMmu->NcDeviceCount = SmmuConfig->NcDeviceListSize / sizeof (SMMU_NC_DEVICE_ENTRY);
    DEBUG ((DEBUG_VERBOSE, "%a: NonDiscoverable lookup table: %u entries\n", __func__, mIoMmu->NcDeviceCount));
  } else {
    mIoMmu->NcDeviceList  = NULL;
    mIoMmu->NcDeviceCount = 0;
    DEBUG ((DEBUG_WARN, "%a: No NonDiscoverable lookup table - NC StreamID resolution will fail\n", __func__));
  }

  // Add IORT Table
  Status = AddIortTable (AcpiTable, IortData, SmmuConfig->IortSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to add IORT table\n", __func__));
    goto Error;
  }

  // Set SMMUs' Enabled status based on the SmmuDisabledList in the SMMU_CONFIG HOB structure.
  SmmuDisabledCount = SmmuConfig->SmmuDisabledListSize / sizeof (UINT64);
  SmmuDisabledList  = (UINT64 *)((UINTN)SmmuConfig + (UINTN)SmmuConfig->SmmuDisabledListOffset);

  for (SmmuIndex = 0; SmmuIndex < mIoMmu->SmmuCount; SmmuIndex++) {
    mIoMmu->SmmuInfo[SmmuIndex].Enabled = TRUE;
    for (SmmuStatusIndex = 0; SmmuStatusIndex < SmmuDisabledCount; SmmuStatusIndex++) {
      if (mIoMmu->SmmuInfo[SmmuIndex].SmmuBase == SmmuDisabledList[SmmuStatusIndex]) {
        mIoMmu->SmmuInfo[SmmuIndex].Enabled = FALSE;
      }
    }
  }

  // Configure SMMUv3 hardware
  for (SmmuIndex = 0; SmmuIndex < mIoMmu->SmmuCount; SmmuIndex++) {
    if (mIoMmu->SmmuInfo[SmmuIndex].Enabled) {
      Status = SmmuV3Configure (&mIoMmu->SmmuInfo[SmmuIndex]);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: Failed to configure SMMUv3 hardware\n", __func__));
        goto Error;
      }

      DEBUG ((DEBUG_INFO, "%a: SMMUv3 0x%llx is configured for Stage2 Translation\n", __func__, mIoMmu->SmmuInfo[SmmuIndex].SmmuBase));

      // Pre-map any IORT RMR ranges into the per-StreamID page tables that the RMR's IdMappings cover.
      Status = SmmuV3AddRMRMapping (&mIoMmu->SmmuInfo[SmmuIndex]);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: Failed to add RMR mappings for SMMU 0x%llx: %r\n", __func__, mIoMmu->SmmuInfo[SmmuIndex].SmmuBase, Status));
        goto Error;
      }
    }
  }

  // Disable any SMMU that is not enabled.
  // Disables translation and sets global bypass.
  for (SmmuIndex = 0; SmmuIndex < mIoMmu->SmmuCount; SmmuIndex++) {
    if (mIoMmu->SmmuInfo[SmmuIndex].Enabled == FALSE) {
      Status = SmmuV3DisableTranslation (mIoMmu->SmmuInfo[SmmuIndex].SmmuBase);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: Failed to disable smmu 0x%llx translation.\n", __func__, mIoMmu->SmmuInfo[SmmuIndex].SmmuBase));
        ASSERT_EFI_ERROR (Status);
      }

      Status = SmmuV3SetGlobalBypass (mIoMmu->SmmuInfo[SmmuIndex].SmmuBase);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: Failed to set smmu 0x%llx global bypass.\n", __func__, mIoMmu->SmmuInfo[SmmuIndex].SmmuBase));
        ASSERT_EFI_ERROR (Status);
      }

      DEBUG ((DEBUG_INFO, "%a: SMMUv3 0x%llx is disabled/global bypass.\n", __func__, mIoMmu->SmmuInfo[SmmuIndex].SmmuBase));
    }
  }

  // Initialize IoMmu Protocol
  Status = IoMmuInit ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to intall IoMmuProtocol\n", __func__));
    goto Error;
  }

  DEBUG ((DEBUG_INFO, "%a: Status = %llx\n", __func__, Status));

  return Status;

Error:
  DEBUG ((DEBUG_ERROR, "%a: SMMU DMA protection failed to initialize. Status = %llx\n", __func__, Status));
  IoMmuDeInit (mIoMmu);
  mIoMmu = NULL;
  return Status;
}
