/** @file
*  MMU implementation for RISC-V
*
*  Copyright (c) 2011-2020, ARM Limited. All rights reserved.
*  Copyright (c) 2016, Linaro Limited. All rights reserved.
*  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
*  Copyright (c) 2023, Ventana Micro Systems Inc. All Rights Reserved.<BR>
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <PiDxe.h>
#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseRiscVMmuLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Register/RiscV64/RiscVEncoding.h>

#define RISCV_PG_V           BIT0
#define RISCV_PG_R           BIT1
#define RISCV_PG_W           BIT2
#define RISCV_PG_X           BIT3
#define RISCV_PG_G           BIT5
#define RISCV_PG_A           BIT6
#define RISCV_PG_D           BIT7
#define PTE_ATTRIBUTES_MASK  0xE

#define PTE_PPN_MASK          0x3FFFFFFFFFFC00ULL
#define PTE_PPN_SHIFT         10
#define RISCV_MMU_PAGE_SHIFT  12

STATIC UINTN  mMaxRootTableLevel;
STATIC UINTN  mBitPerLevel;
STATIC UINTN  mTableEntryCount;

STATIC
BOOLEAN
RiscVMmuEnabled (
  VOID
  )
{
  return ((RiscVGetSupervisorAddressTranslationRegister () &
           SATP64_MODE) != (SATP_MODE_OFF << SATP64_MODE_SHIFT));
}

STATIC
UINTN
RiscVGetRootTranslateTable (
  VOID
  )
{
  return (RiscVGetSupervisorAddressTranslationRegister () & SATP64_PPN) <<
         RISCV_MMU_PAGE_SHIFT;
}

STATIC
BOOLEAN
IsValidPte (
  IN  UINTN  Entry
  )
{
  if (!(Entry & RISCV_PG_V) ||
      (((Entry & (RISCV_PG_R | RISCV_PG_W)) == RISCV_PG_W)))
  {
    return FALSE;
  }

  return TRUE;
}

STATIC
UINTN
SetValidPte (
  IN  UINTN  Entry
  )
{
  /* Set Valid and Global mapping bits */
  return Entry | RISCV_PG_G | RISCV_PG_V;
}

STATIC
BOOLEAN
IsBlockEntry (
  IN  UINTN  Entry
  )
{
  return IsValidPte (Entry) &&
         (Entry & (RISCV_PG_X | RISCV_PG_R));
}

STATIC
BOOLEAN
IsTableEntry (
  IN  UINTN  Entry
  )
{
  return IsValidPte (Entry) &&
         !IsBlockEntry (Entry);
}

STATIC
UINTN
SetTableEntry (
  IN  UINTN  Entry
  )
{
  Entry  = SetValidPte (Entry);
  Entry &= ~(RISCV_PG_X | RISCV_PG_W | RISCV_PG_R);

  return Entry;
}

STATIC
VOID
ReplaceTableEntry (
  IN  UINTN    *Entry,
  IN  UINTN    Value,
  IN  UINTN    RegionStart,
  IN  BOOLEAN  IsLiveBlockMapping
  )
{
  *Entry = Value;

  if (IsLiveBlockMapping && RiscVMmuEnabled ()) {
    RiscVLocalTlbFlush (RegionStart);
  }
}

STATIC
UINTN
GetPpnfromPte (
  UINTN  Entry,
  UINTN  Level
  )
{
  return ((Entry & PTE_PPN_MASK) >> PTE_PPN_SHIFT);
}

STATIC
UINTN
SetPpnToPte (
  UINTN  Entry,
  UINTN  Address,
  UINTN  Level
  )
{
  UINTN  Ppn;

  Ppn = ((Address >> RISCV_MMU_PAGE_SHIFT) << PTE_PPN_SHIFT);
  ASSERT (~(Ppn & ~PTE_PPN_MASK));
  Entry &= ~PTE_PPN_MASK;
  return Entry | Ppn;
}

STATIC
VOID
FreePageTablesRecursive (
  IN  UINTN  *TranslationTable,
  IN  UINTN  Level
  )
{
  UINTN  Index;

  if (Level < mMaxRootTableLevel - 1) {
    for (Index = 0; Index < mTableEntryCount; Index++) {
      if (IsTableEntry (TranslationTable[Index])) {
        FreePageTablesRecursive (
          (UINTN *)(GetPpnfromPte ((TranslationTable[Index]), Level) <<
                    RISCV_MMU_PAGE_SHIFT),
          Level + 1
          );
      }
    }
  }

  FreePages (TranslationTable, 1);
}

STATIC
EFI_STATUS
UpdateRegionMappingRecursive (
  IN  UINTN    RegionStart,
  IN  UINTN    RegionEnd,
  IN  UINTN    AttributeSetMask,
  IN  UINTN    AttributeClearMask,
  IN  UINTN    *PageTable,
  IN  UINTN    Level,
  IN  BOOLEAN  TableIsLive
  )
{
  EFI_STATUS  Status;
  UINTN       BlockShift;
  UINTN       BlockMask;
  UINTN       BlockEnd;
  UINTN       *Entry;
  UINTN       EntryValue;
  UINTN       *TranslationTable;
  BOOLEAN     NextTableIsLive;

  ASSERT (Level < mMaxRootTableLevel);
  ASSERT (((RegionStart | RegionEnd) & EFI_PAGE_MASK) == 0);

  BlockShift = (mMaxRootTableLevel - Level - 1) * mBitPerLevel + RISCV_MMU_PAGE_SHIFT;
  BlockMask  = MAX_ADDRESS >> (64 - BlockShift);

  DEBUG (
    (
     DEBUG_VERBOSE,
     "%a(%d): %llx - %llx set %lx clr %lx\n",
     __func__,
     Level,
     RegionStart,
     RegionEnd,
     AttributeSetMask,
     AttributeClearMask
    )
    );

  for ( ; RegionStart < RegionEnd; RegionStart = BlockEnd) {
    BlockEnd = MIN (RegionEnd, (RegionStart | BlockMask) + 1);
    Entry    = &PageTable[(RegionStart >> BlockShift) & (mTableEntryCount - 1)];

    //
    // If RegionStart or BlockEnd is not aligned to the block size at this
    // level, we will have to create a table mapping in order to map less
    // than a block, and recurse to create the block or page entries at
    // the next level. No block mappings are allowed at all at level 0,
    // so in that case, we have to recurse unconditionally.
    //
    if ((Level == 0) ||
        (((RegionStart | BlockEnd) & BlockMask) != 0) || IsTableEntry (*Entry))
    {
      ASSERT (Level < mMaxRootTableLevel - 1);
      if (!IsTableEntry (*Entry)) {
        //
        // No table entry exists yet, so we need to allocate a page table
        // for the next level.
        //
        TranslationTable = AllocatePages (1);
        if (TranslationTable == NULL) {
          return EFI_OUT_OF_RESOURCES;
        }

        ZeroMem (TranslationTable, EFI_PAGE_SIZE);

        if (IsBlockEntry (*Entry)) {
          //
          // We are splitting an existing block entry, so we have to populate
          // the new table with the attributes of the block entry it replaces.
          //
          Status = UpdateRegionMappingRecursive (
                     RegionStart & ~BlockMask,
                     (RegionStart | BlockMask) + 1,
                     *Entry & PTE_ATTRIBUTES_MASK,
                     PTE_ATTRIBUTES_MASK,
                     TranslationTable,
                     Level + 1,
                     FALSE
                     );
          if (EFI_ERROR (Status)) {
            //
            // The range we passed to UpdateRegionMappingRecursive () is block
            // aligned, so it is guaranteed that no further pages were allocated
            // by it, and so we only have to free the page we allocated here.
            //
            FreePages (TranslationTable, 1);
            return Status;
          }
        }

        NextTableIsLive = FALSE;
      } else {
        TranslationTable = (UINTN *)(GetPpnfromPte (*Entry, Level) << RISCV_MMU_PAGE_SHIFT);
        NextTableIsLive  = TableIsLive;
      }

      //
      // Recurse to the next level
      //
      Status = UpdateRegionMappingRecursive (
                 RegionStart,
                 BlockEnd,
                 AttributeSetMask,
                 AttributeClearMask,
                 TranslationTable,
                 Level + 1,
                 NextTableIsLive
                 );
      if (EFI_ERROR (Status)) {
        if (!IsTableEntry (*Entry)) {
          //
          // We are creating a new table entry, so on failure, we can free all
          // allocations we made recursively, given that the whole subhierarchy
          // has not been wired into the live page tables yet. (This is not
          // possible for existing table entries, since we cannot revert the
          // modifications we made to the subhierarchy it represents.)
          //
          FreePageTablesRecursive (TranslationTable, Level + 1);
        }

        return Status;
      }

      if (!IsTableEntry (*Entry)) {
        EntryValue = SetPpnToPte (0, (UINTN)TranslationTable, Level);
        EntryValue = SetTableEntry (EntryValue);
        ReplaceTableEntry (
          Entry,
          EntryValue,
          RegionStart,
          TableIsLive
          );
      }
    } else {
      EntryValue = (*Entry & ~AttributeClearMask) | AttributeSetMask;
      //
      // We don't have page fault exception handler when a virtual page is accessed and
      // the A bit is clear, or is written and the D bit is clear.
      // So just set A for read and D for write permission.
      //
      if (AttributeSetMask & RISCV_PG_R) {
        EntryValue |= RISCV_PG_A;
      }

      if (AttributeSetMask & RISCV_PG_W) {
        EntryValue |= RISCV_PG_D;
      }

      EntryValue = SetPpnToPte (EntryValue, RegionStart, Level);
      EntryValue = SetValidPte (EntryValue);
      ReplaceTableEntry (Entry, EntryValue, RegionStart, TableIsLive);
    }
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
UpdateRegionMapping (
  IN  UINTN    RegionStart,
  IN  UINTN    RegionLength,
  IN  UINTN    AttributeSetMask,
  IN  UINTN    AttributeClearMask,
  IN  UINTN    *RootTable,
  IN  BOOLEAN  TableIsLive
  )
{
  if (((RegionStart | RegionLength) & EFI_PAGE_MASK) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  return UpdateRegionMappingRecursive (
           RegionStart,
           RegionStart + RegionLength,
           AttributeSetMask,
           AttributeClearMask,
           RootTable,
           0,
           TableIsLive
           );
}

STATIC
UINTN
GcdAttributeToPageAttribute (
  IN UINTN  GcdAttributes
  )
{
  UINTN  RiscVAttributes = RISCV_PG_R | RISCV_PG_W | RISCV_PG_X;

  // Determine protection attributes
  if (GcdAttributes & EFI_MEMORY_RO) {
    RiscVAttributes &= ~(RISCV_PG_W);
  }

  // Process eXecute Never attribute
  if (GcdAttributes & EFI_MEMORY_XP) {
    RiscVAttributes &= ~RISCV_PG_X;
  }

  return RiscVAttributes;
}

EFI_STATUS
EFIAPI
RiscVSetMemoryAttributes (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINTN                 Length,
  IN UINTN                 Attributes
  )
{
  UINTN  PageAttributesSet = GcdAttributeToPageAttribute (Attributes);

  if (!RiscVMmuEnabled ()) {
    return EFI_SUCCESS;
  }

  DEBUG (
    (
     DEBUG_VERBOSE,
     "%a: Set %llX page attribute 0x%X\n",
     __func__,
     BaseAddress,
     PageAttributesSet
    )
    );

  return UpdateRegionMapping (
           BaseAddress,
           Length,
           PageAttributesSet,
           PTE_ATTRIBUTES_MASK,
           (UINTN *)RiscVGetRootTranslateTable (),
           TRUE
           );
}

STATIC
EFI_STATUS
RiscVMmuSetSatpMode  (
  UINTN  SatpMode
  )
{
  VOID                             *TranslationTable;
  UINTN                            SatpReg;
  UINTN                            Ppn;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *MemoryMap;
  UINTN                            NumberOfDescriptors;
  UINTN                            Index;
  EFI_STATUS                       Status;

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

  ZeroMem (TranslationTable, mTableEntryCount * sizeof (UINTN));

  NumberOfDescriptors = 0;
  MemoryMap           = NULL;
  Status              = gDS->GetMemorySpaceMap (
                               &NumberOfDescriptors,
                               &MemoryMap
                               );
  ASSERT_EFI_ERROR (Status);

  for (Index = 0; Index < NumberOfDescriptors; Index++) {
    if (MemoryMap[Index].GcdMemoryType == EfiGcdMemoryTypeMemoryMappedIo) {
      // Default Read/Write attribute for memory mapped IO
      UpdateRegionMapping (
        MemoryMap[Index].BaseAddress,
        MemoryMap[Index].Length,
        RISCV_PG_R | RISCV_PG_W,
        PTE_ATTRIBUTES_MASK,
        TranslationTable,
        FALSE
        );
    } else if (MemoryMap[Index].GcdMemoryType == EfiGcdMemoryTypeSystemMemory) {
      // Default Read/Write/Execute attribute for system memory
      UpdateRegionMapping (
        MemoryMap[Index].BaseAddress,
        MemoryMap[Index].Length,
        RISCV_PG_R | RISCV_PG_W | RISCV_PG_X,
        PTE_ATTRIBUTES_MASK,
        TranslationTable,
        FALSE
        );
    }
  }

  FreePool ((VOID *)MemoryMap);

  if (GetInterruptState ()) {
    DisableInterrupts ();
  }

  Ppn = (UINTN)TranslationTable >> RISCV_MMU_PAGE_SHIFT;
  ASSERT (!(Ppn & ~(SATP64_PPN)));

  SatpReg  = Ppn;
  SatpReg |= (SatpMode <<
              SATP64_MODE_SHIFT) & SATP64_MODE;
  RiscVSetSupervisorAddressTranslationRegister (SatpReg);
  /* Check if HW support the setup satp mode */
  if (SatpReg != RiscVGetSupervisorAddressTranslationRegister ()) {
    DEBUG (
      (
       DEBUG_VERBOSE,
       "%a: HW does not support SATP mode:%d\n",
       __func__,
       SatpMode
      )
      );
    FreePageTablesRecursive (TranslationTable, 0);
    return EFI_DEVICE_ERROR;
  }

  RiscVLocalTlbFlushAll ();

  if (GetInterruptState ()) {
    EnableInterrupts ();
  }

  return Status;
}

EFI_STATUS
EFIAPI
RiscVConfigureMmu (
  VOID
  )
{
  EFI_STATUS  Status        = EFI_SUCCESS;
  INTN        ModeSupport[] = { SATP_MODE_SV57, SATP_MODE_SV48, SATP_MODE_SV39 };
  INTN        Idx;

  /* Try to setup MMU with highest mode as possible */
  for (Idx = 0; Idx < ARRAY_SIZE (ModeSupport); Idx++) {
    Status = RiscVMmuSetSatpMode (ModeSupport[Idx]);
    if (Status == EFI_DEVICE_ERROR) {
      continue;
    } else if (EFI_ERROR (Status)) {
      return Status;
    }

    DEBUG (
      (
       DEBUG_INFO,
       "%a: SATP mode %d successfully configured\n",
       __func__,
       ModeSupport[Idx]
      )
      );
    break;
  }

  return Status;
}
