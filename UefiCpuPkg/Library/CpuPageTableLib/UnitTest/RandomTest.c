/** @file
  Random test case for Unit tests of the CpuPageTableLib instance of the CpuPageTableLib class

  Copyright (c) 2022 - 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CpuPageTableLibUnitTest.h"
#include "RandomTest.h"

UINTN                     RandomNumber = 0;
extern IA32_PAGING_ENTRY  mValidMaskNoLeaf[6];
extern IA32_PAGING_ENTRY  mValidMaskLeaf[6];
extern IA32_PAGING_ENTRY  mValidMaskLeafFlag[6];
UINTN                     mRandomOption;
IA32_MAP_ATTRIBUTE        mSupportedBit;
extern UINTN              mNumberCount;
extern UINT8              mNumbers[];
UINTN                     mNumberIndex;
UINT64                    AlignedTable[] = {
  ~((UINT64)SIZE_4KB - 1),
  ~((UINT64)SIZE_2MB - 1),
  ~((UINT64)SIZE_1GB - 1)
};

/**
  Generates a pseudorandom byte stream of the specified size.

  Return FALSE to indicate this interface is not supported.

  @param[out]  Output  Pointer to buffer to receive random value.
  @param[in]   Size    Size of random bytes to generate.

  @retval TRUE   Always return TRUE

**/
BOOLEAN
EFIAPI
RandomBytesUsingArray (
  OUT  UINT8  *Output,
  IN   UINTN  Size
  )
{
  UINTN  Index;

  for (Index = 0; Index < Size; Index++) {
    if (mNumberIndex >= mNumberCount) {
      mNumberIndex = 0;
    }

    Output[Index] = mNumbers[mNumberIndex];
    mNumberIndex++;
  }

  return TRUE;
}

/**
  Generates a pseudorandom byte stream of the specified size.

  Return FALSE to indicate this interface is not supported.

  @param[out]  Output  Pointer to buffer to receive random value.
  @param[in]   Size    Size of random bytes to generate.

  @retval TRUE   Pseudorandom byte stream generated successfully.
  @retval FALSE  Pseudorandom number generator fails
**/
BOOLEAN
EFIAPI
LocalRandomBytes (
  OUT  UINT8  *Output,
  IN   UINTN  Size
  )
{
  if (mRandomOption & USE_RANDOM_ARRAY) {
    return RandomBytesUsingArray (Output, Size);
  } else {
    return RandomBytes (Output, Size);
  }
}

/**
  Return a 32bit random number.

  @param Start  Start of the random number range.
  @param Limit  Limit of the random number range, and return value can be Limit.
  @return 32bit random number
**/
UINT32
Random32 (
  UINT32  Start,
  UINT32  Limit
  )
{
  UINT64  Value;

  LocalRandomBytes ((UINT8 *)&Value, sizeof (UINT64));
  return (UINT32)(Value % (Limit - Start + 1)) + Start;
}

/**
  Return a 64bit random number.

  @param Start  Start of the random number range.
  @param Limit  Limit of the random number range, and return value can be Limit.
  @return 64bit random number
**/
UINT64
Random64 (
  UINT64  Start,
  UINT64  Limit
  )
{
  UINT64  Value;

  LocalRandomBytes ((UINT8 *)&Value, sizeof (UINT64));
  if (Limit - Start  == MAX_UINT64) {
    return (UINT64)(Value);
  }

  return (UINT64)(Value % (Limit - Start  + 1)) + Start;
}

/**
  Returns true with the percentage of input Probability.

  @param[in]   Probability    The percentage to return true.

  @return boolean
**/
BOOLEAN
RandomBoolean (
  UINT8  Probability
  )
{
  return ((Probability > ((UINT8)Random64 (0, 100))) ? TRUE : FALSE);
}

/**
  Set 8K stack as random value.
**/
VOID
SetRandomStack (
  VOID
  )
{
  UINT64  Buffer[SIZE_1KB];
  UINTN   Index;

  for (Index = 0; Index < SIZE_1KB; Index++) {
    Buffer[Index] = Random64 (0, MAX_UINT64);
    Buffer[Index] = Buffer[Index];
  }
}

/**
  Check if the Page table entry is valid

  @param[in]   PagingEntry    The entry in page table to verify
  @param[in]   Level          the level of PagingEntry.
  @param[in]   MaxLeafLevel   Max leaf entry level.
  @param[in]   LinearAddress  The linear address verified.

  @retval  Leaf entry.
**/
UNIT_TEST_STATUS
ValidateAndRandomeModifyPageTablePageTableEntry (
  IN IA32_PAGING_ENTRY  *PagingEntry,
  IN UINTN              Level,
  IN UINTN              MaxLeafLevel,
  IN UINT64             Address
  )
{
  UINT64             Index;
  UINT32             PageTableBaseAddressLow;
  UINT32             PageTableBaseAddressHigh;
  IA32_PAGING_ENTRY  *ChildPageEntry;
  UNIT_TEST_STATUS   Status;

  if (PagingEntry->Pce.Present == 0) {
    return UNIT_TEST_PASSED;
  }

  if ((PagingEntry->Uint64 & mValidMaskLeafFlag[Level].Uint64) == mValidMaskLeafFlag[Level].Uint64) {
    //
    // It is a Leaf
    //
    if (Level > MaxLeafLevel) {
      UT_ASSERT_TRUE (Level <= MaxLeafLevel);
    }

    if ((PagingEntry->Uint64 & mValidMaskLeaf[Level].Uint64) != PagingEntry->Uint64) {
      UT_ASSERT_EQUAL ((PagingEntry->Uint64 & mValidMaskLeaf[Level].Uint64), PagingEntry->Uint64);
    }

    if ((RandomNumber < 100) && RandomBoolean (50)) {
      RandomNumber++;
      if (Level == 1) {
        PageTableBaseAddressLow  = PagingEntry->Pte4K.Bits.PageTableBaseAddressLow;
        PageTableBaseAddressHigh = PagingEntry->Pte4K.Bits.PageTableBaseAddressHigh;
      } else {
        PageTableBaseAddressLow  = PagingEntry->PleB.Bits.PageTableBaseAddressLow;
        PageTableBaseAddressHigh = PagingEntry->PleB.Bits.PageTableBaseAddressHigh;
      }

      PagingEntry->Uint64             = (Random64 (0, MAX_UINT64) & mValidMaskLeaf[Level].Uint64) | mValidMaskLeafFlag[Level].Uint64;
      PagingEntry->Pte4K.Bits.Present = 1;
      if (Level == 1) {
        PagingEntry->Pte4K.Bits.PageTableBaseAddressLow  = PageTableBaseAddressLow;
        PagingEntry->Pte4K.Bits.PageTableBaseAddressHigh = PageTableBaseAddressHigh;
      } else {
        PagingEntry->PleB.Bits.PageTableBaseAddressLow  = PageTableBaseAddressLow;
        PagingEntry->PleB.Bits.PageTableBaseAddressHigh = PageTableBaseAddressHigh;
      }

      if ((PagingEntry->Uint64 & mValidMaskLeaf[Level].Uint64) != PagingEntry->Uint64) {
        UT_ASSERT_EQUAL ((PagingEntry->Uint64 & mValidMaskLeaf[Level].Uint64), PagingEntry->Uint64);
      }
    }

    return UNIT_TEST_PASSED;
  }

  //
  // Not a leaf
  //
  UT_ASSERT_NOT_EQUAL (Level, 1);
  if ((PagingEntry->Uint64 & mValidMaskNoLeaf[Level].Uint64) != PagingEntry->Uint64) {
    DEBUG ((DEBUG_ERROR, "ERROR: Level %d no Leaf entry is 0x%lx, which reserved bit is set \n", Level, PagingEntry->Uint64));
    UT_ASSERT_EQUAL ((PagingEntry->Uint64 & mValidMaskNoLeaf[Level].Uint64), PagingEntry->Uint64);
  }

  if ((RandomNumber < 100) && RandomBoolean (50)) {
    RandomNumber++;
    PageTableBaseAddressLow  = PagingEntry->PleB.Bits.PageTableBaseAddressLow;
    PageTableBaseAddressHigh = PagingEntry->PleB.Bits.PageTableBaseAddressHigh;

    PagingEntry->Uint64                             = Random64 (0, MAX_UINT64) & mValidMaskNoLeaf[Level].Uint64;
    PagingEntry->Pnle.Bits.Present                  = 1;
    PagingEntry->PleB.Bits.PageTableBaseAddressLow  = PageTableBaseAddressLow;
    PagingEntry->PleB.Bits.PageTableBaseAddressHigh = PageTableBaseAddressHigh;
    ASSERT ((PagingEntry->Uint64 & mValidMaskLeafFlag[Level].Uint64) != mValidMaskLeafFlag[Level].Uint64);
  }

  ChildPageEntry = (IA32_PAGING_ENTRY  *)(UINTN)(IA32_PNLE_PAGE_TABLE_BASE_ADDRESS (&PagingEntry->Pnle));
  for (Index = 0; Index < 512; Index++) {
    Status = ValidateAndRandomeModifyPageTablePageTableEntry (&ChildPageEntry[Index], Level-1, MaxLeafLevel, Address + (Index<<(9*(Level-1) + 3)));
    if (Status != UNIT_TEST_PASSED) {
      return Status;
    }
  }

  return UNIT_TEST_PASSED;
}

/**
  Check if the Page table is valid

  @param[in]   PageTable      The pointer to the page table.
  @param[in]   PagingMode     The paging mode.

  @retval  UNIT_TEST_PASSED   It is a valid Page Table
**/
UNIT_TEST_STATUS
ValidateAndRandomeModifyPageTable (
  IN     UINTN        PageTable,
  IN     PAGING_MODE  PagingMode
  )
{
  UINTN              MaxLevel;
  UINTN              MaxLeafLevel;
  UINT64             Index;
  UNIT_TEST_STATUS   Status;
  IA32_PAGING_ENTRY  *PagingEntry;

  if ((PagingMode == Paging32bit) || (PagingMode >= PagingModeMax)) {
    //
    // 32bit paging is never supported.
    //
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  MaxLeafLevel = (UINT8)PagingMode;
  MaxLevel     = (UINT8)(PagingMode >> 8);

  PagingEntry = (IA32_PAGING_ENTRY *)(UINTN)PageTable;
  for (Index = 0; Index < 512; Index++) {
    Status = ValidateAndRandomeModifyPageTablePageTableEntry (&PagingEntry[Index], MaxLevel, MaxLeafLevel, Index << (9 * MaxLevel + 3));
    if (Status != UNIT_TEST_PASSED) {
      return Status;
    }
  }

  return Status;
}

/**
  Remove the last MAP_ENTRY in MapEntrys.

  @param MapEntrys   Pointer to MapEntrys buffer
**/
VOID
RemoveLastMapEntry (
  IN OUT MAP_ENTRYS  *MapEntrys
  )
{
  UINTN  MapsIndex;

  if (MapEntrys->Count == 0) {
    return;
  }

  MapsIndex = MapEntrys->Count - 1;
  ZeroMem (&(MapEntrys->Maps[MapsIndex]), sizeof (MAP_ENTRY));
  MapEntrys->Count = MapsIndex;
}

/**
  Generate single random map entry.
  The map entry can be the input of function PageTableMap
  the LinearAddress and length is aligned to aligned table.

  @param MaxAddress  Max Address.
  @param MapEntrys   Output MapEntrys contains all parameter as input of function PageTableMap
**/
VOID
GenerateSingleRandomMapEntry (
  IN     UINT64      MaxAddress,
  IN OUT MAP_ENTRYS  *MapEntrys
  )
{
  UINTN   MapsIndex;
  UINT64  FormerLinearAddress;
  UINT64  FormerLinearAddressBottom;
  UINT64  FormerLinearAddressTop;

  MapsIndex = MapEntrys->Count;

  ASSERT (MapsIndex < MapEntrys->MaxCount);
  //
  // use AlignedTable to avoid that a random number can be very hard to be 1G or 2M aligned
  //
  if ((MapsIndex != 0) &&  (RandomBoolean (50))) {
    FormerLinearAddress = MapEntrys->Maps[Random32 (0, (UINT32)MapsIndex-1)].LinearAddress;
    if (FormerLinearAddress < 2 * (UINT64)SIZE_1GB) {
      FormerLinearAddressBottom = 0;
    } else {
      FormerLinearAddressBottom = FormerLinearAddress - 2 * (UINT64)SIZE_1GB;
    }

    if (FormerLinearAddress + 2 * (UINT64)SIZE_1GB > MaxAddress) {
      FormerLinearAddressTop = MaxAddress;
    } else {
      FormerLinearAddressTop = FormerLinearAddress + 2 * (UINT64)SIZE_1GB;
    }

    MapEntrys->Maps[MapsIndex].LinearAddress = Random64 (FormerLinearAddressBottom, FormerLinearAddressTop) & AlignedTable[Random32 (0, ARRAY_SIZE (AlignedTable) -1)];
  } else {
    MapEntrys->Maps[MapsIndex].LinearAddress = Random64 (0, MaxAddress) & AlignedTable[Random32 (0, ARRAY_SIZE (AlignedTable) -1)];
  }

  //
  // To have better performance, limit the size less than 10G
  //
  MapEntrys->Maps[MapsIndex].Length = Random64 (0, MIN (MaxAddress - MapEntrys->Maps[MapsIndex].LinearAddress, 10 * (UINT64)SIZE_1GB)) & AlignedTable[Random32 (0, ARRAY_SIZE (AlignedTable) -1)];

  if ((MapsIndex != 0)  && (RandomBoolean (50))) {
    MapEntrys->Maps[MapsIndex].Attribute.Uint64 = MapEntrys->Maps[Random32 (0, (UINT32)MapsIndex-1)].Attribute.Uint64;
    MapEntrys->Maps[MapsIndex].Mask.Uint64      = MapEntrys->Maps[Random32 (0, (UINT32)MapsIndex-1)].Mask.Uint64;
  } else {
    MapEntrys->Maps[MapsIndex].Attribute.Uint64 = Random64 (0, MAX_UINT64) & mSupportedBit.Uint64;
    if (RandomBoolean (5)) {
      //
      // The probability to get random Mask should be small since all bits of a random number
      // have a high probability of containing 0, which may be a invalid input.
      //
      MapEntrys->Maps[MapsIndex].Mask.Uint64 = Random64 (0, MAX_UINT64) & mSupportedBit.Uint64;
    } else {
      MapEntrys->Maps[MapsIndex].Mask.Uint64 = MAX_UINT64;
    }

    if (MapEntrys->Maps[MapsIndex].Mask.Bits.ProtectionKey != 0) {
      MapEntrys->Maps[MapsIndex].Mask.Bits.ProtectionKey = 0xF;
    }
  }

  if (mRandomOption & ONLY_ONE_ONE_MAPPING) {
    MapEntrys->Maps[MapsIndex].Attribute.Uint64 &= (~IA32_MAP_ATTRIBUTE_PAGE_TABLE_BASE_ADDRESS_MASK);
    MapEntrys->Maps[MapsIndex].Attribute.Uint64 |= MapEntrys->Maps[MapsIndex].LinearAddress;
    MapEntrys->Maps[MapsIndex].Mask.Uint64      |= IA32_MAP_ATTRIBUTE_PAGE_TABLE_BASE_ADDRESS_MASK;
  } else {
    MapEntrys->Maps[MapsIndex].Attribute.Uint64 &= (~IA32_MAP_ATTRIBUTE_PAGE_TABLE_BASE_ADDRESS_MASK);
    MapEntrys->Maps[MapsIndex].Attribute.Uint64 |= (Random64 (0, (((UINT64)1)<<52) - 1) & AlignedTable[Random32 (0, ARRAY_SIZE (AlignedTable) -1)]);
  }

  MapEntrys->Count += 1;
}

/**
  Compare the attribute for one point.
  MapEntrys records every memory ranges that is used as input
  Map and MapCount are gotten from Page table
  Compare if this point have same attribute.

  @param[in] Address      Address of one Point.
  @param[in] MapEntrys    Record every memory ranges that is used as input
  @param[in] Map          Pointer to an array that describes multiple linear address ranges.
  @param[in] MapCount     Pointer to a UINTN that hold the number of entries in the Map.
  @param[in] InitMap      Pointer to an array that describes init map entries.
  @param[in] InitMapCount Pointer to a UINTN that hold the number of init map entries.

  @retval TRUE          At least one byte of data is available to be read
  @retval FALSE         No data is available to be read
**/
BOOLEAN
CompareEntrysforOnePoint (
  IN  UINT64          Address,
  IN  MAP_ENTRYS      *MapEntrys,
  IN  IA32_MAP_ENTRY  *Map,
  IN  UINTN           MapCount,
  IN  IA32_MAP_ENTRY  *InitMap,
  IN  UINTN           InitMapCount
  )
{
  UINTN               Index;
  IA32_MAP_ATTRIBUTE  AttributeInInitMap;
  IA32_MAP_ATTRIBUTE  AttributeInMap;
  IA32_MAP_ATTRIBUTE  AttributeInMapEntrys;
  IA32_MAP_ATTRIBUTE  MaskInMapEntrys;

  AttributeInMap.Uint64       = 0;
  AttributeInMapEntrys.Uint64 = 0;
  AttributeInInitMap.Uint64   = 0;
  MaskInMapEntrys.Uint64      = 0;
  //
  // Assume every entry in maps does not overlap with each other
  //
  for (Index = 0; Index < MapCount; Index++) {
    if ((Address >= Map[Index].LinearAddress) && (Address < (Map[Index].LinearAddress + Map[Index].Length))) {
      AttributeInMap.Uint64  = (Map[Index].Attribute.Uint64 & mSupportedBit.Uint64);
      AttributeInMap.Uint64 &= (~IA32_MAP_ATTRIBUTE_PAGE_TABLE_BASE_ADDRESS_MASK);
      AttributeInMap.Uint64 |= (Address - Map[Index].LinearAddress + IA32_MAP_ATTRIBUTE_PAGE_TABLE_BASE_ADDRESS (&Map[Index].Attribute)) & IA32_MAP_ATTRIBUTE_PAGE_TABLE_BASE_ADDRESS_MASK;
      break;
    }
  }

  //
  // Assume every entry in maps does not overlap with each other
  //
  for (Index = 0; Index < InitMapCount; Index++) {
    if ((Address >= InitMap[Index].LinearAddress) && (Address < (InitMap[Index].LinearAddress + InitMap[Index].Length))) {
      AttributeInInitMap.Uint64  = (InitMap[Index].Attribute.Uint64 & mSupportedBit.Uint64);
      AttributeInInitMap.Uint64 &= (~IA32_MAP_ATTRIBUTE_PAGE_TABLE_BASE_ADDRESS_MASK);
      AttributeInInitMap.Uint64 |= (Address - InitMap[Index].LinearAddress + IA32_MAP_ATTRIBUTE_PAGE_TABLE_BASE_ADDRESS (&InitMap[Index].Attribute)) & IA32_MAP_ATTRIBUTE_PAGE_TABLE_BASE_ADDRESS_MASK;

      break;
    }
  }

  AttributeInMapEntrys.Uint64 = AttributeInInitMap.Uint64;

  for (Index = MapEntrys->InitCount; Index < MapEntrys->Count; Index++) {
    if ((Address >= MapEntrys->Maps[Index].LinearAddress) && (Address < (MapEntrys->Maps[Index].LinearAddress + MapEntrys->Maps[Index].Length))) {
      if (AttributeInMapEntrys.Bits.Present == 0) {
        AttributeInMapEntrys.Uint64 = 0;
        MaskInMapEntrys.Uint64      = 0;
      }

      MaskInMapEntrys.Uint64      |= MapEntrys->Maps[Index].Mask.Uint64;
      AttributeInMapEntrys.Uint64 &= (~MapEntrys->Maps[Index].Mask.Uint64);
      AttributeInMapEntrys.Uint64 |=  (MapEntrys->Maps[Index].Attribute.Uint64 & MapEntrys->Maps[Index].Mask.Uint64);
      if (IA32_MAP_ATTRIBUTE_PAGE_TABLE_BASE_ADDRESS (&MapEntrys->Maps[Index].Mask) != 0) {
        AttributeInMapEntrys.Uint64 &= (~IA32_MAP_ATTRIBUTE_PAGE_TABLE_BASE_ADDRESS_MASK);
        AttributeInMapEntrys.Uint64 |= (Address - MapEntrys->Maps[Index].LinearAddress + IA32_MAP_ATTRIBUTE_PAGE_TABLE_BASE_ADDRESS (&MapEntrys->Maps[Index].Attribute)) & IA32_MAP_ATTRIBUTE_PAGE_TABLE_BASE_ADDRESS_MASK;
      }
    }
  }

  if (AttributeInMap.Bits.Present == 0) {
    if (AttributeInMapEntrys.Bits.Present == 0) {
      return TRUE;
    }
  }

  if ((AttributeInMap.Uint64 & MaskInMapEntrys.Uint64) != (AttributeInMapEntrys.Uint64 & MaskInMapEntrys.Uint64)) {
    DEBUG ((DEBUG_INFO, "======detailed information begin=====\n"));
    DEBUG ((DEBUG_INFO, "\nError: Detect different attribute on a point with linear address: 0x%lx\n", Address));
    DEBUG ((DEBUG_INFO, "By parsing page table, the point has Attribute 0x%lx, and map to physical address 0x%lx\n", IA32_MAP_ATTRIBUTE_ATTRIBUTES (&AttributeInMap) & MaskInMapEntrys.Uint64, IA32_MAP_ATTRIBUTE_PAGE_TABLE_BASE_ADDRESS (&AttributeInMap)));
    DEBUG ((DEBUG_INFO, "While according to inputs, the point should Attribute 0x%lx, and should map to physical address 0x%lx\n", IA32_MAP_ATTRIBUTE_ATTRIBUTES (&AttributeInMapEntrys) & MaskInMapEntrys.Uint64, IA32_MAP_ATTRIBUTE_PAGE_TABLE_BASE_ADDRESS (&AttributeInMapEntrys)));
    DEBUG ((DEBUG_INFO, "The total Mask is 0x%lx\n", MaskInMapEntrys.Uint64));

    if (MapEntrys->InitCount != 0) {
      DEBUG ((DEBUG_INFO, "Below is the initialization status:\n"));
      for (Index = 0; Index < InitMapCount; Index++) {
        if ((Address >= InitMap[Index].LinearAddress) && (Address < (InitMap[Index].LinearAddress + InitMap[Index].Length))) {
          DEBUG ((DEBUG_INFO, " *"));
        } else {
          DEBUG ((DEBUG_INFO, "  "));
        }

        DEBUG ((DEBUG_INFO, "  %02d: {0x%lx, 0x%lx, 0x%lx}\n", Index, InitMap[Index].LinearAddress, InitMap[Index].LinearAddress + InitMap[Index].Length, InitMap[Index].Attribute.Uint64));
      }
    }

    DEBUG ((DEBUG_INFO, "Below is the inputs:\n"));
    DEBUG ((DEBUG_INFO, "  Index: {LinearAddress, LinearLimit, Mask, Attribute}\n"));
    for (Index = MapEntrys->InitCount; Index < MapEntrys->Count; Index++) {
      if ((Address >= MapEntrys->Maps[Index].LinearAddress) && (Address < (MapEntrys->Maps[Index].LinearAddress + MapEntrys->Maps[Index].Length))) {
        DEBUG ((DEBUG_INFO, " *"));
      } else {
        DEBUG ((DEBUG_INFO, "  "));
      }

      DEBUG ((
        DEBUG_INFO,
        "  %02d: {0x%lx, 0x%lx, 0x%lx,0x%lx}\n",
        Index,
        MapEntrys->Maps[Index].LinearAddress,
        MapEntrys->Maps[Index].LinearAddress + MapEntrys->Maps[Index].Length,
        MapEntrys->Maps[Index].Mask.Uint64,
        MapEntrys->Maps[Index].Attribute.Uint64
        ));
    }

    DEBUG ((DEBUG_INFO, "Below is the dumped from pagetable:\n"));
    for (Index = 0; Index < MapCount; Index++) {
      if ((Address >= Map[Index].LinearAddress) && (Address < (Map[Index].LinearAddress + Map[Index].Length))) {
        DEBUG ((DEBUG_INFO, " *"));
      } else {
        DEBUG ((DEBUG_INFO, "  "));
      }

      DEBUG ((DEBUG_INFO, "%02d: {0x%lx, 0x%lx, 0x%lx}\n", Index, Map[Index].LinearAddress, Map[Index].LinearAddress + Map[Index].Length, Map[Index].Attribute.Uint64));
    }

    DEBUG ((DEBUG_INFO, "======detailed information done=====\n"));
    return FALSE;
  }

  return TRUE;
}

/**
  Append key point of a given address to Buffer
  if buffer is NULL, only count needed count

  @param[in, out] Buffer  Buffer to contains all key point.
  @param[in, out] Count   Count of the key point.
  @param[in]      Address given address
**/
VOID
AppendKeyPointToBuffer (
  IN OUT UINT64  *Buffer,
  IN OUT UINTN   *Count,
  IN     UINT64  Address
  )
{
  if ( Buffer != NULL) {
    Buffer[*Count] = Address;
    (*Count)++;
    Buffer[*Count] = Address+1;
    (*Count)++;
    Buffer[*Count] = Address-1;
    (*Count)++;
  } else {
    (*Count) = (*Count) +3;
  }
}

/**
  Get all key points from a buffer
  if buffer is NULL, only count needed count

  @param[in] MapEntrys    Record every memory ranges that is used as input
  @param[in] Map          Pointer to an array that describes multiple linear address ranges.
  @param[in] MapCount     Pointer to a UINTN that hold the actual number of entries in the Map.
  @param[in, out] Buffer  Buffer to contains all key point.
  @param[in, out] Count   Count of the key point.
**/
VOID
GetKeyPointList (
  IN     MAP_ENTRYS      *MapEntrys,
  IN     IA32_MAP_ENTRY  *Map,
  IN     UINTN           MapCount,
  IN OUT UINT64          *Buffer,
  IN OUT UINTN           *Count
  )
{
  UINTN  TemCount;
  UINTN  Index1;
  UINTN  Index2;

  TemCount = 0;

  for (Index1 = 0; Index1 < MapEntrys->Count; Index1++) {
    AppendKeyPointToBuffer (Buffer, &TemCount, MapEntrys->Maps[Index1].LinearAddress);
    AppendKeyPointToBuffer (Buffer, &TemCount, MapEntrys->Maps[Index1].LinearAddress + MapEntrys->Maps[Index1].Length);
  }

  for (Index2 = 0; Index2 < MapCount; Index2++) {
    if (Buffer != NULL) {
      for (Index1 = 0; Index1 < TemCount; Index1++) {
        if (Buffer[Index1] == Map[Index2].LinearAddress) {
          break;
        }
      }

      if (Index1 < TemCount) {
        continue;
      }
    }

    AppendKeyPointToBuffer (Buffer, &TemCount, Map[Index2].LinearAddress);
  }

  for (Index2 = 0; Index2 < MapCount; Index2++) {
    if (Buffer != NULL) {
      for (Index1 = 0; Index1 < TemCount; Index1++) {
        if (Buffer[Index1] == (Map[Index2].LinearAddress + Map[Index2].Length)) {
          break;
        }
      }

      if (Index1 < TemCount) {
        continue;
      }
    }

    AppendKeyPointToBuffer (Buffer, &TemCount, Map[Index2].LinearAddress + Map[Index2].Length);
  }

  *Count = TemCount;
}

/**
  Generate random one range with randome attribute, and add it into pagetable
  Compare the key point has same attribute

  @param[in, out] PageTable     The pointer to the page table to update, or pointer to NULL if a new page table is to be created.
  @param[in]      PagingMode    The paging mode.
  @param[in]      MaxAddress    Max Address.
  @param[in]      MapEntrys     Record every memory ranges that is used as input
  @param[in]      PagesRecord   Used to record memory usage for page table.
  @param[in]      InitMap      Pointer to an array that describes init map entries.
  @param[in]      InitMapCount Pointer to a UINTN that hold the number of init map entries.

  @retval  UNIT_TEST_PASSED        The test is successful.
**/
UNIT_TEST_STATUS
SingleMapEntryTest (
  IN OUT UINTN                  *PageTable,
  IN     PAGING_MODE            PagingMode,
  IN     UINT64                 MaxAddress,
  IN     MAP_ENTRYS             *MapEntrys,
  IN     ALLOCATE_PAGE_RECORDS  *PagesRecord,
  IN     IA32_MAP_ENTRY         *InitMap,
  IN     UINTN                  InitMapCount
  )
{
  UINTN               MapsIndex;
  RETURN_STATUS       Status;
  UINTN               PageTableBufferSize;
  VOID                *Buffer;
  IA32_MAP_ENTRY      *Map;
  UINTN               MapCount;
  IA32_MAP_ENTRY      *Map2;
  UINTN               MapCount2;
  UINTN               Index;
  UINTN               KeyPointCount;
  UINTN               NewKeyPointCount;
  UINT64              *KeyPointBuffer;
  UINTN               Level;
  UINT64              Value;
  UNIT_TEST_STATUS    TestStatus;
  MAP_ENTRY           *LastMapEntry;
  IA32_MAP_ATTRIBUTE  *Mask;
  IA32_MAP_ATTRIBUTE  *Attribute;
  UINT64              LastNotPresentRegionStart;
  BOOLEAN             IsNotPresent;
  BOOLEAN             IsModified;

  MapsIndex                 = MapEntrys->Count;
  MapCount                  = 0;
  LastNotPresentRegionStart = 0;
  IsNotPresent              = FALSE;
  IsModified                = FALSE;

  SetRandomStack ();
  GenerateSingleRandomMapEntry (MaxAddress, MapEntrys);
  LastMapEntry = &MapEntrys->Maps[MapsIndex];
  Status       = PageTableParse (*PageTable, PagingMode, NULL, &MapCount);

  if (MapCount != 0) {
    UT_ASSERT_EQUAL (Status, RETURN_BUFFER_TOO_SMALL);
    Map = AllocatePages (EFI_SIZE_TO_PAGES (MapCount * sizeof (IA32_MAP_ENTRY)));
    ASSERT (Map != NULL);
    Status = PageTableParse (*PageTable, PagingMode, Map, &MapCount);
  }

  //
  // Check if the generated MapEntrys->Maps[MapsIndex] contains not-present range.
  //
  if (LastMapEntry->Length > 0) {
    for (Index = 0; Index < MapCount; Index++) {
      if ((LastNotPresentRegionStart < Map[Index].LinearAddress) &&
          (LastMapEntry->LinearAddress < Map[Index].LinearAddress) && (LastMapEntry->LinearAddress + LastMapEntry->Length > LastNotPresentRegionStart))
      {
        //
        // MapEntrys->Maps[MapsIndex] contains not-present range in exsiting page table.
        //
        break;
      }

      LastNotPresentRegionStart = Map[Index].LinearAddress + Map[Index].Length;
    }

    //
    // Either LastMapEntry overlaps with the not-present region in the very end
    // Or it overlaps with one in the middle
    if (LastNotPresentRegionStart < LastMapEntry->LinearAddress + LastMapEntry->Length) {
      IsNotPresent = TRUE;
    }
  }

  PageTableBufferSize = 0;
  Status              = PageTableMap (
                          PageTable,
                          PagingMode,
                          NULL,
                          &PageTableBufferSize,
                          LastMapEntry->LinearAddress,
                          LastMapEntry->Length,
                          &LastMapEntry->Attribute,
                          &LastMapEntry->Mask,
                          &IsModified
                          );

  Attribute = &LastMapEntry->Attribute;
  Mask      = &LastMapEntry->Mask;
  //
  // If set [LinearAddress, LinearAddress+Attribute] to not preset, all
  // other attributes should not be provided.
  //
  if ((LastMapEntry->Length > 0) && (Attribute->Bits.Present == 0) && (Mask->Bits.Present == 1) && (Mask->Uint64 > 1)) {
    RemoveLastMapEntry (MapEntrys);
    UT_ASSERT_EQUAL (Status, RETURN_INVALID_PARAMETER);
    return UNIT_TEST_PASSED;
  }

  //
  // Return Status for non-present range also should be InvalidParameter when:
  // 1. Some of attributes are not provided when mapping non-present range to present.
  // 2. Set any other attribute without setting the non-present range to Present.
  //
  if (IsNotPresent) {
    if ((Mask->Bits.Present == 1) && (Attribute->Bits.Present == 1)) {
      //
      // Creating new page table or remapping non-present range to present.
      //
      if ((Mask->Bits.ReadWrite == 0) || (Mask->Bits.UserSupervisor == 0) || (Mask->Bits.WriteThrough == 0) || (Mask->Bits.CacheDisabled == 0) ||
          (Mask->Bits.Accessed == 0) || (Mask->Bits.Dirty == 0) || (Mask->Bits.Pat == 0) || (Mask->Bits.Global == 0) ||
          ((Mask->Bits.PageTableBaseAddressLow == 0) && (Mask->Bits.PageTableBaseAddressHigh == 0)) || (Mask->Bits.ProtectionKey == 0) || (Mask->Bits.Nx == 0))
      {
        RemoveLastMapEntry (MapEntrys);
        UT_ASSERT_EQUAL (Status, RETURN_INVALID_PARAMETER);
        return UNIT_TEST_PASSED;
      }
    } else if ((Mask->Bits.Present == 0) && (Mask->Uint64 > 1)) {
      //
      // Only change other attributes for non-present range is not permitted.
      //
      RemoveLastMapEntry (MapEntrys);
      UT_ASSERT_EQUAL (Status, RETURN_INVALID_PARAMETER);
      return UNIT_TEST_PASSED;
    }
  }

  if (PageTableBufferSize != 0) {
    UT_ASSERT_EQUAL (Status, RETURN_BUFFER_TOO_SMALL);

    //
    // Allocate memory for Page table
    // Note the memory is used in one complete Random test.
    //
    Buffer = PagesRecord->AllocatePagesForPageTable (PagesRecord, EFI_SIZE_TO_PAGES (PageTableBufferSize));
    UT_ASSERT_NOT_EQUAL (Buffer, NULL);
    Status = PageTableMap (
               PageTable,
               PagingMode,
               Buffer,
               &PageTableBufferSize,
               LastMapEntry->LinearAddress,
               LastMapEntry->Length,
               &LastMapEntry->Attribute,
               &LastMapEntry->Mask,
               &IsModified
               );
  }

  if (Status != RETURN_SUCCESS ) {
    UT_ASSERT_EQUAL (Status, RETURN_SUCCESS);
  }

  UT_ASSERT_EQUAL (Status, RETURN_SUCCESS);
  TestStatus = IsPageTableValid (*PageTable, PagingMode);
  if (TestStatus != UNIT_TEST_PASSED) {
    return TestStatus;
  }

  MapCount2 = 0;
  Status    = PageTableParse (*PageTable, PagingMode, NULL, &MapCount2);
  if (MapCount2 != 0) {
    UT_ASSERT_EQUAL (Status, RETURN_BUFFER_TOO_SMALL);

    //
    // Allocate memory for Map2
    // Note the memory is only used in this one Single MapEntry Test
    //
    Map2 = AllocatePages (EFI_SIZE_TO_PAGES (MapCount2 * sizeof (IA32_MAP_ENTRY)));
    ASSERT (Map2 != NULL);
    Status = PageTableParse (*PageTable, PagingMode, Map2, &MapCount2);
  }

  //
  // Check if PageTable has been modified.
  //
  if (MapCount2 != MapCount) {
    UT_ASSERT_EQUAL (IsModified, TRUE);
  } else {
    if (CompareMem (Map, Map2, MapCount2 * sizeof (IA32_MAP_ENTRY)) != 0) {
      UT_ASSERT_EQUAL (IsModified, TRUE);
    } else {
      UT_ASSERT_EQUAL (IsModified, FALSE);
    }
  }

  UT_ASSERT_EQUAL (Status, RETURN_SUCCESS);

  //
  // Allocate memory to record all key point
  // Note the memory is only used in this one Single MapEntry Test
  //
  KeyPointCount = 0;
  GetKeyPointList (MapEntrys, Map2, MapCount2, NULL, &KeyPointCount);
  KeyPointBuffer = AllocatePages (EFI_SIZE_TO_PAGES (KeyPointCount * sizeof (UINT64)));
  ASSERT (KeyPointBuffer != NULL);
  NewKeyPointCount = 0;
  GetKeyPointList (MapEntrys, Map2, MapCount2, KeyPointBuffer, &NewKeyPointCount);

  //
  // Compare all key point's attribute
  //
  for (Index = 0; Index < NewKeyPointCount; Index++) {
    if (!CompareEntrysforOnePoint (KeyPointBuffer[Index], MapEntrys, Map2, MapCount2, InitMap, InitMapCount)) {
      DEBUG ((DEBUG_INFO, "Error happens at below key point\n"));
      DEBUG ((DEBUG_INFO, "Index = %d KeyPointBuffer[Index] = 0x%lx\n", Index, KeyPointBuffer[Index]));
      Value = GetEntryFromPageTable (*PageTable, PagingMode, KeyPointBuffer[Index], &Level);
      DEBUG ((DEBUG_INFO, "From Page table, this key point is in level %d entry, with entry value is 0x%lx\n", Level, Value));
      UT_ASSERT_TRUE (FALSE);
    }
  }

  FreePages (KeyPointBuffer, EFI_SIZE_TO_PAGES (KeyPointCount * sizeof (UINT64)));
  if (MapCount != 0) {
    FreePages (Map, EFI_SIZE_TO_PAGES (MapCount * sizeof (IA32_MAP_ENTRY)));
  }

  if (MapCount2 != 0) {
    FreePages (Map2, EFI_SIZE_TO_PAGES (MapCount2 * sizeof (IA32_MAP_ENTRY)));
  }

  return UNIT_TEST_PASSED;
}

/**
  Allocate page and record the information in PagesRecord

  @param[in]  PagesRecord   Point to a struct to record memory usage
  @param[in]  Pages         Page count needed to allocate

  @return A pointer to the allocated buffer or NULL if allocation fails.
**/
VOID *
EFIAPI
RecordAllocatePages (
  IN ALLOCATE_PAGE_RECORDS  *PagesRecord,
  IN UINTN                  Pages
  )
{
  VOID  *Buffer;

  Buffer = NULL;
  if (PagesRecord->Count < PagesRecord->MaxCount) {
    Buffer                                          = AllocatePages (Pages);
    PagesRecord->Records[PagesRecord->Count].Buffer = Buffer;
    PagesRecord->Records[PagesRecord->Count].Pages  = Pages;
    PagesRecord->Count++;
  }

  ASSERT (Buffer != NULL);

  return Buffer;
}

/**
  The function is a whole Random test, it will call SingleMapEntryTest for ExpctedEntryNumber times

  @param[in]  ExpctedEntryNumber   The count of random entry
  @param[in]  PagingMode           The paging mode.

  @retval  UNIT_TEST_PASSED        The test is successful.
**/
UNIT_TEST_STATUS
MultipleMapEntryTest (
  IN UINTN        ExpctedEntryNumber,
  IN PAGING_MODE  PagingMode
  )
{
  UINTN                  PageTable;
  UINT64                 MaxAddress;
  MAP_ENTRYS             *MapEntrys;
  ALLOCATE_PAGE_RECORDS  *PagesRecord;
  UINTN                  Index;
  UNIT_TEST_STATUS       TestStatus;
  RETURN_STATUS          Status;
  IA32_MAP_ENTRY         *InitMap;
  UINTN                  InitMapCount;

  MaxAddress = GetMaxAddress (PagingMode);
  PageTable  = 0;
  MapEntrys  = AllocatePages (EFI_SIZE_TO_PAGES (1000*sizeof (MAP_ENTRY) + sizeof (MAP_ENTRYS)));
  ASSERT (MapEntrys != NULL);
  MapEntrys->Count     = 0;
  MapEntrys->InitCount = 0;
  MapEntrys->MaxCount  = 1000;
  PagesRecord          = AllocatePages (EFI_SIZE_TO_PAGES (1000*sizeof (ALLOCATE_PAGE_RECORD) + sizeof (ALLOCATE_PAGE_RECORDS)));
  ASSERT (PagesRecord != NULL);
  PagesRecord->Count                     = 0;
  PagesRecord->MaxCount                  = 1000;
  PagesRecord->AllocatePagesForPageTable = RecordAllocatePages;

  if (mRandomOption & MANUAL_CHANGE_PAGE_TABLE) {
    ExpctedEntryNumber = ExpctedEntryNumber/2;
  }

  for (Index = 0; Index < ExpctedEntryNumber; Index++) {
    TestStatus = SingleMapEntryTest (
                   &PageTable,
                   PagingMode,
                   MaxAddress,
                   MapEntrys,
                   PagesRecord,
                   NULL,
                   0
                   );
    if (TestStatus != UNIT_TEST_PASSED) {
      return TestStatus;
    }
  }

  if ((mRandomOption & MANUAL_CHANGE_PAGE_TABLE) != 0) {
    MapEntrys->InitCount = ExpctedEntryNumber;
    TestStatus           = ValidateAndRandomeModifyPageTable (PageTable, PagingMode);
    RandomNumber         = 0;
    if (TestStatus != UNIT_TEST_PASSED) {
      return TestStatus;
    }

    InitMapCount = 0;
    Status       = PageTableParse (PageTable, PagingMode, NULL, &InitMapCount);
    if (InitMapCount != 0) {
      UT_ASSERT_EQUAL (Status, RETURN_BUFFER_TOO_SMALL);

      //
      // Allocate memory for Maps
      // Note the memory is only used in this one Single MapEntry Test
      //
      InitMap = AllocatePages (EFI_SIZE_TO_PAGES (InitMapCount * sizeof (IA32_MAP_ENTRY)));
      ASSERT (InitMap != NULL);
      Status = PageTableParse (PageTable, PagingMode, InitMap, &InitMapCount);
    }

    UT_ASSERT_EQUAL (Status, RETURN_SUCCESS);
    for (Index = 0; Index < ExpctedEntryNumber; Index++) {
      TestStatus = SingleMapEntryTest (
                     &PageTable,
                     PagingMode,
                     MaxAddress,
                     MapEntrys,
                     PagesRecord,
                     InitMap,
                     InitMapCount
                     );
      if (TestStatus != UNIT_TEST_PASSED) {
        return TestStatus;
      }
    }

    if (InitMapCount != 0) {
      FreePages (InitMap, EFI_SIZE_TO_PAGES (InitMapCount*sizeof (IA32_MAP_ENTRY)));
    }
  }

  FreePages (
    MapEntrys,
    EFI_SIZE_TO_PAGES (1000*sizeof (MAP_ENTRY) + sizeof (MAP_ENTRYS))
    );

  for (Index = 0; Index < PagesRecord->Count; Index++) {
    FreePages (PagesRecord->Records[Index].Buffer, PagesRecord->Records[Index].Pages);
  }

  FreePages (PagesRecord, EFI_SIZE_TO_PAGES (1000*sizeof (ALLOCATE_PAGE_RECORD) + sizeof (ALLOCATE_PAGE_RECORDS)));

  return UNIT_TEST_PASSED;
}

/**
  Random Test

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestCaseforRandomTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UNIT_TEST_STATUS  Status;
  UINTN             Index;

  UT_ASSERT_EQUAL (RandomSeed (NULL, 0), TRUE);
  UT_ASSERT_EQUAL (Random32 (100, 100), 100);
  UT_ASSERT_EQUAL (Random64 (100, 100), 100);
  UT_ASSERT_TRUE ((Random32 (9, 10) >= 9) & (Random32 (9, 10) <= 10));
  UT_ASSERT_TRUE ((Random64 (9, 10) >= 9) & (Random64 (9, 10) <= 10));
  mSupportedBit.Uint64              = 0;
  mSupportedBit.Bits.Present        = 1;
  mSupportedBit.Bits.ReadWrite      = 1;
  mSupportedBit.Bits.UserSupervisor = 1;
  mSupportedBit.Bits.WriteThrough   = 1;
  mSupportedBit.Bits.CacheDisabled  = 1;
  mSupportedBit.Bits.Accessed       = 1;
  mSupportedBit.Bits.Dirty          = 1;
  mSupportedBit.Bits.Pat            = 1;
  mSupportedBit.Bits.Global         = 1;
  mSupportedBit.Bits.ProtectionKey  = 0xF;
  if (((CPU_PAGE_TABLE_LIB_RANDOM_TEST_CONTEXT *)Context)->PagingMode == PagingPae) {
    mSupportedBit.Bits.ProtectionKey = 0;
  }

  mSupportedBit.Bits.Nx = 1;

  mRandomOption = ((CPU_PAGE_TABLE_LIB_RANDOM_TEST_CONTEXT *)Context)->RandomOption;
  mNumberIndex  = 0;

  for (Index = 0; Index < ((CPU_PAGE_TABLE_LIB_RANDOM_TEST_CONTEXT *)Context)->TestCount; Index++) {
    Status = MultipleMapEntryTest (
               ((CPU_PAGE_TABLE_LIB_RANDOM_TEST_CONTEXT *)Context)->TestRangeCount,
               ((CPU_PAGE_TABLE_LIB_RANDOM_TEST_CONTEXT *)Context)->PagingMode
               );
    if (Status != UNIT_TEST_PASSED) {
      return Status;
    }

    DEBUG ((DEBUG_INFO, "."));
  }

  DEBUG ((DEBUG_INFO, "\n"));

  return UNIT_TEST_PASSED;
}
