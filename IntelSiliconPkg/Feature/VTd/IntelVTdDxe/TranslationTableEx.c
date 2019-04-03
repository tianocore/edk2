/** @file

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DmaProtection.h"

/**
  Create extended context entry.

  @param[in]  VtdIndex  The index of the VTd engine.

  @retval EFI_SUCCESS          The extended context entry is created.
  @retval EFI_OUT_OF_RESOURCE  No enough resource to create extended context entry.
**/
EFI_STATUS
CreateExtContextEntry (
  IN UINTN  VtdIndex
  )
{
  UINTN                  Index;
  VOID                   *Buffer;
  UINTN                  RootPages;
  UINTN                  ContextPages;
  VTD_EXT_ROOT_ENTRY     *ExtRootEntry;
  VTD_EXT_CONTEXT_ENTRY  *ExtContextEntryTable;
  VTD_EXT_CONTEXT_ENTRY  *ExtContextEntry;
  VTD_SOURCE_ID          *PciSourceId;
  VTD_SOURCE_ID          SourceId;
  UINTN                  MaxBusNumber;
  UINTN                  EntryTablePages;

  MaxBusNumber = 0;
  for (Index = 0; Index < mVtdUnitInformation[VtdIndex].PciDeviceInfo.PciDeviceDataNumber; Index++) {
    PciSourceId = &mVtdUnitInformation[VtdIndex].PciDeviceInfo.PciDeviceData[Index].PciSourceId;
    if (PciSourceId->Bits.Bus > MaxBusNumber) {
      MaxBusNumber = PciSourceId->Bits.Bus;
    }
  }
  DEBUG ((DEBUG_INFO,"  MaxBusNumber - 0x%x\n", MaxBusNumber));

  RootPages = EFI_SIZE_TO_PAGES (sizeof (VTD_EXT_ROOT_ENTRY) * VTD_ROOT_ENTRY_NUMBER);
  ContextPages = EFI_SIZE_TO_PAGES (sizeof (VTD_EXT_CONTEXT_ENTRY) * VTD_CONTEXT_ENTRY_NUMBER);
  EntryTablePages = RootPages + ContextPages * (MaxBusNumber + 1);
  Buffer = AllocateZeroPages (EntryTablePages);
  if (Buffer == NULL) {
    DEBUG ((DEBUG_INFO,"Could not Alloc Root Entry Table.. \n"));
    return EFI_OUT_OF_RESOURCES;
  }
  mVtdUnitInformation[VtdIndex].ExtRootEntryTable = (VTD_EXT_ROOT_ENTRY *)Buffer;
  Buffer = (UINT8 *)Buffer + EFI_PAGES_TO_SIZE (RootPages);

  for (Index = 0; Index < mVtdUnitInformation[VtdIndex].PciDeviceInfo.PciDeviceDataNumber; Index++) {
    PciSourceId = &mVtdUnitInformation[VtdIndex].PciDeviceInfo.PciDeviceData[Index].PciSourceId;

    SourceId.Bits.Bus = PciSourceId->Bits.Bus;
    SourceId.Bits.Device = PciSourceId->Bits.Device;
    SourceId.Bits.Function = PciSourceId->Bits.Function;

    ExtRootEntry = &mVtdUnitInformation[VtdIndex].ExtRootEntryTable[SourceId.Index.RootIndex];
    if (ExtRootEntry->Bits.LowerPresent == 0) {
      ExtRootEntry->Bits.LowerContextTablePointerLo  = (UINT32) RShiftU64 ((UINT64)(UINTN)Buffer, 12);
      ExtRootEntry->Bits.LowerContextTablePointerHi  = (UINT32) RShiftU64 ((UINT64)(UINTN)Buffer, 32);
      ExtRootEntry->Bits.LowerPresent = 1;
      ExtRootEntry->Bits.UpperContextTablePointerLo  = (UINT32) RShiftU64 ((UINT64)(UINTN)Buffer, 12) + 1;
      ExtRootEntry->Bits.UpperContextTablePointerHi  = (UINT32) RShiftU64 (RShiftU64 ((UINT64)(UINTN)Buffer, 12) + 1, 20);
      ExtRootEntry->Bits.UpperPresent = 1;
      Buffer = (UINT8 *)Buffer + EFI_PAGES_TO_SIZE (ContextPages);
    }

    ExtContextEntryTable = (VTD_EXT_CONTEXT_ENTRY *)(UINTN)VTD_64BITS_ADDRESS(ExtRootEntry->Bits.LowerContextTablePointerLo, ExtRootEntry->Bits.LowerContextTablePointerHi) ;
    ExtContextEntry = &ExtContextEntryTable[SourceId.Index.ContextIndex];
    ExtContextEntry->Bits.TranslationType = 0;
    ExtContextEntry->Bits.FaultProcessingDisable = 0;
    ExtContextEntry->Bits.Present = 0;

    DEBUG ((DEBUG_INFO,"DOMAIN: S%04x, B%02x D%02x F%02x\n", mVtdUnitInformation[VtdIndex].Segment, SourceId.Bits.Bus, SourceId.Bits.Device, SourceId.Bits.Function));

    switch (mVtdUnitInformation[VtdIndex].CapReg.Bits.SAGAW) {
    case BIT1:
      ExtContextEntry->Bits.AddressWidth = 0x1;
      break;
    case BIT2:
      ExtContextEntry->Bits.AddressWidth = 0x2;
      break;
    }
  }

  FlushPageTableMemory (VtdIndex, (UINTN)mVtdUnitInformation[VtdIndex].ExtRootEntryTable, EFI_PAGES_TO_SIZE(EntryTablePages));

  return EFI_SUCCESS;
}

/**
  Dump DMAR extended context entry table.

  @param[in]  ExtRootEntry DMAR extended root entry.
**/
VOID
DumpDmarExtContextEntryTable (
  IN VTD_EXT_ROOT_ENTRY *ExtRootEntry
  )
{
  UINTN                 Index;
  UINTN                 Index2;
  VTD_EXT_CONTEXT_ENTRY *ExtContextEntry;

  DEBUG ((DEBUG_INFO,"=========================\n"));
  DEBUG ((DEBUG_INFO,"DMAR ExtContext Entry Table:\n"));

  DEBUG ((DEBUG_INFO,"ExtRootEntry Address - 0x%x\n", ExtRootEntry));

  for (Index = 0; Index < VTD_ROOT_ENTRY_NUMBER; Index++) {
    if ((ExtRootEntry[Index].Uint128.Uint64Lo != 0) || (ExtRootEntry[Index].Uint128.Uint64Hi != 0)) {
      DEBUG ((DEBUG_INFO,"  ExtRootEntry(0x%02x) B%02x - 0x%016lx %016lx\n",
        Index, Index, ExtRootEntry[Index].Uint128.Uint64Hi, ExtRootEntry[Index].Uint128.Uint64Lo));
    }
    if (ExtRootEntry[Index].Bits.LowerPresent == 0) {
      continue;
    }
    ExtContextEntry = (VTD_EXT_CONTEXT_ENTRY *)(UINTN)VTD_64BITS_ADDRESS(ExtRootEntry[Index].Bits.LowerContextTablePointerLo, ExtRootEntry[Index].Bits.LowerContextTablePointerHi);
    for (Index2 = 0; Index2 < VTD_CONTEXT_ENTRY_NUMBER/2; Index2++) {
      if ((ExtContextEntry[Index2].Uint256.Uint64_1 != 0) || (ExtContextEntry[Index2].Uint256.Uint64_2 != 0) ||
          (ExtContextEntry[Index2].Uint256.Uint64_3 != 0) || (ExtContextEntry[Index2].Uint256.Uint64_4 != 0)) {
        DEBUG ((DEBUG_INFO,"    ExtContextEntryLower(0x%02x) D%02xF%02x - 0x%016lx %016lx %016lx %016lx\n",
          Index2, Index2 >> 3, Index2 & 0x7, ExtContextEntry[Index2].Uint256.Uint64_4, ExtContextEntry[Index2].Uint256.Uint64_3, ExtContextEntry[Index2].Uint256.Uint64_2, ExtContextEntry[Index2].Uint256.Uint64_1));
      }
      if (ExtContextEntry[Index2].Bits.Present == 0) {
        continue;
      }
      DumpSecondLevelPagingEntry ((VOID *)(UINTN)VTD_64BITS_ADDRESS(ExtContextEntry[Index2].Bits.SecondLevelPageTranslationPointerLo, ExtContextEntry[Index2].Bits.SecondLevelPageTranslationPointerHi));
    }

    if (ExtRootEntry[Index].Bits.UpperPresent == 0) {
      continue;
    }
    ExtContextEntry = (VTD_EXT_CONTEXT_ENTRY *)(UINTN)VTD_64BITS_ADDRESS(ExtRootEntry[Index].Bits.UpperContextTablePointerLo, ExtRootEntry[Index].Bits.UpperContextTablePointerHi);
    for (Index2 = 0; Index2 < VTD_CONTEXT_ENTRY_NUMBER/2; Index2++) {
      if ((ExtContextEntry[Index2].Uint256.Uint64_1 != 0) || (ExtContextEntry[Index2].Uint256.Uint64_2 != 0) ||
          (ExtContextEntry[Index2].Uint256.Uint64_3 != 0) || (ExtContextEntry[Index2].Uint256.Uint64_4 != 0)) {
        DEBUG ((DEBUG_INFO,"    ExtContextEntryUpper(0x%02x) D%02xF%02x - 0x%016lx %016lx %016lx %016lx\n",
          Index2, (Index2 + 128) >> 3, (Index2 + 128) & 0x7, ExtContextEntry[Index2].Uint256.Uint64_4, ExtContextEntry[Index2].Uint256.Uint64_3, ExtContextEntry[Index2].Uint256.Uint64_2, ExtContextEntry[Index2].Uint256.Uint64_1));
      }
      if (ExtContextEntry[Index2].Bits.Present == 0) {
        continue;
      }
    }
  }
  DEBUG ((DEBUG_INFO,"=========================\n"));
}
