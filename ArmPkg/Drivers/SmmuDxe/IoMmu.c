/** @file IoMmu.c

    This file contains functions for the IoMmu protocol for use with the SMMU driver.
    This driver provides a generic interface for mapping host memory to device memory.
    Maintains a 4-level (0-3) page table for mapping virtual addresses to physical addresses.
    The mapping is identity mapped.

    Copyright (c) Microsoft Corporation.
    SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/ArmLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Protocol/IoMmu.h>
#include "SmmuV3.h"

/**
  IOMMU Mapping structure used to store the mapping information.
  Used to pass between IoMmuMap, IoMmuUnmap and IoMmuSetAttribute.
**/
typedef struct IOMMU_MAP_INFO {
  UINTN                    NumberOfBytes;
  UINT64                   DeviceAddress;
  UINT64                   HostAddress;
  EDKII_IOMMU_OPERATION    Operation;
} IOMMU_MAP_INFO;

/**
  Decode the (PageTableRoot, Vmid) currently programmed in a Valid STAGE_2
  STE. The STE is the single source of truth for per-stream translation
  state; the (Root, Vmid) pair returned here is exactly what the SMMU is
  using to translate this StreamID.

  Caller must have already confirmed Ste->Bits.Valid != 0 (a not-yet-promoted
  STE is a normal initial state, not an error condition).

  @param [in]  Ste     STE slot.
  @param [out] Root    Receives the stage-2 page-table root encoded in S2Ttb.
  @param [out] Vmid    Receives the VMID encoded in S2Vmid.

  @retval EFI_SUCCESS            (Root, Vmid) decoded.
  @retval EFI_INVALID_PARAMETER  Any of Ste / Root / Vmid is NULL.
**/
STATIC
EFI_STATUS
SmmuV3DecodeSte (
  IN  SMMUV3_STREAM_TABLE_ENTRY  *Ste,
  OUT PAGE_TABLE                 **Root,
  OUT UINT16                     *Vmid
  )
{
  if ((Ste == NULL) || (Root == NULL) || (Vmid == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid parameter\n", __func__));
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
    return EFI_INVALID_PARAMETER;
  }

  *Root = (PAGE_TABLE *)(UINTN)((UINT64)Ste->Bits.S2Ttb << SMMUV3_STREAM_TABLE_ENTRY_S2TTB_OFFSET);
  *Vmid = (UINT16)Ste->Bits.S2Vmid;
  return EFI_SUCCESS;
}

/**
  Allocate the next per-stream VMID for SmmuInfo. VMID 0 is reserved as
  "unassigned"; the allocator hands out 1..MaxVmid (width depends on
  IDR0.VMID16) and never reuses a VMID within the same boot.

  @param [in]   SmmuInfo  SMMU instance.
  @param [out]  OutVmid   Receives the newly allocated VMID.

  @retval EFI_SUCCESS            VMID allocated.
  @retval EFI_OUT_OF_RESOURCES   VMID space exhausted.
**/
STATIC
EFI_STATUS
SmmuV3AllocateVmid (
  IN  SMMU_INFO  *SmmuInfo,
  OUT UINT16     *OutVmid
  )
{
  UINT16  MaxVmid;

  MaxVmid = SmmuInfo->Vmid16Supported ? MAX_UINT16 : MAX_UINT8;
  if (SmmuInfo->NextVmid == SMMU_VMID_RESERVED) {
    // wrapped past the max
    DEBUG ((DEBUG_ERROR, "%a: VMID space exhausted on SMMU 0x%llx\n", __func__, SmmuInfo->SmmuBase));
    ASSERT (SmmuInfo->NextVmid != SMMU_VMID_RESERVED);
    return EFI_OUT_OF_RESOURCES;
  }

  *OutVmid = SmmuInfo->NextVmid;
  if (SmmuInfo->NextVmid == MaxVmid) {
    SmmuInfo->NextVmid = SMMU_VMID_RESERVED; // mark exhausted; next allocation will fail above
  } else {
    SmmuInfo->NextVmid++;
  }

  return EFI_SUCCESS;
}

/**
  Ensure a stage-2 page-table root exists for the given StreamID. On first
  call for a StreamID, allocates a fresh root + VMID and promotes the
  corresponding STE from Invalid to a Valid STAGE_2_TRANSLATE entry using
  break-before-make. Subsequent calls read the (Root, Vmid) back out of the
  live STE (the single source of truth).

  @param [in]   SmmuInfo  SMMU instance.
  @param [in]   StreamId  StreamID.
  @param [out]  OutRoot   Receives the stage-2 page-table root.
  @param [out]  OutVmid   Receives the VMID tag installed in the STE.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameters.
  @retval EFI_OUT_OF_RESOURCES   Allocation failed / VMID space exhausted.
  @retval Other                  STE promotion failure.
**/
EFI_STATUS
SmmuV3StreamGetOrCreate (
  IN  SMMU_INFO   *SmmuInfo,
  IN  UINT32      StreamId,
  OUT PAGE_TABLE  **OutRoot,
  OUT UINT16      *OutVmid
  )
{
  EFI_STATUS                 Status;
  SMMUV3_STREAM_TABLE_ENTRY  *Ste;
  PAGE_TABLE                 *NewRoot;
  UINT16                     NewVmid;
  EFI_TPL                    OldTpl;

  if ((SmmuInfo == NULL) || (OutRoot == NULL) || (OutVmid == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid parameter\n", __func__));
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  Ste = SmmuV3GetSteSlot (SmmuInfo, StreamId);
  if (Ste == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: No STE slot for SmmuBase=0x%llx StreamId 0x%x\n", __func__, SmmuInfo->SmmuBase, StreamId));
    ASSERT (Ste != NULL);
    gBS->RestoreTPL (OldTpl);
    return EFI_INVALID_PARAMETER;
  }

  // Already promoted -> read the (Root, Vmid) the SMMU is actively using.
  if (Ste->Bits.Valid != 0) {
    Status = SmmuV3DecodeSte (Ste, OutRoot, OutVmid);
    gBS->RestoreTPL (OldTpl);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to decode STE for SmmuBase=0x%llx StreamId 0x%x\n", __func__, SmmuInfo->SmmuBase, StreamId));
      ASSERT_EFI_ERROR (Status);
    }

    return Status;
  }

  Status = SmmuV3AllocateVmid (SmmuInfo, &NewVmid);
  if (EFI_ERROR (Status)) {
    gBS->RestoreTPL (OldTpl);
    return Status;
  }

  gBS->RestoreTPL (OldTpl);

  NewRoot = SmmuV3AllocatePageTableRoot ();
  if (NewRoot == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate page-table root for SmmuBase=0x%llx StreamId 0x%x\n", __func__, SmmuInfo->SmmuBase, StreamId));
    ASSERT (NewRoot != NULL);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = SmmuV3PromoteSteToTranslate (SmmuInfo, StreamId, NewVmid, NewRoot);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to promote STE for SmmuBase=0x%llx StreamId 0x%x: %r\n", __func__, SmmuInfo->SmmuBase, StreamId, Status));
    SmmuV3FreePageTableTree (0, NewRoot);
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  *OutRoot = NewRoot;
  *OutVmid = NewVmid;
  return EFI_SUCCESS;
}

/**
  Bind a StreamID's STE to share an existing primary stream's stage-2
  page-table root and VMID. Used when a single device exposes multiple
  StreamIDs that must all see the same translations.

  If the alias STE already encodes the same (Root, Vmid) it is a no-op.
  Otherwise the alias STE is promoted in place to publish the shared root +
  VMID. An alias STE that has already been promoted with a *different*
  root is treated as a configuration error.

  @param [in]  SmmuInfo       SMMU instance.
  @param [in]  AliasStreamId  StreamID that should alias the primary.
  @param [in]  PrimaryRoot    Primary stream's page-table root (non-NULL).
  @param [in]  PrimaryVmid    Primary stream's VMID (non-zero).

  @retval EFI_SUCCESS            Alias bound.
  @retval EFI_INVALID_PARAMETER  Invalid parameters.
  @retval EFI_ALREADY_STARTED    Alias STE already points at a different root.
  @retval Other                  STE-promotion failure.
**/
STATIC
EFI_STATUS
SmmuV3StreamAlias (
  IN  SMMU_INFO   *SmmuInfo,
  IN  UINT32      AliasStreamId,
  IN  PAGE_TABLE  *PrimaryRoot,
  IN  UINT16      PrimaryVmid
  )
{
  EFI_STATUS                 Status;
  SMMUV3_STREAM_TABLE_ENTRY  *AliasSte;
  PAGE_TABLE                 *ExistingRoot;
  UINT16                     ExistingVmid;
  EFI_TPL                    OldTpl;

  if ((SmmuInfo == NULL) || (PrimaryRoot == NULL) || (PrimaryVmid == 0)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid parameter\n", __func__));
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  AliasSte = SmmuV3GetSteSlot (SmmuInfo, AliasStreamId);
  if (AliasSte == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: No STE slot for SmmuBase=0x%llx StreamId 0x%x\n", __func__, SmmuInfo->SmmuBase, AliasStreamId));
    ASSERT (AliasSte != NULL);
    gBS->RestoreTPL (OldTpl);
    return EFI_INVALID_PARAMETER;
  }

  // If the alias STE is already promoted, compare against the primary.
  if (AliasSte->Bits.Valid != 0) {
    Status = SmmuV3DecodeSte (AliasSte, &ExistingRoot, &ExistingVmid);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      gBS->RestoreTPL (OldTpl);
      return Status;
    }

    if ((ExistingRoot == PrimaryRoot) && (ExistingVmid == PrimaryVmid)) {
      gBS->RestoreTPL (OldTpl);
      return EFI_SUCCESS;
    }

    if (ExistingRoot != PrimaryRoot) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: StreamId 0x%x already has its own root %p; cannot alias to %p\n",
        __func__,
        AliasStreamId,
        ExistingRoot,
        PrimaryRoot
        ));
      ASSERT (ExistingRoot == PrimaryRoot);
      gBS->RestoreTPL (OldTpl);
      return EFI_ALREADY_STARTED;
    }
  }

  gBS->RestoreTPL (OldTpl);

  Status = SmmuV3PromoteSteToTranslate (
             SmmuInfo,
             AliasStreamId,
             PrimaryVmid,
             PrimaryRoot
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: STE promotion failed for alias StreamId 0x%x: %r\n",
      __func__,
      AliasStreamId,
      Status
      ));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: Aliased StreamId 0x%x (root=%p VMID=0x%x)\n",
    __func__,
    AliasStreamId,
    PrimaryRoot,
    PrimaryVmid
    ));

  return EFI_SUCCESS;
}

/**
  Update the mapping of a virtual address to a physical address in the page table.

  Iterates through the page table levels to find the leaf entry for the given virtual address and
  validates entries along the way as needed. The leaf entry is then updated with the physical address along
  with appropriate flags and valid bit set. The option SetFlagsOnly allows traversal of the page table while
  only updating the flags of the entry, allowing clearing of flag bits as well.

  Break-before-make does not apply here because we are only switching between invalid/valid,
  no other Entry bits are changing. If the entry is already valid, it must have the same
  PA and flags to be considered a match; otherwise it's an error because we don't expect
  multiple mappings for the same VA.

  @param [in]  SmmuInfo                   SMMU instance whose translation parameters drive the page-table walk.
  @param [in]  Root                       Pointer to the root page table.
  @param [in]  VirtualAddress             Virtual address to map.
  @param [in]  PhysicalAddress            Physical address to map to.
  @param [in]  Flags                      Flags to set for the mapping. 12 bit or less.
  @param [in]  Valid                      Boolean to indicate if the entry is valid.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES   Out of resources.
**/
STATIC
EFI_STATUS
UpdateMapping (
  IN SMMU_INFO   CONST *CONST  SmmuInfo,
  IN PAGE_TABLE                *Root,
  IN UINT64                    VirtualAddress,
  IN UINT64                    PhysicalAddress,
  IN UINT16                    Flags,
  IN BOOLEAN                   Valid
  )
{
  EFI_STATUS  Status;
  UINT8       Level;
  UINT32      Index;
  PAGE_TABLE  *Current;
  UINT64      Entry;
  EFI_TPL     OldTpl;

  // Flags must be 12 bits or less
  if ((Root == NULL) || (SmmuInfo == NULL) || ((Flags & ~PAGE_TABLE_BLOCK_OFFSET) != 0) || (PhysicalAddress == 0)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid parameter.\n", __func__));
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
    return EFI_INVALID_PARAMETER;
  }

  Status  = EFI_SUCCESS;
  Current = Root;

  OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  // Traverse the page table to the leaf level.
  for (Level = SmmuInfo->TranslationStartingLevel; Level < PAGE_TABLE_DEPTH - 1; Level++) {
    Index = PAGE_TABLE_INDEX (VirtualAddress, Level, SmmuInfo->OutputAddressWidth, SmmuInfo->TranslationStartingLevel, SmmuInfo->PageTableRootConcatenated);

    if (Current->Entries[Index] == 0) {
      gBS->RestoreTPL (OldTpl);
      PAGE_TABLE  *NewPage = (PAGE_TABLE *)AllocatePages (1);
      OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
      if (NewPage == NULL) {
        DEBUG ((DEBUG_ERROR, "%a: Failed allocating page.\n", __func__));
        Status = EFI_OUT_OF_RESOURCES;
        goto End;
      }

      ZeroMem ((VOID *)NewPage, EFI_PAGE_SIZE);
      Entry = (PAGE_TABLE_ENTRY)(UINTN)NewPage | PAGE_TABLE_ACCESS_FLAG | PAGE_TABLE_DESCRIPTOR | PAGE_TABLE_ENTRY_VALID_BIT;
      ArmDataSynchronizationBarrier ();

      Current->Entries[Index] = Entry; // valid entry
    }

    Current = (PAGE_TABLE *)((UINTN)Current->Entries[Index] & ~PAGE_TABLE_BLOCK_OFFSET);
  }

  // leaf level
  if (Current != 0) {
    Index = PAGE_TABLE_INDEX (VirtualAddress, Level, SmmuInfo->OutputAddressWidth, SmmuInfo->TranslationStartingLevel, SmmuInfo->PageTableRootConcatenated);

    if (Valid) {
      Entry = (PhysicalAddress & ~PAGE_TABLE_BLOCK_OFFSET); // Assign PA
      // validate entry and set leaf level flags
      Entry |= Flags | PAGE_TABLE_ACCESS_FLAG | PAGE_TABLE_DESCRIPTOR | PAGE_TABLE_ENTRY_VALID_BIT;

      // Break-before-make does not apply here because we are only switching between invalid/valid, no other Entry bits are changing.
      // If the entry is already valid, it must have the same PA and flags to be considered a match; otherwise it's an error because we don't expect multiple mappings for the same VA.
      if ((Current->Entries[Index] & PAGE_TABLE_ENTRY_VALID_BIT) != 0) {
        DEBUG ((DEBUG_INFO, "%a: Page already mapped with valid Entry. VirtualAddress = 0x%llx PhysicalAddress=0x%llx\n", __func__, VirtualAddress, PhysicalAddress));
        if (Current->Entries[Index] != Entry) {
          DEBUG ((DEBUG_ERROR, "%a: Page already mapped with different PA or flags. OldEntry = 0x%llx NewEntry = 0x%llx\n", __func__, Current->Entries[Index], Entry));
          Status = EFI_DEVICE_ERROR;
        }

        goto End;
      }

      Current->Entries[Index] =  Entry;
    } else {
      Entry                   = Current->Entries[Index] & ~PAGE_TABLE_ENTRY_VALID_BIT;
      Current->Entries[Index] = Entry; // only invalidate leaf entry
    }
  }

End:
  gBS->RestoreTPL (OldTpl);
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/**
  Update the page table mapping with the given physical address and flags.

  @param [in]  SmmuInfo                   SMMU instance.
  @param [in]  Root                       Pointer to the root page table.
  @param [in]  Vmid                       VMID for associated page table root.
  @param [in]  PhysicalAddress            Physical address to map.
  @param [in]  Bytes                      Number of bytes to map.
  @param [in]  Flags                      Flags to set for the mapping. 12 bits or less.
  @param [in]  Valid                      Boolean to indicate if the entry is valid.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES   Out of resources.
**/
EFI_STATUS
UpdatePageTable (
  IN SMMU_INFO   *SmmuInfo,
  IN PAGE_TABLE  *Root,
  IN UINT16      Vmid,
  IN UINT64      PhysicalAddress,
  IN UINT64      Bytes,
  IN UINT16      Flags,
  IN BOOLEAN     Valid
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  PhysicalAddressEnd;
  EFI_PHYSICAL_ADDRESS  CurPhysicalAddress;

  if ((Root == NULL) || (SmmuInfo == NULL) || ((Flags & ~PAGE_TABLE_BLOCK_OFFSET) != 0) || (PhysicalAddress == 0) || (Bytes == 0)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid parameter\n", __func__));
    Status = EFI_INVALID_PARAMETER;
    goto End;
  }

  CurPhysicalAddress = PhysicalAddress;
  PhysicalAddressEnd = ALIGN_VALUE (PhysicalAddress + Bytes, EFI_PAGE_SIZE);

  while (CurPhysicalAddress < PhysicalAddressEnd) {
    Status = UpdateMapping (SmmuInfo, Root, CurPhysicalAddress, CurPhysicalAddress, Flags, Valid);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to update page table mapping\n", __func__));
      goto End;
    }

    CurPhysicalAddress += EFI_PAGE_SIZE;
  }

  if (!Valid) {
    Status = SmmuV3TLBInvalidateAll (SmmuInfo, Vmid);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to invalidate TLB for Vmid 0x%llx\n", __func__, Vmid));
      goto End;
    }
  }

End:
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/**
  Map a host address to a device address using the Page Table.
  Currently, this function only supports identity mapping.

  @param [in]      This            Pointer to the IOMMU protocol instance.
  @param [in]      Operation       The type of IOMMU operation.
  @param [in]      HostAddress     The host address to map.
  @param [in, out] NumberOfBytes   On input, the number of bytes to map. On output, the number of bytes mapped.
  @param [out]     DeviceAddress   The resulting device address.
  @param [out]     Mapping         A handle to the mapping.

  @retval EFI_SUCCESS              Success.
  @retval EFI_INVALID_PARAMETER    Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES     Out of resources.
**/
EFI_STATUS
EFIAPI
IoMmuMap (
  IN     EDKII_IOMMU_PROTOCOL   *This,
  IN     EDKII_IOMMU_OPERATION  Operation,
  IN     VOID                   *HostAddress,
  IN OUT UINTN                  *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS   *DeviceAddress,
  OUT    VOID                   **Mapping
  )
{
  EFI_STATUS            Status;
  IOMMU_MAP_INFO        *MapInfo;
  EFI_PHYSICAL_ADDRESS  PhysicalAddress;
  BOOLEAN               NeedRemap;
  EFI_PHYSICAL_ADDRESS  DmaMemoryTop;

  Status    = EFI_SUCCESS;
  NeedRemap = FALSE;

  if ((This == NULL) ||
      (HostAddress == NULL) ||
      (NumberOfBytes == NULL) ||
      (*NumberOfBytes == 0) ||
      (DeviceAddress == NULL) ||
      (Mapping == NULL))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid parameter\n", __func__));
    Status = EFI_INVALID_PARAMETER;
    goto End;
  }

  // Allocate and fill the IOMMU_MAP_INFO structure with mapping information
  MapInfo = (IOMMU_MAP_INFO *)AllocateZeroPool (sizeof (IOMMU_MAP_INFO));
  if (MapInfo == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate IOMMU_MAP_INFO structure\n", __func__));
    Status = EFI_OUT_OF_RESOURCES;
    goto End;
  }

  DmaMemoryTop    = MAX_UINTN;
  PhysicalAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)HostAddress;

  if ((Operation != EdkiiIoMmuOperationBusMasterCommonBuffer) && (Operation != EdkiiIoMmuOperationBusMasterCommonBuffer64)) {
    if ((*NumberOfBytes != ALIGN_VALUE (*NumberOfBytes, SIZE_4KB)) || (PhysicalAddress != ALIGN_VALUE (PhysicalAddress, SIZE_4KB))) {
      NeedRemap = TRUE;
    }

    if ((((Operation != EdkiiIoMmuOperationBusMasterRead64) &&
          (Operation != EdkiiIoMmuOperationBusMasterWrite64))) &&
        ((PhysicalAddress + *NumberOfBytes) > SIZE_4GB))
    {
      //
      // If the root bridge or the device cannot handle performing DMA above
      // 4GB but any part of the DMA transfer being mapped is above 4GB, then
      // map the DMA transfer to a buffer below 4GB.
      //
      NeedRemap    = TRUE;
      DmaMemoryTop = SIZE_4GB - 1;
    }
  }

  MapInfo->NumberOfBytes = *NumberOfBytes;
  MapInfo->DeviceAddress = DmaMemoryTop;
  MapInfo->HostAddress   = PhysicalAddress;
  MapInfo->Operation     = Operation;

  // Bounce buffer case
  if (NeedRemap) {
    Status = gBS->AllocatePages (
                    AllocateMaxAddress,
                    EfiBootServicesData,
                    EFI_SIZE_TO_PAGES (MapInfo->NumberOfBytes),
                    &MapInfo->DeviceAddress
                    );
    if (EFI_ERROR (Status)) {
      FreePool (MapInfo);
      *NumberOfBytes = 0;
      DEBUG ((DEBUG_ERROR, "%a: %r\n", __func__, Status));
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    //
    // If this is a read operation from the Bus Master's point of view,
    // then copy the contents of the real buffer into the mapped buffer
    // so the Bus Master can read the contents of the real buffer.
    //
    if ((Operation == EdkiiIoMmuOperationBusMasterRead) || (Operation == EdkiiIoMmuOperationBusMasterRead64)) {
      CopyMem ((VOID *)(UINTN)MapInfo->DeviceAddress, (VOID *)(UINTN)MapInfo->HostAddress, MapInfo->NumberOfBytes);
    }
  } else {
    MapInfo->DeviceAddress = MapInfo->HostAddress;
  }

  *DeviceAddress = MapInfo->DeviceAddress;
  *Mapping       = MapInfo;

End:
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/**
  Unmap a device address in the Page Table, invalidate the TLB with TLBI operation.

  @param [in]  This      Pointer to the IOMMU protocol instance.
  @param [in]  Mapping   The mapping to unmap.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES   Out of resources.
  @retval EFI_TIMEOUT            Timeout.
**/
EFI_STATUS
EFIAPI
IoMmuUnmap (
  IN  EDKII_IOMMU_PROTOCOL  *This,
  IN  VOID                  *Mapping
  )
{
  IOMMU_MAP_INFO  *MapInfo;

  if ((This == NULL) || (Mapping == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid parameter\n", __func__));
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
    return EFI_INVALID_PARAMETER;
  }

  MapInfo = (IOMMU_MAP_INFO *)Mapping;

  // Bounce buffer case
  if (MapInfo->DeviceAddress != MapInfo->HostAddress) {
    if ((MapInfo->DeviceAddress == 0) || (MapInfo->HostAddress == 0) || (MapInfo->NumberOfBytes == 0)) {
      DEBUG ((DEBUG_ERROR, "%a: Invalid fields in MapInfo struct.\n", __func__));
      ASSERT ((MapInfo->DeviceAddress != 0) && (MapInfo->HostAddress != 0) && (MapInfo->NumberOfBytes != 0));
      return EFI_INVALID_PARAMETER;
    }

    //
    // If this is a write operation from the Bus Master's point of view,
    // then copy the contents of the mapped buffer into the real buffer
    // so the processor can read the contents of the real buffer.
    //
    if ((MapInfo->Operation == EdkiiIoMmuOperationBusMasterWrite) || (MapInfo->Operation == EdkiiIoMmuOperationBusMasterWrite64)) {
      CopyMem (
        (VOID *)(UINTN)MapInfo->HostAddress,
        (VOID *)(UINTN)MapInfo->DeviceAddress,
        MapInfo->NumberOfBytes
        );
    }

    //
    // Free the mapped buffer and the MAP_INFO structure.
    //
    gBS->FreePages (MapInfo->DeviceAddress, EFI_SIZE_TO_PAGES (MapInfo->NumberOfBytes));
  }

  // Free the mapping structure allocated in IoMmuMap
  FreePool (Mapping);

  return EFI_SUCCESS;
}

/**
  Free a buffer allocated by IoMmuAllocateBuffer.

  @param [in]  This          Pointer to the IOMMU protocol instance.
  @param [in]  Pages         The number of pages to free.
  @param [in]  HostAddress   The host address to free.

  @retval EFI_SUCCESS            The requested pages were freed.
  @retval EFI_INVALID_PARAMETER  Memory is not a page-aligned address or Pages is invalid.
  @retval EFI_NOT_FOUND          The requested memory pages were not allocated with AllocatePages().
**/
EFI_STATUS
EFIAPI
IoMmuFreeBuffer (
  IN  EDKII_IOMMU_PROTOCOL  *This,
  IN  UINTN                 Pages,
  IN  VOID                  *HostAddress
  )
{
  EFI_STATUS  Status;

  if ((This == NULL) || (HostAddress == NULL) || (Pages == 0)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid parameter\n", __func__));
    Status = EFI_INVALID_PARAMETER;
    goto End;
  }

  Status = gBS->FreePages ((EFI_PHYSICAL_ADDRESS)(UINTN)HostAddress, Pages);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to free pages\n", __func__));
    goto End;
  }

End:
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/**
  Allocate a buffer for use with the IOMMU.

  @param [in]      This          Pointer to the IOMMU protocol instance.
  @param [in]      Type          The type of allocation to perform.
  @param [in]      MemoryType    The type of memory to allocate.
  @param [in]      Pages         The number of pages to allocate.
  @param [in, out] HostAddress   On input, the desired host address. On output, the allocated host address.
  @param [in]      Attributes    The memory attributes to use for the allocation.

  @retval EFI_SUCCESS           The requested pages were allocated.
  @retval EFI_INVALID_PARAMETER 1) Type is not AllocateAnyPages or
                                AllocateMaxAddress or AllocateAddress.
                                2) MemoryType is in the range
                                EfiMaxMemoryType..0x6FFFFFFF.
                                3) Memory is NULL.
                                4) MemoryType is EfiPersistentMemory.
  @retval EFI_OUT_OF_RESOURCES  The pages could not be allocated.
  @retval EFI_NOT_FOUND         The requested pages could not be found.
**/
EFI_STATUS
EFIAPI
IoMmuAllocateBuffer (
  IN     EDKII_IOMMU_PROTOCOL  *This,
  IN     EFI_ALLOCATE_TYPE     Type,
  IN     EFI_MEMORY_TYPE       MemoryType,
  IN     UINTN                 Pages,
  IN OUT VOID                  **HostAddress,
  IN     UINT64                Attributes
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  PhysicalAddress;

  if ((This == NULL) || (Pages == 0) || (HostAddress == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid parameter\n", __func__));
    Status = EFI_INVALID_PARAMETER;
    goto End;
  }

  if ((Attributes & EDKII_IOMMU_ATTRIBUTE_DUAL_ADDRESS_CYCLE) == 0) {
    // Limit allocations to memory below 4GB
    PhysicalAddress = SIZE_4GB - 1;
    Type            = AllocateMaxAddress;
  }

  Status = gBS->AllocatePages (
                  Type,
                  MemoryType,
                  Pages,
                  &PhysicalAddress
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate pages\n", __func__));
    goto End;
  }

  *HostAddress = (VOID *)(UINTN)PhysicalAddress;

End:
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/**
  Shared back-end for IoMmuSetAttribute / IoMmuSetAttributeById.

  Resolves OwningSmmuBase to an enabled SMMU instance, ensures a stage-2
  page-table root + VMID exist for PrimaryStreamId on it, optionally aliases
  additional StreamIDs (every node after the head of StreamIdList) to that
  root + VMID, then updates the page table with the requested mapping /
  permissions.

  @param [in] OwningSmmuBase   Base MMIO address of the SMMU that owns
                               PrimaryStreamId. Resolved to an enabled
                               SMMU_INFO instance internally.
  @param [in] PrimaryStreamId  Primary StreamID whose page-table root is
                               ensured / used for the mapping update.
  @param [in] StreamIdList     OPTIONAL. Full StreamID list whose first node
                               is the primary; every subsequent node is
                               aliased to the primary's root + VMID. Pass
                               NULL when there are no aliases to bind.
  @param [in] MapInfo          Mapping info from Map().
  @param [in] IoMmuAccess      R/W access bits.

  @retval EFI_SUCCESS            Success.
  @retval EFI_NOT_FOUND          No enabled SMMU matches OwningSmmuBase.
  @retval EFI_OUT_OF_RESOURCES   Out of resources.
  @retval EFI_DEVICE_ERROR       The IOMMU device reported an error.
  @retval Other                  Page-table / alias setup failure.
**/
STATIC
EFI_STATUS
IoMmuSetAttributeHelper (
  IN UINT64          OwningSmmuBase,
  IN UINT32          PrimaryStreamId,
  IN LIST_ENTRY      *StreamIdList OPTIONAL,
  IN IOMMU_MAP_INFO  *MapInfo,
  IN UINT64          IoMmuAccess
  )
{
  EFI_STATUS  Status;
  SMMU_INFO   *TargetSmmu;
  PAGE_TABLE  *PrimaryRoot;
  UINT16      PrimaryVmid;
  LIST_ENTRY  *Link;
  UINT32      SmmuIndex;

  if ((MapInfo == NULL) || (OwningSmmuBase == 0)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid parameter.\n", __func__));
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
    return EFI_INVALID_PARAMETER;
  }

  TargetSmmu = NULL;
  for (SmmuIndex = 0; SmmuIndex < mIoMmu->SmmuCount; SmmuIndex++) {
    if (mIoMmu->SmmuInfo[SmmuIndex].Enabled &&
        (mIoMmu->SmmuInfo[SmmuIndex].SmmuBase == OwningSmmuBase))
    {
      TargetSmmu = &mIoMmu->SmmuInfo[SmmuIndex];
      break;
    }
  }

  if (TargetSmmu == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: No enabled SMMU with base 0x%llx (StreamId 0x%x)\n",
      __func__,
      OwningSmmuBase,
      PrimaryStreamId
      ));
    ASSERT (TargetSmmu != NULL);
    return EFI_NOT_FOUND;
  }

  // Ensure / allocate root + VMID for the primary StreamID.
  PrimaryRoot = NULL;
  PrimaryVmid = 0;
  Status      = SmmuV3StreamGetOrCreate (TargetSmmu, PrimaryStreamId, &PrimaryRoot, &PrimaryVmid);
  if (EFI_ERROR (Status) || (PrimaryRoot == NULL) || (PrimaryVmid == 0)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to ensure page-table root for StreamId 0x%x on SMMU 0x%llx: %r\n",
      __func__,
      PrimaryStreamId,
      TargetSmmu->SmmuBase,
      Status
      ));
    if (!EFI_ERROR (Status)) {
      Status = EFI_OUT_OF_RESOURCES;
    }

    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: SmmuBase=0x%llx PrimaryStreamId=0x%x IoMmuAccess=0x%llx HostAddress=0x%llx DeviceAddress=0x%llx Bytes=0x%llx Root=%p VMID=0x%x\n",
    __func__,
    TargetSmmu->SmmuBase,
    PrimaryStreamId,
    IoMmuAccess,
    MapInfo->HostAddress,
    MapInfo->DeviceAddress,
    (UINT64)MapInfo->NumberOfBytes,
    PrimaryRoot,
    PrimaryVmid
    ));

  // Bind any alias StreamIDs (every node after the head of StreamIdList) to
  // the primary's root + VMID so a single page-table update below covers DMA
  // from all of them.
  if ((StreamIdList != NULL) && !IsListEmpty (StreamIdList)) {
    for (Link = GetNextNode (StreamIdList, GetFirstNode (StreamIdList));
         !IsNull (StreamIdList, Link);
         Link = GetNextNode (StreamIdList, Link))
    {
      SMMU_STREAM_ID_ENTRY  *AliasEntry;

      AliasEntry = BASE_CR (Link, SMMU_STREAM_ID_ENTRY, Link);
      Status     = SmmuV3StreamAlias (TargetSmmu, AliasEntry->StreamId, PrimaryRoot, PrimaryVmid);
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to alias StreamId 0x%x -> primary 0x%x on SMMU 0x%llx: %r\n",
          __func__,
          AliasEntry->StreamId,
          PrimaryStreamId,
          TargetSmmu->SmmuBase,
          Status
          ));
        ASSERT_EFI_ERROR (Status);
        return Status;
      }
    }
  }

  for (SmmuIndex = 0; SmmuIndex < mIoMmu->SmmuCount; SmmuIndex++) {
    if (mIoMmu->SmmuInfo[SmmuIndex].Enabled) {
      SmmuV3LogErrors (&mIoMmu->SmmuInfo[SmmuIndex]);
    }
  }

  Status = UpdatePageTable (
             TargetSmmu,
             PrimaryRoot,
             PrimaryVmid,
             MapInfo->DeviceAddress,
             MapInfo->NumberOfBytes,
             PAGE_TABLE_READ_WRITE_FROM_IOMMU_ACCESS ((EDKII_IOMMU_ACCESS_READ | EDKII_IOMMU_ACCESS_WRITE)), // TODO: https://github.com/microsoft/mu_silicon_arm_tiano/issues/375 debug issue on physical platform and revert the permissions
             (IoMmuAccess != 0)
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to update page table.\n", __func__));
  }

  ASSERT_EFI_ERROR (Status);
  return Status;
}

/**
  Set the R/W access attributes for Mapping in the Page Table.

  @param [in]  This          Pointer to the IOMMU protocol instance.
  @param [in]  DeviceHandle  The device handle to set attributes for.
  @param [in]  Mapping       The mapping to set attributes for.
  @param [in]  IoMmuAccess   The IOMMU access attributes for R/W.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES   Out of resources.
**/
EFI_STATUS
EFIAPI
IoMmuSetAttribute (
  IN EDKII_IOMMU_PROTOCOL  *This,
  IN EFI_HANDLE            DeviceHandle,
  IN VOID                  *Mapping,
  IN UINT64                IoMmuAccess
  )
{
  EFI_STATUS            Status;
  IOMMU_MAP_INFO        *MapInfo;
  LIST_ENTRY            StreamIdList;
  SMMU_STREAM_ID_ENTRY  *StreamIdEntry;
  UINT32                PrimaryStreamId;
  UINT64                OwningSmmuBase;

  if ((This == NULL) || (Mapping == NULL) || ((IoMmuAccess & ~(EDKII_IOMMU_ACCESS_READ | EDKII_IOMMU_ACCESS_WRITE)) != 0)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid parameter\n", __func__));
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
    return EFI_INVALID_PARAMETER;
  }

  MapInfo = (IOMMU_MAP_INFO *)Mapping;
  InitializeListHead (&StreamIdList);

  //
  // PCI-only lazy mapping path:
  //   1. Resolve the DeviceHandle to its IORT-derived StreamID(s) *and* the
  //      base address of the SMMUv3 node that owns them (via the matched
  //      RC ID-mapping's OutputReference, or the platform NC table for
  //      NonDiscoverable devices).
  //   2. Locate the SMMU_INFO whose SmmuBase matches.
  //   3. Ensure the *primary* StreamID has a per-stream stage-2 page-table
  //      root on that SMMU (allocates + promotes the STE on first call).
  //   4. For any additional StreamIDs reported for this device, alias them
  //      to share the primary's root + VMID so a single page-table update
  //      covers all of them.
  //   5. Update that shared root once with the requested mapping /
  //      permissions.
  //
  // The current IoMmu protocol does not have the DeviceHandle on Map(),
  // so all real page-table mutation happens here in SetAttribute().
  //
  if ((mIortData == NULL) || (DeviceHandle == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: IORT/DeviceHandle not available; cannot resolve StreamID\n", __func__));
    Status = EFI_UNSUPPORTED;
    goto End;
  }

  OwningSmmuBase = 0;
  Status         = DeviceHandleToStreamId (mIortData, DeviceHandle, &StreamIdList, &OwningSmmuBase);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: DeviceHandleToStreamId failed: %r\n", __func__, Status));
    goto End;
  }

  if (IsListEmpty (&StreamIdList)) {
    DEBUG ((DEBUG_ERROR, "%a: DeviceHandleToStreamId returned no StreamIDs\n", __func__));
    Status = EFI_DEVICE_ERROR;
    goto End;
  }

  StreamIdEntry   = BASE_CR (GetFirstNode (&StreamIdList), SMMU_STREAM_ID_ENTRY, Link);
  PrimaryStreamId = StreamIdEntry->StreamId;

  if (OwningSmmuBase == 0) {
    DEBUG ((DEBUG_ERROR, "%a: IORT did not name an owning SMMU for StreamId 0x%x\n", __func__, PrimaryStreamId));
    Status = EFI_NOT_FOUND;
    goto End;
  }

  Status = IoMmuSetAttributeHelper (
             OwningSmmuBase,
             PrimaryStreamId,
             &StreamIdList,
             MapInfo,
             IoMmuAccess
             );

End:
  SmmuStreamIdListFree (&StreamIdList);
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/**
  Set the R/W access attributes for Mapping in the Page Table for a caller
  that explicitly specifies (IommuBase, DmaId) rather than supplying an
  EFI_HANDLE.

  Intended for firmware-internal DMA agents that have no UEFI DeviceHandle, so
  DeviceHandleToStreamId() cannot resolve them. The caller MUST pass the
  base address of the owning SMMU and the DMA identifier (StreamID on Arm SMMU)
  emitted by the device.

  Only the single DmaId provided is programmed; no alias resolution is
  performed.

  @param [in]  This         Pointer to the IOMMU protocol instance.
  @param [in]  IommuBase    Base MMIO address of the IOMMU that owns DmaId.
                            0 means "no DMA protection / IOMMU not enabled" -
                            the call returns EFI_SUCCESS without touching any
                            page tables. For Arm this is the SmmuV3 base address.
  @param [in]  DmaId        DMA identifier emitted by the calling agent
                            (StreamID on Arm SMMU, RequesterID on VT-d).
  @param [in]  Mapping      The mapping returned from Map().
  @param [in]  IoMmuAccess  The IOMMU access attributes (R/W bits).

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter.
  @retval EFI_NOT_FOUND          No enabled SMMU matches IommuBase.
  @retval EFI_OUT_OF_RESOURCES   Out of resources.
  @retval EFI_DEVICE_ERROR       The IOMMU device reported an error.
**/
EFI_STATUS
EFIAPI
IoMmuSetAttributeById (
  IN EDKII_IOMMU_PROTOCOL  *This,
  IN UINT64                IommuBase,
  IN UINT32                DmaId,
  IN VOID                  *Mapping,
  IN UINT64                IoMmuAccess
  )
{
  EFI_STATUS  Status;

  if ((This == NULL) || (Mapping == NULL) ||
      ((IoMmuAccess & ~(EDKII_IOMMU_ACCESS_READ | EDKII_IOMMU_ACCESS_WRITE)) != 0))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid parameter\n", __func__));
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
    return EFI_INVALID_PARAMETER;
  }

  Status = IoMmuSetAttributeHelper (
             IommuBase,
             DmaId,
             NULL,
             (IOMMU_MAP_INFO *)Mapping,
             IoMmuAccess
             );

  ASSERT_EFI_ERROR (Status);
  return Status;
}

// IOMMU Protocol instance for the SMMU.
EDKII_IOMMU_PROTOCOL  SmmuIoMmu = {
  EDKII_IOMMU_PROTOCOL_REVISION,
  IoMmuSetAttribute,
  IoMmuMap,
  IoMmuUnmap,
  IoMmuAllocateBuffer,
  IoMmuFreeBuffer,
  IoMmuSetAttributeById,
};

/**
  Installs the IOMMU Protocol on this SMMU instance.

  @retval EFI_SUCCESS           All the protocol interface was installed.
  @retval EFI_OUT_OF_RESOURCES  There was not enough memory in pool to install all the protocols.
  @retval EFI_ALREADY_STARTED   A Device Path Protocol instance was passed in that is already present in
                                the handle database.
  @retval EFI_INVALID_PARAMETER Handle is NULL.
  @retval EFI_INVALID_PARAMETER Protocol is already installed on the handle specified by Handle.
**/
EFI_STATUS
IoMmuInit (
  VOID
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEdkiiIoMmuProtocolGuid,
                  &SmmuIoMmu,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to install gEdkiiIoMmuProtocolGuid\n", __func__));
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}
