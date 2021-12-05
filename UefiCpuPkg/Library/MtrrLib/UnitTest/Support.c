/** @file
  Unit tests of the MtrrLib instance of the MtrrLib class

  Copyright (c) 2018 - 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "MtrrLibUnitTest.h"

MTRR_MEMORY_CACHE_TYPE  mMemoryCacheTypes[] = {
  CacheUncacheable, CacheWriteCombining, CacheWriteThrough, CacheWriteProtected, CacheWriteBack
};

UINT64                           mFixedMtrrsValue[MTRR_NUMBER_OF_FIXED_MTRR];
MSR_IA32_MTRR_PHYSBASE_REGISTER  mVariableMtrrsPhysBase[MTRR_NUMBER_OF_VARIABLE_MTRR];
MSR_IA32_MTRR_PHYSMASK_REGISTER  mVariableMtrrsPhysMask[MTRR_NUMBER_OF_VARIABLE_MTRR];
MSR_IA32_MTRR_DEF_TYPE_REGISTER  mDefTypeMsr;
MSR_IA32_MTRRCAP_REGISTER        mMtrrCapMsr;
CPUID_VERSION_INFO_EDX           mCpuidVersionInfoEdx;
CPUID_VIR_PHY_ADDRESS_SIZE_EAX   mCpuidVirPhyAddressSizeEax;

BOOLEAN       mRandomInput;
UINTN         mNumberIndex = 0;
extern UINTN  mNumbers[];
extern UINTN  mNumberCount;

/**
  Return a random number between 0 and RAND_MAX.

  If mRandomInput is TRUE, the routine directly calls rand().
  Otherwise, the routine returns the pre-generated numbers.

  @return a number between 0 and RAND_MAX.
**/
UINTN
Rand (
  VOID
  )
{
  if (mRandomInput) {
    return rand ();
  } else {
    DEBUG ((DEBUG_INFO, "random: %d\n", mNumberIndex));
    return mNumbers[mNumberIndex++ % (mNumberCount - 1)];
  }
}

CHAR8  mContentTemplate[] = {
  "/** @file\n"
  "  Pre-generated random number used by MtrrLib test.\n"
  "\n"
  "  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>\n"
  "  SPDX-License-Identifier: BSD-2-Clause-Patent\n"
  "**/\n"
  "UINTN mNumberCount = %d;\n"
  "UINTN mNumbers[] = {"
};

/**
  Generate Count random numbers in FilePath.

  @param FilePath  The file path to put the generated random numbers.
  @param Count     Count of random numbers.
**/
VOID
GenerateRandomNumbers (
  CHAR8  *FilePath,
  UINTN  Count
  )
{
  FILE   *File;
  UINTN  Index;

  File = fopen (FilePath, "w");
  fprintf (File, mContentTemplate, Count);
  for (Index = 0; Index < Count; Index++) {
    if (Index % 10 == 0) {
      fprintf (File, "\n ");
    }

    fprintf (File, " %d,", rand ());
  }

  fprintf (File, "\n};\n");
  fclose (File);
}

/**
  Retrieves CPUID information.

  Executes the CPUID instruction with EAX set to the value specified by Index.
  This function always returns Index.
  If Eax is not NULL, then the value of EAX after CPUID is returned in Eax.
  If Ebx is not NULL, then the value of EBX after CPUID is returned in Ebx.
  If Ecx is not NULL, then the value of ECX after CPUID is returned in Ecx.
  If Edx is not NULL, then the value of EDX after CPUID is returned in Edx.
  This function is only available on IA-32 and x64.

  @param  Index The 32-bit value to load into EAX prior to invoking the CPUID
                instruction.
  @param  Eax   The pointer to the 32-bit EAX value returned by the CPUID
                instruction. This is an optional parameter that may be NULL.
  @param  Ebx   The pointer to the 32-bit EBX value returned by the CPUID
                instruction. This is an optional parameter that may be NULL.
  @param  Ecx   The pointer to the 32-bit ECX value returned by the CPUID
                instruction. This is an optional parameter that may be NULL.
  @param  Edx   The pointer to the 32-bit EDX value returned by the CPUID
                instruction. This is an optional parameter that may be NULL.

  @return Index.

**/
UINT32
EFIAPI
UnitTestMtrrLibAsmCpuid (
  IN      UINT32  Index,
  OUT     UINT32  *Eax   OPTIONAL,
  OUT     UINT32  *Ebx   OPTIONAL,
  OUT     UINT32  *Ecx   OPTIONAL,
  OUT     UINT32  *Edx   OPTIONAL
  )
{
  switch (Index) {
    case CPUID_VERSION_INFO:
      if (Edx != NULL) {
        *Edx = mCpuidVersionInfoEdx.Uint32;
      }

      return Index;
      break;
    case CPUID_EXTENDED_FUNCTION:
      if (Eax != NULL) {
        *Eax = CPUID_VIR_PHY_ADDRESS_SIZE;
      }

      return Index;
      break;
    case CPUID_VIR_PHY_ADDRESS_SIZE:
      if (Eax != NULL) {
        *Eax = mCpuidVirPhyAddressSizeEax.Uint32;
      }

      return Index;
      break;
  }

  //
  // Should never fall through to here
  //
  ASSERT (FALSE);
  return Index;
}

/**
  Returns a 64-bit Machine Specific Register(MSR).

  Reads and returns the 64-bit MSR specified by Index. No parameter checking is
  performed on Index, and some Index values may cause CPU exceptions. The
  caller must either guarantee that Index is valid, or the caller must set up
  exception handlers to catch the exceptions. This function is only available
  on IA-32 and x64.

  @param  MsrIndex The 32-bit MSR index to read.

  @return The value of the MSR identified by MsrIndex.

**/
UINT64
EFIAPI
UnitTestMtrrLibAsmReadMsr64 (
  IN UINT32  MsrIndex
  )
{
  UINT32  Index;

  for (Index = 0; Index < ARRAY_SIZE (mFixedMtrrsValue); Index++) {
    if (MsrIndex == mFixedMtrrsIndex[Index]) {
      return mFixedMtrrsValue[Index];
    }
  }

  if ((MsrIndex >= MSR_IA32_MTRR_PHYSBASE0) &&
      (MsrIndex <= MSR_IA32_MTRR_PHYSMASK0 + (MTRR_NUMBER_OF_VARIABLE_MTRR << 1)))
  {
    if (MsrIndex % 2 == 0) {
      Index = (MsrIndex - MSR_IA32_MTRR_PHYSBASE0) >> 1;
      return mVariableMtrrsPhysBase[Index].Uint64;
    } else {
      Index = (MsrIndex - MSR_IA32_MTRR_PHYSMASK0) >> 1;
      return mVariableMtrrsPhysMask[Index].Uint64;
    }
  }

  if (MsrIndex == MSR_IA32_MTRR_DEF_TYPE) {
    return mDefTypeMsr.Uint64;
  }

  if (MsrIndex == MSR_IA32_MTRRCAP) {
    return mMtrrCapMsr.Uint64;
  }

  //
  // Should never fall through to here
  //
  ASSERT (FALSE);
  return 0;
}

/**
  Writes a 64-bit value to a Machine Specific Register(MSR), and returns the
  value.

  Writes the 64-bit value specified by Value to the MSR specified by Index. The
  64-bit value written to the MSR is returned. No parameter checking is
  performed on Index or Value, and some of these may cause CPU exceptions. The
  caller must either guarantee that Index and Value are valid, or the caller
  must establish proper exception handlers. This function is only available on
  IA-32 and x64.

  @param  MsrIndex The 32-bit MSR index to write.
  @param  Value The 64-bit value to write to the MSR.

  @return Value

**/
UINT64
EFIAPI
UnitTestMtrrLibAsmWriteMsr64 (
  IN      UINT32  MsrIndex,
  IN      UINT64  Value
  )
{
  UINT32  Index;

  for (Index = 0; Index < ARRAY_SIZE (mFixedMtrrsValue); Index++) {
    if (MsrIndex == mFixedMtrrsIndex[Index]) {
      mFixedMtrrsValue[Index] = Value;
      return Value;
    }
  }

  if ((MsrIndex >= MSR_IA32_MTRR_PHYSBASE0) &&
      (MsrIndex <= MSR_IA32_MTRR_PHYSMASK0 + (MTRR_NUMBER_OF_VARIABLE_MTRR << 1)))
  {
    if (MsrIndex % 2 == 0) {
      Index                                = (MsrIndex - MSR_IA32_MTRR_PHYSBASE0) >> 1;
      mVariableMtrrsPhysBase[Index].Uint64 = Value;
      return Value;
    } else {
      Index                                = (MsrIndex - MSR_IA32_MTRR_PHYSMASK0) >> 1;
      mVariableMtrrsPhysMask[Index].Uint64 = Value;
      return Value;
    }
  }

  if (MsrIndex == MSR_IA32_MTRR_DEF_TYPE) {
    mDefTypeMsr.Uint64 = Value;
    return Value;
  }

  if (MsrIndex == MSR_IA32_MTRRCAP) {
    mMtrrCapMsr.Uint64 = Value;
    return Value;
  }

  //
  // Should never fall through to here
  //
  ASSERT (FALSE);
  return 0;
}

/**
  Initialize the MTRR registers.

  @param SystemParameter System parameter that controls the MTRR registers initialization.
**/
UNIT_TEST_STATUS
EFIAPI
InitializeMtrrRegs (
  IN MTRR_LIB_SYSTEM_PARAMETER  *SystemParameter
  )
{
  UINT32  Index;

  SetMem (mFixedMtrrsValue, sizeof (mFixedMtrrsValue), SystemParameter->DefaultCacheType);

  for (Index = 0; Index < ARRAY_SIZE (mVariableMtrrsPhysBase); Index++) {
    mVariableMtrrsPhysBase[Index].Uint64         = 0;
    mVariableMtrrsPhysBase[Index].Bits.Type      = SystemParameter->DefaultCacheType;
    mVariableMtrrsPhysBase[Index].Bits.Reserved1 = 0;

    mVariableMtrrsPhysMask[Index].Uint64         = 0;
    mVariableMtrrsPhysMask[Index].Bits.V         = 0;
    mVariableMtrrsPhysMask[Index].Bits.Reserved1 = 0;
  }

  mDefTypeMsr.Bits.E         = 1;
  mDefTypeMsr.Bits.FE        = 1;
  mDefTypeMsr.Bits.Type      = SystemParameter->DefaultCacheType;
  mDefTypeMsr.Bits.Reserved1 = 0;
  mDefTypeMsr.Bits.Reserved2 = 0;
  mDefTypeMsr.Bits.Reserved3 = 0;

  mMtrrCapMsr.Bits.SMRR      = 0;
  mMtrrCapMsr.Bits.WC        = 0;
  mMtrrCapMsr.Bits.VCNT      = SystemParameter->VariableMtrrCount;
  mMtrrCapMsr.Bits.FIX       = SystemParameter->FixedMtrrSupported;
  mMtrrCapMsr.Bits.Reserved1 = 0;
  mMtrrCapMsr.Bits.Reserved2 = 0;
  mMtrrCapMsr.Bits.Reserved3 = 0;

  mCpuidVersionInfoEdx.Bits.MTRR                      = SystemParameter->MtrrSupported;
  mCpuidVirPhyAddressSizeEax.Bits.PhysicalAddressBits = SystemParameter->PhysicalAddressBits;

  //
  // Hook BaseLib functions used by MtrrLib that require some emulation.
  //
  gUnitTestHostBaseLib.X86->AsmCpuid      = UnitTestMtrrLibAsmCpuid;
  gUnitTestHostBaseLib.X86->AsmReadMsr64  = UnitTestMtrrLibAsmReadMsr64;
  gUnitTestHostBaseLib.X86->AsmWriteMsr64 = UnitTestMtrrLibAsmWriteMsr64;

  return UNIT_TEST_PASSED;
}

/**
  Initialize the MTRR registers.

  @param Context System parameter that controls the MTRR registers initialization.
**/
UNIT_TEST_STATUS
EFIAPI
InitializeSystem (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return InitializeMtrrRegs ((MTRR_LIB_SYSTEM_PARAMETER *)Context);
}

/**
  Collect the test result.

  @param DefaultType          Default memory type.
  @param PhysicalAddressBits  Physical address bits.
  @param VariableMtrrCount    Count of variable MTRRs.
  @param Mtrrs                MTRR settings to collect from.
  @param Ranges               Return the memory ranges.
  @param RangeCount           Return the count of memory ranges.
  @param MtrrCount            Return the count of variable MTRRs being used.
**/
VOID
CollectTestResult (
  IN     MTRR_MEMORY_CACHE_TYPE  DefaultType,
  IN     UINT32                  PhysicalAddressBits,
  IN     UINT32                  VariableMtrrCount,
  IN     MTRR_SETTINGS           *Mtrrs,
  OUT    MTRR_MEMORY_RANGE       *Ranges,
  IN OUT UINTN                   *RangeCount,
  OUT    UINT32                  *MtrrCount
  )
{
  UINTN              Index;
  UINT64             MtrrValidBitsMask;
  UINT64             MtrrValidAddressMask;
  MTRR_MEMORY_RANGE  RawMemoryRanges[ARRAY_SIZE (Mtrrs->Variables.Mtrr)];

  ASSERT (Mtrrs != NULL);
  ASSERT (VariableMtrrCount <= ARRAY_SIZE (Mtrrs->Variables.Mtrr));

  MtrrValidBitsMask    = (1ull << PhysicalAddressBits) - 1;
  MtrrValidAddressMask = MtrrValidBitsMask & ~0xFFFull;

  *MtrrCount = 0;
  for (Index = 0; Index < VariableMtrrCount; Index++) {
    if (((MSR_IA32_MTRR_PHYSMASK_REGISTER *)&Mtrrs->Variables.Mtrr[Index].Mask)->Bits.V == 1) {
      RawMemoryRanges[*MtrrCount].BaseAddress = Mtrrs->Variables.Mtrr[Index].Base & MtrrValidAddressMask;
      RawMemoryRanges[*MtrrCount].Type        =
        ((MSR_IA32_MTRR_PHYSBASE_REGISTER *)&Mtrrs->Variables.Mtrr[Index].Base)->Bits.Type;
      RawMemoryRanges[*MtrrCount].Length =
        ((~(Mtrrs->Variables.Mtrr[Index].Mask & MtrrValidAddressMask)) & MtrrValidBitsMask) + 1;
      (*MtrrCount)++;
    }
  }

  GetEffectiveMemoryRanges (DefaultType, PhysicalAddressBits, RawMemoryRanges, *MtrrCount, Ranges, RangeCount);
}

/**
  Return a 32bit random number.

  @param Start  Start of the random number range.
  @param Limit  Limit of the random number range.
  @return 32bit random number
**/
UINT32
Random32 (
  UINT32  Start,
  UINT32  Limit
  )
{
  return (UINT32)(((double)Rand () / RAND_MAX) * (Limit - Start)) + Start;
}

/**
  Return a 64bit random number.

  @param Start  Start of the random number range.
  @param Limit  Limit of the random number range.
  @return 64bit random number
**/
UINT64
Random64 (
  UINT64  Start,
  UINT64  Limit
  )
{
  return (UINT64)(((double)Rand () / RAND_MAX) * (Limit - Start)) + Start;
}

/**
  Generate random MTRR BASE/MASK for a specified type.

  @param PhysicalAddressBits Physical address bits.
  @param CacheType           Cache type.
  @param MtrrPair            Return the random MTRR.
  @param MtrrMemoryRange     Return the random memory range.
**/
VOID
GenerateRandomMtrrPair (
  IN  UINT32                  PhysicalAddressBits,
  IN  MTRR_MEMORY_CACHE_TYPE  CacheType,
  OUT MTRR_VARIABLE_SETTING   *MtrrPair        OPTIONAL,
  OUT MTRR_MEMORY_RANGE       *MtrrMemoryRange OPTIONAL
  )
{
  MSR_IA32_MTRR_PHYSBASE_REGISTER  PhysBase;
  MSR_IA32_MTRR_PHYSMASK_REGISTER  PhysMask;
  UINT32                           SizeShift;
  UINT32                           BaseShift;
  UINT64                           RandomBoundary;
  UINT64                           MaxPhysicalAddress;
  UINT64                           RangeSize;
  UINT64                           RangeBase;
  UINT64                           PhysBasePhyMaskValidBitsMask;

  MaxPhysicalAddress = 1ull << PhysicalAddressBits;
  do {
    SizeShift = Random32 (12, PhysicalAddressBits - 1);
    RangeSize = 1ull << SizeShift;

    BaseShift      = Random32 (SizeShift, PhysicalAddressBits - 1);
    RandomBoundary = Random64 (0, 1ull << (PhysicalAddressBits - BaseShift));
    RangeBase      = RandomBoundary << BaseShift;
  } while (RangeBase < SIZE_1MB || RangeBase > MaxPhysicalAddress - 1);

  PhysBasePhyMaskValidBitsMask = (MaxPhysicalAddress - 1) & 0xfffffffffffff000ULL;

  PhysBase.Uint64    = 0;
  PhysBase.Bits.Type = CacheType;
  PhysBase.Uint64   |= RangeBase & PhysBasePhyMaskValidBitsMask;
  PhysMask.Uint64    = 0;
  PhysMask.Bits.V    = 1;
  PhysMask.Uint64   |= ((~RangeSize) + 1) & PhysBasePhyMaskValidBitsMask;

  if (MtrrPair != NULL) {
    MtrrPair->Base = PhysBase.Uint64;
    MtrrPair->Mask = PhysMask.Uint64;
  }

  if (MtrrMemoryRange != NULL) {
    MtrrMemoryRange->BaseAddress = RangeBase;
    MtrrMemoryRange->Length      = RangeSize;
    MtrrMemoryRange->Type        = CacheType;
  }
}

/**
  Check whether the Range overlaps with any one in Ranges.

  @param Range  The memory range to check.
  @param Ranges The memory ranges.
  @param Count  Count of memory ranges.

  @return TRUE when overlap exists.
**/
BOOLEAN
RangesOverlap (
  IN MTRR_MEMORY_RANGE  *Range,
  IN MTRR_MEMORY_RANGE  *Ranges,
  IN UINTN              Count
  )
{
  while (Count-- != 0) {
    //
    // Two ranges overlap when:
    // 1. range#2.base is in the middle of range#1
    // 2. range#1.base is in the middle of range#2
    //
    if (  ((Range->BaseAddress <= Ranges[Count].BaseAddress) && (Ranges[Count].BaseAddress < Range->BaseAddress + Range->Length))
       || ((Ranges[Count].BaseAddress <= Range->BaseAddress) && (Range->BaseAddress < Ranges[Count].BaseAddress + Ranges[Count].Length)))
    {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Generate random MTRRs.

  @param PhysicalAddressBits  Physical address bits.
  @param RawMemoryRanges      Return the randomly generated MTRRs.
  @param UcCount              Count of Uncacheable MTRRs.
  @param WtCount              Count of Write Through MTRRs.
  @param WbCount              Count of Write Back MTRRs.
  @param WpCount              Count of Write Protected MTRRs.
  @param WcCount              Count of Write Combine MTRRs.
**/
VOID
GenerateValidAndConfigurableMtrrPairs (
  IN     UINT32             PhysicalAddressBits,
  IN OUT MTRR_MEMORY_RANGE  *RawMemoryRanges,
  IN     UINT32             UcCount,
  IN     UINT32             WtCount,
  IN     UINT32             WbCount,
  IN     UINT32             WpCount,
  IN     UINT32             WcCount
  )
{
  UINT32  Index;

  //
  // 1. Generate UC, WT, WB in order.
  //
  for (Index = 0; Index < UcCount; Index++) {
    GenerateRandomMtrrPair (PhysicalAddressBits, CacheUncacheable, NULL, &RawMemoryRanges[Index]);
  }

  for (Index = UcCount; Index < UcCount + WtCount; Index++) {
    GenerateRandomMtrrPair (PhysicalAddressBits, CacheWriteThrough, NULL, &RawMemoryRanges[Index]);
  }

  for (Index = UcCount + WtCount; Index < UcCount + WtCount + WbCount; Index++) {
    GenerateRandomMtrrPair (PhysicalAddressBits, CacheWriteBack, NULL, &RawMemoryRanges[Index]);
  }

  //
  // 2. Generate WP MTRR and DO NOT overlap with WT, WB.
  //
  for (Index = UcCount + WtCount + WbCount; Index < UcCount + WtCount + WbCount + WpCount; Index++) {
    GenerateRandomMtrrPair (PhysicalAddressBits, CacheWriteProtected, NULL, &RawMemoryRanges[Index]);
    while (RangesOverlap (&RawMemoryRanges[Index], &RawMemoryRanges[UcCount], WtCount + WbCount)) {
      GenerateRandomMtrrPair (PhysicalAddressBits, CacheWriteProtected, NULL, &RawMemoryRanges[Index]);
    }
  }

  //
  // 3. Generate WC MTRR and DO NOT overlap with WT, WB, WP.
  //
  for (Index = UcCount + WtCount + WbCount + WpCount; Index < UcCount + WtCount + WbCount + WpCount + WcCount; Index++) {
    GenerateRandomMtrrPair (PhysicalAddressBits, CacheWriteCombining, NULL, &RawMemoryRanges[Index]);
    while (RangesOverlap (&RawMemoryRanges[Index], &RawMemoryRanges[UcCount], WtCount + WbCount + WpCount)) {
      GenerateRandomMtrrPair (PhysicalAddressBits, CacheWriteCombining, NULL, &RawMemoryRanges[Index]);
    }
  }
}

/**
  Return a random memory cache type.
**/
MTRR_MEMORY_CACHE_TYPE
GenerateRandomCacheType (
  VOID
  )
{
  return mMemoryCacheTypes[Random32 (0, ARRAY_SIZE (mMemoryCacheTypes) - 1)];
}

/**
  Compare function used by qsort().
**/

/**
  Compare function used by qsort().

  @param Left   Left operand to compare.
  @param Right  Right operand to compare.

  @retval 0  Left == Right
  @retval -1 Left < Right
  @retval 1  Left > Right
**/
INT32
CompareFuncUint64 (
  CONST VOID  *Left,
  CONST VOID  *Right
  )
{
  INT64  Delta;

  Delta = (*(UINT64 *)Left - *(UINT64 *)Right);
  if (Delta > 0) {
    return 1;
  } else if (Delta == 0) {
    return 0;
  } else {
    return -1;
  }
}

/**
  Determin the memory cache type for the Range.

  @param DefaultType Default cache type.
  @param Range       The memory range to determin the cache type.
  @param Ranges      The entire memory ranges.
  @param RangeCount  Count of the entire memory ranges.
**/
VOID
DetermineMemoryCacheType (
  IN     MTRR_MEMORY_CACHE_TYPE  DefaultType,
  IN OUT MTRR_MEMORY_RANGE       *Range,
  IN     MTRR_MEMORY_RANGE       *Ranges,
  IN     UINT32                  RangeCount
  )
{
  UINT32  Index;

  Range->Type = CacheInvalid;
  for (Index = 0; Index < RangeCount; Index++) {
    if (RangesOverlap (Range, &Ranges[Index], 1)) {
      if (Ranges[Index].Type < Range->Type) {
        Range->Type = Ranges[Index].Type;
      }
    }
  }

  if (Range->Type == CacheInvalid) {
    Range->Type = DefaultType;
  }
}

/**
  Get the index of the element that does NOT equals to Array[Index].

  @param Index   Current element.
  @param Array   Array to scan.
  @param Count   Count of the array.

  @return Next element that doesn't equal to current one.
**/
UINT32
GetNextDifferentElementInSortedArray (
  IN UINT32  Index,
  IN UINT64  *Array,
  IN UINT32  Count
  )
{
  UINT64  CurrentElement;

  CurrentElement = Array[Index];
  while (CurrentElement == Array[Index] && Index < Count) {
    Index++;
  }

  return Index;
}

/**
  Remove the duplicates from the array.

  @param Array  The array to operate on.
  @param Count  Count of the array.
**/
VOID
RemoveDuplicatesInSortedArray (
  IN OUT UINT64  *Array,
  IN OUT UINT32  *Count
  )
{
  UINT32  Index;
  UINT32  NewCount;

  Index    = 0;
  NewCount = 0;
  while (Index < *Count) {
    Array[NewCount] = Array[Index];
    NewCount++;
    Index = GetNextDifferentElementInSortedArray (Index, Array, *Count);
  }

  *Count = NewCount;
}

/**
  Return TRUE when Address is in the Range.

  @param Address The address to check.
  @param Range   The range to check.
  @return TRUE when Address is in the Range.
**/
BOOLEAN
AddressInRange (
  IN UINT64             Address,
  IN MTRR_MEMORY_RANGE  Range
  )
{
  return (Address >= Range.BaseAddress) && (Address <= Range.BaseAddress + Range.Length - 1);
}

/**
  Get the overlap bit flag.

  @param RawMemoryRanges     Raw memory ranges.
  @param RawMemoryRangeCount Count of raw memory ranges.
  @param Address             The address to check.
**/
UINT64
GetOverlapBitFlag (
  IN MTRR_MEMORY_RANGE  *RawMemoryRanges,
  IN UINT32             RawMemoryRangeCount,
  IN UINT64             Address
  )
{
  UINT64  OverlapBitFlag;
  UINT32  Index;

  OverlapBitFlag = 0;
  for (Index = 0; Index < RawMemoryRangeCount; Index++) {
    if (AddressInRange (Address, RawMemoryRanges[Index])) {
      OverlapBitFlag |= (1ull << Index);
    }
  }

  return OverlapBitFlag;
}

/**
  Return the relationship between flags.

  @param Flag1 Flag 1
  @param Flag2 Flag 2

  @retval 0   Flag1 == Flag2
  @retval 1   Flag1 is a subset of Flag2
  @retval 2   Flag2 is a subset of Flag1
  @retval 3   No subset relations between Flag1 and Flag2.
**/
UINT32
CheckOverlapBitFlagsRelation (
  IN UINT64  Flag1,
  IN UINT64  Flag2
  )
{
  if (Flag1 == Flag2) {
    return 0;
  }

  if ((Flag1 | Flag2) == Flag2) {
    return 1;
  }

  if ((Flag1 | Flag2) == Flag1) {
    return 2;
  }

  return 3;
}

/**
  Return TRUE when the Endpoint is in any of the Ranges.

  @param Endpoint    The endpoint to check.
  @param Ranges      The memory ranges.
  @param RangeCount  Count of memory ranges.

  @retval TRUE  Endpoint is in one of the range.
  @retval FALSE Endpoint is not in any of the ranges.
**/
BOOLEAN
IsEndpointInRanges (
  IN UINT64             Endpoint,
  IN MTRR_MEMORY_RANGE  *Ranges,
  IN UINTN              RangeCount
  )
{
  UINT32  Index;

  for (Index = 0; Index < RangeCount; Index++) {
    if (AddressInRange (Endpoint, Ranges[Index])) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Compact adjacent ranges of the same type.

  @param DefaultType                    Default memory type.
  @param PhysicalAddressBits            Physical address bits.
  @param EffectiveMtrrMemoryRanges      Memory ranges to compact.
  @param EffectiveMtrrMemoryRangesCount Return the new count of memory ranges.
**/
VOID
CompactAndExtendEffectiveMtrrMemoryRanges (
  IN     MTRR_MEMORY_CACHE_TYPE  DefaultType,
  IN     UINT32                  PhysicalAddressBits,
  IN OUT MTRR_MEMORY_RANGE       **EffectiveMtrrMemoryRanges,
  IN OUT UINTN                   *EffectiveMtrrMemoryRangesCount
  )
{
  UINT64                  MaxAddress;
  UINTN                   NewRangesCountAtMost;
  MTRR_MEMORY_RANGE       *NewRanges;
  UINTN                   NewRangesCountActual;
  MTRR_MEMORY_RANGE       *CurrentRangeInNewRanges;
  MTRR_MEMORY_CACHE_TYPE  CurrentRangeTypeInOldRanges;

  MTRR_MEMORY_RANGE  *OldRanges;
  MTRR_MEMORY_RANGE  OldLastRange;
  UINTN              OldRangesIndex;

  NewRangesCountActual = 0;
  NewRangesCountAtMost = *EffectiveMtrrMemoryRangesCount + 2;   // At most with 2 more range entries.
  NewRanges            = (MTRR_MEMORY_RANGE *)calloc (NewRangesCountAtMost, sizeof (MTRR_MEMORY_RANGE));
  OldRanges            = *EffectiveMtrrMemoryRanges;
  if (OldRanges[0].BaseAddress > 0) {
    NewRanges[NewRangesCountActual].BaseAddress = 0;
    NewRanges[NewRangesCountActual].Length      = OldRanges[0].BaseAddress;
    NewRanges[NewRangesCountActual].Type        = DefaultType;
    NewRangesCountActual++;
  }

  OldRangesIndex = 0;
  while (OldRangesIndex < *EffectiveMtrrMemoryRangesCount) {
    CurrentRangeTypeInOldRanges = OldRanges[OldRangesIndex].Type;
    CurrentRangeInNewRanges     = NULL;
    if (NewRangesCountActual > 0) {
      // We need to check CurrentNewRange first before generate a new NewRange.
      CurrentRangeInNewRanges = &NewRanges[NewRangesCountActual - 1];
    }

    if ((CurrentRangeInNewRanges != NULL) && (CurrentRangeInNewRanges->Type == CurrentRangeTypeInOldRanges)) {
      CurrentRangeInNewRanges->Length += OldRanges[OldRangesIndex].Length;
    } else {
      NewRanges[NewRangesCountActual].BaseAddress = OldRanges[OldRangesIndex].BaseAddress;
      NewRanges[NewRangesCountActual].Length     += OldRanges[OldRangesIndex].Length;
      NewRanges[NewRangesCountActual].Type        = CurrentRangeTypeInOldRanges;
      while (OldRangesIndex + 1 < *EffectiveMtrrMemoryRangesCount && OldRanges[OldRangesIndex + 1].Type == CurrentRangeTypeInOldRanges) {
        OldRangesIndex++;
        NewRanges[NewRangesCountActual].Length += OldRanges[OldRangesIndex].Length;
      }

      NewRangesCountActual++;
    }

    OldRangesIndex++;
  }

  MaxAddress              = (1ull << PhysicalAddressBits) - 1;
  OldLastRange            = OldRanges[(*EffectiveMtrrMemoryRangesCount) - 1];
  CurrentRangeInNewRanges = &NewRanges[NewRangesCountActual - 1];
  if (OldLastRange.BaseAddress + OldLastRange.Length - 1 < MaxAddress) {
    if (CurrentRangeInNewRanges->Type == DefaultType) {
      CurrentRangeInNewRanges->Length = MaxAddress - CurrentRangeInNewRanges->BaseAddress + 1;
    } else {
      NewRanges[NewRangesCountActual].BaseAddress = OldLastRange.BaseAddress + OldLastRange.Length;
      NewRanges[NewRangesCountActual].Length      = MaxAddress - NewRanges[NewRangesCountActual].BaseAddress + 1;
      NewRanges[NewRangesCountActual].Type        = DefaultType;
      NewRangesCountActual++;
    }
  }

  free (*EffectiveMtrrMemoryRanges);
  *EffectiveMtrrMemoryRanges      = NewRanges;
  *EffectiveMtrrMemoryRangesCount = NewRangesCountActual;
}

/**
  Collect all the endpoints in the raw memory ranges.

  @param Endpoints           Return the collected endpoints.
  @param EndPointCount       Return the count of endpoints.
  @param RawMemoryRanges     Raw memory ranges.
  @param RawMemoryRangeCount Count of raw memory ranges.
**/
VOID
CollectEndpoints (
  IN OUT UINT64         *Endpoints,
  IN OUT UINT32         *EndPointCount,
  IN MTRR_MEMORY_RANGE  *RawMemoryRanges,
  IN UINT32             RawMemoryRangeCount
  )
{
  UINT32  Index;
  UINT32  RawRangeIndex;

  ASSERT ((RawMemoryRangeCount << 1) == *EndPointCount);

  for (Index = 0; Index < *EndPointCount; Index += 2) {
    RawRangeIndex        = Index >> 1;
    Endpoints[Index]     = RawMemoryRanges[RawRangeIndex].BaseAddress;
    Endpoints[Index + 1] = RawMemoryRanges[RawRangeIndex].BaseAddress + RawMemoryRanges[RawRangeIndex].Length - 1;
  }

  qsort (Endpoints, *EndPointCount, sizeof (UINT64), CompareFuncUint64);
  RemoveDuplicatesInSortedArray (Endpoints, EndPointCount);
}

/**
  Convert the MTRR BASE/MASK array to memory ranges.

  @param DefaultType          Default memory type.
  @param PhysicalAddressBits  Physical address bits.
  @param RawMemoryRanges      Raw memory ranges.
  @param RawMemoryRangeCount  Count of raw memory ranges.
  @param MemoryRanges         Memory ranges.
  @param MemoryRangeCount     Count of memory ranges.
**/
VOID
GetEffectiveMemoryRanges (
  IN MTRR_MEMORY_CACHE_TYPE  DefaultType,
  IN UINT32                  PhysicalAddressBits,
  IN MTRR_MEMORY_RANGE       *RawMemoryRanges,
  IN UINT32                  RawMemoryRangeCount,
  OUT MTRR_MEMORY_RANGE      *MemoryRanges,
  OUT UINTN                  *MemoryRangeCount
  )
{
  UINTN              Index;
  UINT32             AllEndPointsCount;
  UINT64             *AllEndPointsInclusive;
  UINT32             AllRangePiecesCountMax;
  MTRR_MEMORY_RANGE  *AllRangePieces;
  UINTN              AllRangePiecesCountActual;
  UINT64             OverlapBitFlag1;
  UINT64             OverlapBitFlag2;
  INT32              OverlapFlagRelation;

  if (RawMemoryRangeCount == 0) {
    MemoryRanges[0].BaseAddress = 0;
    MemoryRanges[0].Length      = (1ull << PhysicalAddressBits);
    MemoryRanges[0].Type        = DefaultType;
    *MemoryRangeCount           = 1;
    return;
  }

  AllEndPointsCount      = RawMemoryRangeCount << 1;
  AllEndPointsInclusive  = calloc (AllEndPointsCount, sizeof (UINT64));
  AllRangePiecesCountMax = RawMemoryRangeCount * 3 + 1;
  AllRangePieces         = calloc (AllRangePiecesCountMax, sizeof (MTRR_MEMORY_RANGE));
  CollectEndpoints (AllEndPointsInclusive, &AllEndPointsCount, RawMemoryRanges, RawMemoryRangeCount);

  for (Index = 0, AllRangePiecesCountActual = 0; Index < AllEndPointsCount - 1; Index++) {
    OverlapBitFlag1     = GetOverlapBitFlag (RawMemoryRanges, RawMemoryRangeCount, AllEndPointsInclusive[Index]);
    OverlapBitFlag2     = GetOverlapBitFlag (RawMemoryRanges, RawMemoryRangeCount, AllEndPointsInclusive[Index + 1]);
    OverlapFlagRelation = CheckOverlapBitFlagsRelation (OverlapBitFlag1, OverlapBitFlag2);
    switch (OverlapFlagRelation) {
      case 0:   // [1, 2]
        AllRangePieces[AllRangePiecesCountActual].BaseAddress = AllEndPointsInclusive[Index];
        AllRangePieces[AllRangePiecesCountActual].Length      = AllEndPointsInclusive[Index + 1] - AllEndPointsInclusive[Index] + 1;
        AllRangePiecesCountActual++;
        break;
      case 1:   // [1, 2)
        AllRangePieces[AllRangePiecesCountActual].BaseAddress = AllEndPointsInclusive[Index];
        AllRangePieces[AllRangePiecesCountActual].Length      = (AllEndPointsInclusive[Index + 1] - 1) - AllEndPointsInclusive[Index] + 1;
        AllRangePiecesCountActual++;
        break;
      case 2:   // (1, 2]
        AllRangePieces[AllRangePiecesCountActual].BaseAddress = AllEndPointsInclusive[Index] + 1;
        AllRangePieces[AllRangePiecesCountActual].Length      = AllEndPointsInclusive[Index + 1] - (AllEndPointsInclusive[Index] + 1) + 1;
        AllRangePiecesCountActual++;

        if (!IsEndpointInRanges (AllEndPointsInclusive[Index], AllRangePieces, AllRangePiecesCountActual)) {
          AllRangePieces[AllRangePiecesCountActual].BaseAddress = AllEndPointsInclusive[Index];
          AllRangePieces[AllRangePiecesCountActual].Length      = 1;
          AllRangePiecesCountActual++;
        }

        break;
      case 3:   // (1, 2)
        AllRangePieces[AllRangePiecesCountActual].BaseAddress = AllEndPointsInclusive[Index] + 1;
        AllRangePieces[AllRangePiecesCountActual].Length      = (AllEndPointsInclusive[Index + 1] - 1) - (AllEndPointsInclusive[Index] + 1) + 1;
        if (AllRangePieces[AllRangePiecesCountActual].Length == 0) {
          // Only in case 3 can exists Length=0, we should skip such "segment".
          break;
        }

        AllRangePiecesCountActual++;
        if (!IsEndpointInRanges (AllEndPointsInclusive[Index], AllRangePieces, AllRangePiecesCountActual)) {
          AllRangePieces[AllRangePiecesCountActual].BaseAddress = AllEndPointsInclusive[Index];
          AllRangePieces[AllRangePiecesCountActual].Length      = 1;
          AllRangePiecesCountActual++;
        }

        break;
      default:
        ASSERT (FALSE);
    }
  }

  for (Index = 0; Index < AllRangePiecesCountActual; Index++) {
    DetermineMemoryCacheType (DefaultType, &AllRangePieces[Index], RawMemoryRanges, RawMemoryRangeCount);
  }

  CompactAndExtendEffectiveMtrrMemoryRanges (DefaultType, PhysicalAddressBits, &AllRangePieces, &AllRangePiecesCountActual);
  ASSERT (*MemoryRangeCount >= AllRangePiecesCountActual);
  memcpy (MemoryRanges, AllRangePieces, AllRangePiecesCountActual * sizeof (MTRR_MEMORY_RANGE));
  *MemoryRangeCount = AllRangePiecesCountActual;

  free (AllEndPointsInclusive);
  free (AllRangePieces);
}
