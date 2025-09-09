/** @file
  IGVM Parameter parsing

  Copyright (c) 2025, Red Hat. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <PiPei.h>
#include <IndustryStandard/IgvmData.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HobLib.h>
#include <Library/PlatformInitLib.h>
#include <Library/DebugLib.h>
#include <Pi/PiHob.h>

/* keep in sync with OvmfPkg/ResetVector/X64/IgvmMetadata.asm */
#define MEMORY_MAP_OFFSET   0
#define MEMORY_MAP_ENTRIES  8
#define MEMORY_MAP_SIZE     (MEMORY_MAP_ENTRIES * sizeof(IGVM_MEMORY_MAP_ENTRY))

#define IGVM_MM_ENTRY_TYPE_MEMORY      0x00
#define IGVM_MM_ENTRY_TYPE_RESERVED    0x01
#define IGVM_MM_ENTRY_TYPE_PERSISTENT  0x02

typedef struct IGVM_MEMORY_MAP_ENTRY {
  UINT64    PageStart;
  UINT64    PageCount;
  UINT16    EntryType;
  UINT16    EntryFlags;
  UINT32    Reserved;
} IGVM_MEMORY_MAP_ENTRY;

STATIC
IGVM_MEMORY_MAP_ENTRY *
EFIAPI
PlatformIgvmMemoryMapFind (
  VOID
  )
{
  UINT64                 Address;
  IGVM_MEMORY_MAP_ENTRY  *Map;

  Address = FixedPcdGet64 (PcdOvmfIgvmParamBase);
  if (Address == 0) {
    // no parameter area
    return NULL;
  }

  Map = (VOID *)(UINTN)(Address + MEMORY_MAP_OFFSET);
  if (Map[0].PageCount == 0) {
    // no memory map entries
    return NULL;
  }

  return Map;
}

BOOLEAN
EFIAPI
PlatformIgvmMemoryMapCheck (
  VOID
  )
{
  return PlatformIgvmMemoryMapFind () != NULL;
}

EFI_STATUS
EFIAPI
PlatformIgvmScanE820 (
  IN      E820_SCAN_CALLBACK     Callback,
  IN OUT  EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  )
{
  STATIC BOOLEAN         First = TRUE;
  IGVM_MEMORY_MAP_ENTRY  *Map;
  EFI_E820_ENTRY64       Entry;
  UINT32                 Index;

  Map = PlatformIgvmMemoryMapFind ();

  if (Map == NULL) {
    return EFI_NOT_FOUND;
  }

  for (Index = 0; Map[Index].PageCount > 0; Index++) {
    if (First) {
      DEBUG ((
        DEBUG_INFO,
        "%a: IgvmMemoryMap[%d]: PageStart %lx, PageCount %lx, EntryType %d\n",
        __func__,
        Index,
        Map[Index].PageStart,
        Map[Index].PageCount,
        Map[Index].EntryType
        ));
    }

    Entry.BaseAddr = LShiftU64 (Map[Index].PageStart, EFI_PAGE_SHIFT);
    Entry.Length   = LShiftU64 (Map[Index].PageCount, EFI_PAGE_SHIFT);
    switch (Map[Index].EntryType) {
      case IGVM_MM_ENTRY_TYPE_MEMORY:
        Entry.Type = EfiAcpiAddressRangeMemory;
        Callback (&Entry, PlatformInfoHob);
        break;
      case IGVM_MM_ENTRY_TYPE_RESERVED:
        Entry.Type = EfiAcpiAddressRangeReserved;
        Callback (&Entry, PlatformInfoHob);
        break;
    }
  }

  First = FALSE;
  return EFI_SUCCESS;
}

VOID
EFIAPI
PlatformIgvmDataHobs (
  VOID
  )
{
  UINT64                Start, End;
  EFI_PEI_HOB_POINTERS  Hob;
  EFI_IGVM_DATA_HOB     *IgvmData;

  Start = FixedPcdGet64 (PcdOvmfIgvmHobBase);
  End   = Start + FixedPcdGet64 (PcdOvmfIgvmHobSize);
  if (Start == 0) {
    // no parameter area
    return;
  }

  for (Hob.Raw = (VOID *)(UINTN)Start;
       ;
       Hob.Raw = GET_NEXT_HOB (Hob))
  {
    DEBUG ((
      DEBUG_VERBOSE,
      "%a: hob: ptr=%p type=0x%x length=%u\n",
      __func__,
      Hob.Raw,
      Hob.Header->HobType,
      Hob.Header->HobLength
      ));

    if (Hob.Header->HobType != EFI_HOB_TYPE_GUID_EXTENSION) {
      break;
    }

    if ((UINTN)Hob.Raw + Hob.Header->HobLength > End) {
      break;
    }

    if (CompareMem (&Hob.Guid->Name, &gEfiIgvmDataHobGuid, sizeof (EFI_GUID)) != 0) {
      break;
    }

    IgvmData = (VOID *)(&Hob.Guid->Name + 1);
    DEBUG ((
      DEBUG_INFO,
      "%a: data: address=0x%x length=0x%x type=0x%x flags=0x%x\n",
      __func__,
      IgvmData->Address,
      IgvmData->Length,
      IgvmData->DataType,
      IgvmData->DataFlags
      ));

    // copy over hob so dxe can easily use it
    BuildGuidDataHob (&gEfiIgvmDataHobGuid, IgvmData, sizeof (*IgvmData));

    // reserve data block memory
    BuildMemoryAllocationHob (
      IgvmData->Address,
      ALIGN_VALUE (IgvmData->Length, EFI_PAGE_SIZE),
      EfiBootServicesData
      );
  }
}
