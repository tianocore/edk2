/** @file
*
*  Copyright (c) 2011-2012, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include <PiDxe.h>
#include <Library/UefiLib.h>
#include <Library/ArmLib.h>
#include <Chipset/ArmV7.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/EblCmdLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

#define GET_TT_ATTRIBUTES(TTEntry)  ((TTEntry) & ~(TT_DESCRIPTOR_SECTION_BASE_ADDRESS_MASK))
#define GET_TT_PAGE_ATTRIBUTES(TTEntry)  ((TTEntry) & 0xFFF)
#define GET_TT_LARGEPAGE_ATTRIBUTES(TTEntry)  ((TTEntry) & 0xFFFF)

// Section
#define TT_DESCRIPTOR_SECTION_STRONGLY_ORDER   (TT_DESCRIPTOR_SECTION_TYPE_SECTION    | \
                                                TT_DESCRIPTOR_SECTION_NG_GLOBAL       | \
                                                TT_DESCRIPTOR_SECTION_S_NOT_SHARED    | \
                                                TT_DESCRIPTOR_SECTION_DOMAIN(0)       | \
                                                TT_DESCRIPTOR_SECTION_AP_RW_RW        | \
                                                TT_DESCRIPTOR_SECTION_CACHE_POLICY_STRONGLY_ORDERED)

// Small Page
#define TT_DESCRIPTOR_PAGE_STRONGLY_ORDER      (TT_DESCRIPTOR_PAGE_TYPE_PAGE          | \
                                                TT_DESCRIPTOR_PAGE_NG_GLOBAL          | \
                                                TT_DESCRIPTOR_PAGE_S_NOT_SHARED       | \
                                                TT_DESCRIPTOR_PAGE_AP_RW_RW           | \
                                                TT_DESCRIPTOR_PAGE_CACHE_POLICY_STRONGLY_ORDERED)

// Large Page
#define TT_DESCRIPTOR_LARGEPAGE_WRITE_BACK     (TT_DESCRIPTOR_PAGE_TYPE_LARGEPAGE     | \
                                                TT_DESCRIPTOR_PAGE_NG_GLOBAL          | \
                                                TT_DESCRIPTOR_PAGE_S_NOT_SHARED       | \
                                                TT_DESCRIPTOR_PAGE_AP_RW_RW           | \
                                                TT_DESCRIPTOR_SECTION_CACHE_POLICY_WRITE_BACK_ALLOC)
#define TT_DESCRIPTOR_LARGEPAGE_WRITE_THROUGH  (TT_DESCRIPTOR_PAGE_TYPE_LARGEPAGE     | \
                                                TT_DESCRIPTOR_PAGE_NG_GLOBAL          | \
                                                TT_DESCRIPTOR_PAGE_S_NOT_SHARED       | \
                                                TT_DESCRIPTOR_PAGE_AP_RW_RW           | \
                                                TT_DESCRIPTOR_SECTION_CACHE_POLICY_WRITE_THROUGH_NO_ALLOC)
#define TT_DESCRIPTOR_LARGEPAGE_DEVICE         (TT_DESCRIPTOR_PAGE_TYPE_LARGEPAGE     | \
                                                TT_DESCRIPTOR_PAGE_NG_GLOBAL          | \
                                                TT_DESCRIPTOR_PAGE_S_NOT_SHARED       | \
                                                TT_DESCRIPTOR_PAGE_AP_RW_RW           | \
                                                TT_DESCRIPTOR_SECTION_CACHE_POLICY_SHAREABLE_DEVICE)
#define TT_DESCRIPTOR_LARGEPAGE_UNCACHED       (TT_DESCRIPTOR_PAGE_TYPE_LARGEPAGE     | \
                                                TT_DESCRIPTOR_PAGE_NG_GLOBAL          | \
                                                TT_DESCRIPTOR_PAGE_S_NOT_SHARED       | \
                                                TT_DESCRIPTOR_PAGE_AP_RW_RW           | \
                                                TT_DESCRIPTOR_SECTION_CACHE_POLICY_NON_CACHEABLE)


typedef enum { Level0, Level1,Level2 } MMU_LEVEL;

typedef struct {
  MMU_LEVEL   Level;
  UINT32      Value;
  UINT32      Index;
  UINT32*     Table;
} MMU_ENTRY;

MMU_ENTRY
MmuEntryCreate (
  IN MMU_LEVEL Level,
  IN UINT32* Table,
  IN UINT32 Index
  )
{
  MMU_ENTRY Entry;
  Entry.Level = Level;
  Entry.Value = Table[Index];
  Entry.Table = Table;
  Entry.Index = Index;
  return Entry;
}

UINT32
MmuEntryIsValidAddress (
  IN MMU_LEVEL Level,
  IN UINT32 Entry
  )
{
  if (Level == Level0) {
    return 0;
  } else if (Level == Level1) {
    if ((Entry & 0x3) == 0) {           // Ignored
      return 0;
    } else if ((Entry & 0x3) == 2) {    // Section Type
      return 1;
    } else {                            // Page Type
      return 0;
    }
  } else if (Level == Level2){
    if ((Entry & 0x3) == 0) {           // Ignored
      return 0;
    } else {                            // Page Type
      return 1;
    }
  } else {
    DEBUG((EFI_D_ERROR,"MmuEntryIsValidAddress: Level:%d Entry:0x%X\n",(UINT32)Level,(UINT32)Entry));
    ASSERT(0);
    return 0;
  }
}

UINT32
MmuEntryGetAddress (
  IN MMU_ENTRY Entry
  )
{
  if (Entry.Level == Level1) {
    if ((Entry.Value & 0x3) == 0) {
      return 0;
    } else if ((Entry.Value & 0x3) == 2) {    // Section Type
      return Entry.Value & TT_DESCRIPTOR_SECTION_BASE_ADDRESS_MASK;
    } else if ((Entry.Value & 0x3) == 1) {    // Level2 Table
      MMU_ENTRY Level2Entry = MmuEntryCreate (Level2,(UINT32*)(Entry.Value & 0xFFFFC000),0);
      return MmuEntryGetAddress (Level2Entry);
    } else {                                  // Page Type
      return 0;
    }
  } else if (Entry.Level == Level2) {
    if ((Entry.Value & 0x3) == 0) {           // Ignored
      return 0;
    } else if ((Entry.Value & 0x3) == 1) {    // Large Page
      return Entry.Value & 0xFFFF0000;
    } else if ((Entry.Value & 0x2) == 2) {    // Small Page
      return Entry.Value & 0xFFFFF000;
    } else {
      return 0;
    }
  } else {
    ASSERT(0);
    return 0;
  }
}

UINT32
MmuEntryGetSize (
  IN MMU_ENTRY Entry
  )
{
  if (Entry.Level == Level1) {
    if ((Entry.Value & 0x3) == 0) {
      return 0;
    } else if ((Entry.Value & 0x3) == 2) {
      if (Entry.Value & (1 << 18))
        return 16*SIZE_1MB;
      else
        return SIZE_1MB;
    } else if ((Entry.Value & 0x3) == 1) {      // Level2 Table split 1MB section
      return SIZE_1MB;
    } else {
      DEBUG((EFI_D_ERROR, "MmuEntryGetSize: Value:0x%X",Entry.Value));
      ASSERT(0);
      return 0;
    }
  } else if (Entry.Level == Level2) {
    if ((Entry.Value & 0x3) == 0) {           // Ignored
      return 0;
    } else if ((Entry.Value & 0x3) == 1) {    // Large Page
      return SIZE_64KB;
    } else if ((Entry.Value & 0x2) == 2) {    // Small Page
      return SIZE_4KB;
    } else {
      ASSERT(0);
      return 0;
    }
  } else {
    ASSERT(0);
    return 0;
  }
}

CONST CHAR8*
MmuEntryGetAttributesName (
  IN MMU_ENTRY Entry
  )
{
  UINT32 Value;

  if (Entry.Level == Level1) {
    Value = GET_TT_ATTRIBUTES(Entry.Value) | TT_DESCRIPTOR_SECTION_NS_MASK;
    if (Value == TT_DESCRIPTOR_SECTION_WRITE_BACK(0))
      return "TT_DESCRIPTOR_SECTION_WRITE_BACK";
    else if (Value == TT_DESCRIPTOR_SECTION_WRITE_THROUGH(0))
      return "TT_DESCRIPTOR_SECTION_WRITE_THROUGH";
    else if (Value == TT_DESCRIPTOR_SECTION_DEVICE(0))
      return "TT_DESCRIPTOR_SECTION_DEVICE";
    else if (Value == TT_DESCRIPTOR_SECTION_UNCACHED(0))
      return "TT_DESCRIPTOR_SECTION_UNCACHED";
    else if (Value == TT_DESCRIPTOR_SECTION_STRONGLY_ORDER)
      return "TT_DESCRIPTOR_SECTION_STRONGLY_ORDERED";
    else {
      return "SectionUnknown";
    }
  } else if ((Entry.Level == Level2) && ((Entry.Value & 0x2) == 2)) { //Small Page
    Value = GET_TT_PAGE_ATTRIBUTES(Entry.Value);
    if (Value == TT_DESCRIPTOR_PAGE_WRITE_BACK)
      return "TT_DESCRIPTOR_PAGE_WRITE_BACK";
    else if (Value == TT_DESCRIPTOR_PAGE_WRITE_THROUGH)
      return "TT_DESCRIPTOR_PAGE_WRITE_THROUGH";
    else if (Value == TT_DESCRIPTOR_PAGE_DEVICE)
      return "TT_DESCRIPTOR_PAGE_DEVICE";
    else if (Value == TT_DESCRIPTOR_PAGE_UNCACHED)
      return "TT_DESCRIPTOR_PAGE_UNCACHED";
    else if (Value == TT_DESCRIPTOR_PAGE_STRONGLY_ORDER)
      return "TT_DESCRIPTOR_PAGE_STRONGLY_ORDERED";
    else {
      return "PageUnknown";
    }
  } else if ((Entry.Level == Level2) && ((Entry.Value & 0x3) == 1)) { //Large Page
    Value = GET_TT_LARGEPAGE_ATTRIBUTES(Entry.Value);
    if (Value == TT_DESCRIPTOR_LARGEPAGE_WRITE_BACK)
      return "TT_DESCRIPTOR_LARGEPAGE_WRITE_BACK";
    else if (Value == TT_DESCRIPTOR_LARGEPAGE_WRITE_THROUGH)
      return "TT_DESCRIPTOR_LARGEPAGE_WRITE_THROUGH";
    else if (Value == TT_DESCRIPTOR_LARGEPAGE_DEVICE)
      return "TT_DESCRIPTOR_LARGEPAGE_DEVICE";
    else if (Value == TT_DESCRIPTOR_LARGEPAGE_UNCACHED)
      return "TT_DESCRIPTOR_LARGEPAGE_UNCACHED";
    else {
      return "LargePageUnknown";
    }
  } else {
    ASSERT(0);
    return "";
  }
}

UINT32
MmuEntryGetAttributes (
  IN MMU_ENTRY Entry
  )
{
  if (Entry.Level == Level1) {
    if ((Entry.Value & 0x3) == 0) {
      return 0;
    } else if ((Entry.Value & 0x3) == 2) {
      return GET_TT_ATTRIBUTES(Entry.Value);
    } else {
      return 0;
    }
  } else if ((Entry.Level == Level2) && ((Entry.Value & 0x2) == 2)) { //Small Page
    if (GET_TT_PAGE_ATTRIBUTES(Entry.Value) == TT_DESCRIPTOR_PAGE_WRITE_BACK)
      return TT_DESCRIPTOR_SECTION_WRITE_BACK(0);
    else if (GET_TT_PAGE_ATTRIBUTES(Entry.Value) == TT_DESCRIPTOR_PAGE_WRITE_THROUGH)
      return TT_DESCRIPTOR_SECTION_WRITE_THROUGH(0);
    else if (GET_TT_PAGE_ATTRIBUTES(Entry.Value) == TT_DESCRIPTOR_PAGE_DEVICE)
      return TT_DESCRIPTOR_SECTION_DEVICE(0);
    else if (GET_TT_PAGE_ATTRIBUTES(Entry.Value) == TT_DESCRIPTOR_PAGE_UNCACHED)
      return TT_DESCRIPTOR_SECTION_UNCACHED(0);
    else if (GET_TT_PAGE_ATTRIBUTES(Entry.Value) == TT_DESCRIPTOR_PAGE_STRONGLY_ORDER)
      return TT_DESCRIPTOR_SECTION_STRONGLY_ORDER;
    else {
      return 0;
    }
  } else if ((Entry.Level == Level2) && ((Entry.Value & 0x3) == 1)) { //Large Page
    if (GET_TT_LARGEPAGE_ATTRIBUTES(Entry.Value) == TT_DESCRIPTOR_LARGEPAGE_WRITE_BACK)
      return TT_DESCRIPTOR_SECTION_WRITE_BACK(0);
    else if (GET_TT_LARGEPAGE_ATTRIBUTES(Entry.Value) == TT_DESCRIPTOR_LARGEPAGE_WRITE_THROUGH)
      return TT_DESCRIPTOR_SECTION_WRITE_THROUGH(0);
    else if (GET_TT_LARGEPAGE_ATTRIBUTES(Entry.Value) == TT_DESCRIPTOR_LARGEPAGE_DEVICE)
      return TT_DESCRIPTOR_SECTION_DEVICE(0);
    else if (GET_TT_LARGEPAGE_ATTRIBUTES(Entry.Value) == TT_DESCRIPTOR_LARGEPAGE_UNCACHED)
      return TT_DESCRIPTOR_SECTION_UNCACHED(0);
    else {
      return 0;
    }
  } else {
    return 0;
  }
}


MMU_ENTRY
DumpMmuLevel (
  IN MMU_LEVEL Level,
  IN UINT32* Table,
  IN MMU_ENTRY PreviousEntry
  )
{
  UINT32      Index = 0, Count;
  MMU_ENTRY   LastEntry, Entry;

  ASSERT((Level == Level1) || (Level == Level2));

  if (Level == Level1)    Count = 4096;
  else                    Count = 256;

  // At Level1, we will get into this function because PreviousEntry is not valid
  if (!MmuEntryIsValidAddress((MMU_LEVEL)(Level-1),PreviousEntry.Value)) {
      // Find the first valid address
      for (; (Index < Count) && (!MmuEntryIsValidAddress(Level,Table[Index])); Index++);

      LastEntry = MmuEntryCreate(Level,Table,Index);
      Index++;
  } else {
    LastEntry = PreviousEntry;
  }

  for (; Index < Count; Index++) {
    Entry = MmuEntryCreate(Level,Table,Index);
    if ((Level == Level1) && ((Entry.Value & 0x3) == 1)) {       // We have got a Level2 table redirection
      LastEntry = DumpMmuLevel(Level2,(UINT32*)(Entry.Value & 0xFFFFFC00),LastEntry);
    } else if (!MmuEntryIsValidAddress(Level,Table[Index])) {
      if (MmuEntryIsValidAddress(LastEntry.Level,LastEntry.Value)) {
          AsciiPrint("0x%08X-0x%08X\t%a\n",
              MmuEntryGetAddress(LastEntry),MmuEntryGetAddress(PreviousEntry)+MmuEntryGetSize(PreviousEntry)-1,
              MmuEntryGetAttributesName(LastEntry));
      }
      LastEntry = Entry;
    } else {
      if (MmuEntryGetAttributes(LastEntry) != MmuEntryGetAttributes(Entry)) {
          if (MmuEntryIsValidAddress(Level,LastEntry.Value)) {
            AsciiPrint("0x%08X-0x%08X\t%a\n",
                      MmuEntryGetAddress(LastEntry),MmuEntryGetAddress(PreviousEntry)+MmuEntryGetSize(PreviousEntry)-1,
                      MmuEntryGetAttributesName(LastEntry));
          }
          LastEntry = Entry;
      } else {
        ASSERT(LastEntry.Value != 0);
      }
    }
    PreviousEntry = Entry;
  }

  if ((Level == Level1) && (LastEntry.Index != Index) && MmuEntryIsValidAddress(Level,LastEntry.Value)) {
    AsciiPrint("0x%08X-0x%08X\t%a\n",
                  MmuEntryGetAddress(LastEntry),MmuEntryGetAddress(PreviousEntry)+MmuEntryGetSize(PreviousEntry)-1,
                  MmuEntryGetAttributesName(LastEntry));
  }

  return LastEntry;
}


EFI_STATUS
EblDumpMmu (
  IN UINTN  Argc,
  IN CHAR8  **Argv
  )
{
  UINT32  *TTEntry;
  MMU_ENTRY NoEntry;

  TTEntry = ArmGetTTBR0BaseAddress();

  AsciiPrint ("\nTranslation Table:0x%X\n",TTEntry);
  AsciiPrint ("Address Range\t\tAttributes\n");
  AsciiPrint ("____________________________________________________\n");

  NoEntry.Level = (MMU_LEVEL)200;
  DumpMmuLevel(Level1,TTEntry,NoEntry);

  return EFI_SUCCESS;
}
