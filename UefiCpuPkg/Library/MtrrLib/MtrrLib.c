/** @file
  MTRR setting library

  Copyright (c) 2008 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>

#include <Library/MtrrLib.h>
#include <Library/BaseLib.h>
#include <Library/CpuLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

//
// Context to save and restore when MTRRs are programmed
//
typedef struct {
  UINTN    Cr4;
  BOOLEAN  InterruptState;
} MTRR_CONTEXT;

//
// This table defines the offset, base and length of the fixed MTRRs
//
CONST FIXED_MTRR  mMtrrLibFixedMtrrTable[] = {
  {
    MTRR_LIB_IA32_MTRR_FIX64K_00000,
    0,
    SIZE_64KB
  },
  {
    MTRR_LIB_IA32_MTRR_FIX16K_80000,
    0x80000,
    SIZE_16KB
  },
  {
    MTRR_LIB_IA32_MTRR_FIX16K_A0000,
    0xA0000,
    SIZE_16KB
  },
  {
    MTRR_LIB_IA32_MTRR_FIX4K_C0000,
    0xC0000,
    SIZE_4KB
  },
  {
    MTRR_LIB_IA32_MTRR_FIX4K_C8000,
    0xC8000,
    SIZE_4KB
  },
  {
    MTRR_LIB_IA32_MTRR_FIX4K_D0000,
    0xD0000,
    SIZE_4KB
  },
  {
    MTRR_LIB_IA32_MTRR_FIX4K_D8000,
    0xD8000,
    SIZE_4KB
  },
  {
    MTRR_LIB_IA32_MTRR_FIX4K_E0000,
    0xE0000,
    SIZE_4KB
  },
  {
    MTRR_LIB_IA32_MTRR_FIX4K_E8000,
    0xE8000,
    SIZE_4KB
  },
  {
    MTRR_LIB_IA32_MTRR_FIX4K_F0000,
    0xF0000,
    SIZE_4KB
  },
  {
    MTRR_LIB_IA32_MTRR_FIX4K_F8000,
    0xF8000,
    SIZE_4KB
  },
};

//
// Lookup table used to print MTRRs
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR8 *mMtrrMemoryCacheTypeShortName[] = {
  "UC",  // CacheUncacheable
  "WC",  // CacheWriteCombining
  "R*",  // Invalid
  "R*",  // Invalid
  "WT",  // CacheWriteThrough
  "WP",  // CacheWriteProtected
  "WB",  // CacheWriteBack
  "R*"   // Invalid
};

/**
  Returns the variable MTRR count for the CPU.

  @return Variable MTRR count

**/
UINT32
EFIAPI
GetVariableMtrrCount (
  VOID
  )
{
  UINT32  VariableMtrrCount;

  if (!IsMtrrSupported ()) {
    return 0;
  }

  VariableMtrrCount = (UINT32)(AsmReadMsr64 (MTRR_LIB_IA32_MTRR_CAP) & MTRR_LIB_IA32_MTRR_CAP_VCNT_MASK);
  ASSERT (VariableMtrrCount <= MTRR_NUMBER_OF_VARIABLE_MTRR);

  return VariableMtrrCount;
}

/**
  Returns the firmware usable variable MTRR count for the CPU.

  @return Firmware usable variable MTRR count

**/
UINT32
EFIAPI
GetFirmwareVariableMtrrCount (
  VOID
  )
{
  UINT32  VariableMtrrCount;

  VariableMtrrCount = GetVariableMtrrCount ();
  if (VariableMtrrCount < RESERVED_FIRMWARE_VARIABLE_MTRR_NUMBER) {
    return 0;
  }

  return VariableMtrrCount - RESERVED_FIRMWARE_VARIABLE_MTRR_NUMBER;
}

/**
  Returns the default MTRR cache type for the system.

  @return  The default MTRR cache type.

**/
MTRR_MEMORY_CACHE_TYPE
EFIAPI
MtrrGetDefaultMemoryType (
  VOID
  )
{
  if (!IsMtrrSupported ()) {
    return CacheUncacheable;
  }

  return (MTRR_MEMORY_CACHE_TYPE) (AsmReadMsr64 (MTRR_LIB_IA32_MTRR_DEF_TYPE) & 0x7);
}

/**
  Preparation before programming MTRR.

  This function will do some preparation for programming MTRRs:
  disable cache, invalid cache and disable MTRR caching functionality

  @param[out] MtrrContext  Pointer to context to save

**/
VOID
PreMtrrChange (
  OUT MTRR_CONTEXT  *MtrrContext
  )
{
  //
  // Disable interrupts and save current interrupt state
  //
  MtrrContext->InterruptState = SaveAndDisableInterrupts();
  
  //
  // Enter no fill cache mode, CD=1(Bit30), NW=0 (Bit29)
  //
  AsmDisableCache ();

  //
  // Save original CR4 value and clear PGE flag (Bit 7)
  //
  MtrrContext->Cr4 = AsmReadCr4 ();
  AsmWriteCr4 (MtrrContext->Cr4 & (~BIT7));

  //
  // Flush all TLBs
  //
  CpuFlushTlb ();

  //
  // Disable Mtrrs
  //
  AsmMsrBitFieldWrite64 (MTRR_LIB_IA32_MTRR_DEF_TYPE, 10, 11, 0);
}

/**
  Cleaning up after programming MTRRs.

  This function will do some clean up after programming MTRRs:
  Flush all TLBs,  re-enable caching, restore CR4.

  @param[in] MtrrContext  Pointer to context to restore

**/
VOID
PostMtrrChangeEnableCache (
  IN MTRR_CONTEXT  *MtrrContext
  )
{
  //
  // Flush all TLBs 
  //
  CpuFlushTlb ();

  //
  // Enable Normal Mode caching CD=NW=0, CD(Bit30), NW(Bit29)
  //
  AsmEnableCache ();

  //
  // Restore original CR4 value
  //
  AsmWriteCr4 (MtrrContext->Cr4);
  
  //
  // Restore original interrupt state
  //
  SetInterruptState (MtrrContext->InterruptState);
}

/**
  Cleaning up after programming MTRRs.

  This function will do some clean up after programming MTRRs:
  enable MTRR caching functionality, and enable cache

  @param[in] MtrrContext  Pointer to context to restore

**/
VOID
PostMtrrChange (
  IN MTRR_CONTEXT  *MtrrContext
  )
{
  //
  // Enable Cache MTRR
  //
  AsmMsrBitFieldWrite64 (MTRR_LIB_IA32_MTRR_DEF_TYPE, 10, 11, 3);

  PostMtrrChangeEnableCache (MtrrContext);
}


/**
  Programs fixed MTRRs registers.

  @param  MemoryCacheType  The memory type to set.
  @param  Base             The base address of memory range.
  @param  Length           The length of memory range.

  @retval RETURN_SUCCESS      The cache type was updated successfully
  @retval RETURN_UNSUPPORTED  The requested range or cache type was invalid
                              for the fixed MTRRs.

**/
RETURN_STATUS
ProgramFixedMtrr (
  IN     UINT64     MemoryCacheType,
  IN OUT UINT64     *Base,
  IN OUT UINT64     *Length
  )
{
  UINT32  MsrNum;
  UINT32  ByteShift;
  UINT64  TempQword;
  UINT64  OrMask;
  UINT64  ClearMask;

  TempQword = 0;
  OrMask    = 0;
  ClearMask = 0;

  for (MsrNum = 0; MsrNum < MTRR_NUMBER_OF_FIXED_MTRR; MsrNum++) {
    if ((*Base >= mMtrrLibFixedMtrrTable[MsrNum].BaseAddress) &&
        (*Base <
            (
              mMtrrLibFixedMtrrTable[MsrNum].BaseAddress +
              (8 * mMtrrLibFixedMtrrTable[MsrNum].Length)
            )
          )
        ) {
      break;
    }
  }

  if (MsrNum == MTRR_NUMBER_OF_FIXED_MTRR) {
    return RETURN_UNSUPPORTED;
  }

  //
  // We found the fixed MTRR to be programmed
  //
  for (ByteShift = 0; ByteShift < 8; ByteShift++) {
    if (*Base ==
         (
           mMtrrLibFixedMtrrTable[MsrNum].BaseAddress +
           (ByteShift * mMtrrLibFixedMtrrTable[MsrNum].Length)
         )
       ) {
      break;
    }
  }

  if (ByteShift == 8) {
    return RETURN_UNSUPPORTED;
  }

  for (
        ;
        ((ByteShift < 8) && (*Length >= mMtrrLibFixedMtrrTable[MsrNum].Length));
        ByteShift++
      ) {
    OrMask |= LShiftU64 ((UINT64) MemoryCacheType, (UINT32) (ByteShift * 8));
    ClearMask |= LShiftU64 ((UINT64) 0xFF, (UINT32) (ByteShift * 8));
    *Length -= mMtrrLibFixedMtrrTable[MsrNum].Length;
    *Base += mMtrrLibFixedMtrrTable[MsrNum].Length;
  }

  if (ByteShift < 8 && (*Length != 0)) {
    return RETURN_UNSUPPORTED;
  }

  TempQword =
    (AsmReadMsr64 (mMtrrLibFixedMtrrTable[MsrNum].Msr) & ~ClearMask) | OrMask;
  AsmWriteMsr64 (mMtrrLibFixedMtrrTable[MsrNum].Msr, TempQword);
  return RETURN_SUCCESS;
}


/**
  Get the attribute of variable MTRRs.

  This function shadows the content of variable MTRRs into an
  internal array: VariableMtrr.

  @param  MtrrValidBitsMask     The mask for the valid bit of the MTRR
  @param  MtrrValidAddressMask  The valid address mask for MTRR
  @param  VariableMtrr          The array to shadow variable MTRRs content

  @return                       The return value of this paramter indicates the
                                number of MTRRs which has been used.

**/
UINT32
EFIAPI
MtrrGetMemoryAttributeInVariableMtrr (
  IN  UINT64                    MtrrValidBitsMask,
  IN  UINT64                    MtrrValidAddressMask,
  OUT VARIABLE_MTRR             *VariableMtrr
  )
{
  UINTN   Index;
  UINT32  MsrNum;
  UINT32  UsedMtrr;
  UINT32  FirmwareVariableMtrrCount;
  UINT32  VariableMtrrEnd;

  if (!IsMtrrSupported ()) {
    return 0;
  }

  FirmwareVariableMtrrCount = GetFirmwareVariableMtrrCount ();
  VariableMtrrEnd = MTRR_LIB_IA32_VARIABLE_MTRR_BASE + (2 * GetVariableMtrrCount ()) - 1;

  ZeroMem (VariableMtrr, sizeof (VARIABLE_MTRR) * MTRR_NUMBER_OF_VARIABLE_MTRR);
  UsedMtrr = 0;

  for (MsrNum = MTRR_LIB_IA32_VARIABLE_MTRR_BASE, Index = 0;
       (
         (MsrNum < VariableMtrrEnd) &&
         (Index < FirmwareVariableMtrrCount)
       );
       MsrNum += 2
      ) {
    if ((AsmReadMsr64 (MsrNum + 1) & MTRR_LIB_CACHE_MTRR_ENABLED) != 0) {
      VariableMtrr[Index].Msr          = MsrNum;
      VariableMtrr[Index].BaseAddress  = (AsmReadMsr64 (MsrNum) &
                                          MtrrValidAddressMask);
      VariableMtrr[Index].Length       = ((~(AsmReadMsr64 (MsrNum + 1) &
                                             MtrrValidAddressMask)
                                          ) &
                                          MtrrValidBitsMask
                                         ) + 1;
      VariableMtrr[Index].Type         = (AsmReadMsr64 (MsrNum) & 0x0ff);
      VariableMtrr[Index].Valid        = TRUE;
      VariableMtrr[Index].Used         = TRUE;
      UsedMtrr = UsedMtrr  + 1;
      Index++;
    }
  }
  return UsedMtrr;
}


/**
  Checks overlap between given memory range and MTRRs.

  @param  Start            The start address of memory range.
  @param  End              The end address of memory range.
  @param  VariableMtrr     The array to shadow variable MTRRs content

  @retval TRUE             Overlap exists.
  @retval FALSE            No overlap.

**/
BOOLEAN
CheckMemoryAttributeOverlap (
  IN PHYSICAL_ADDRESS     Start,
  IN PHYSICAL_ADDRESS     End,
  IN VARIABLE_MTRR      *VariableMtrr
  )
{
  UINT32  Index;

  for (Index = 0; Index < 6; Index++) {
    if (
         VariableMtrr[Index].Valid &&
         !(
           (Start > (VariableMtrr[Index].BaseAddress +
                     VariableMtrr[Index].Length - 1)
           ) ||
           (End < VariableMtrr[Index].BaseAddress)
         )
       ) {
      return TRUE;
    }
  }

  return FALSE;
}


/**
  Marks a variable MTRR as non-valid.

  @param  Index         The index of the array VariableMtrr to be invalidated
  @param  VariableMtrr  The array to shadow variable MTRRs content
  @param  UsedMtrr      The number of MTRRs which has already been used

**/
VOID
InvalidateShadowMtrr (
  IN   UINTN              Index,
  IN   VARIABLE_MTRR      *VariableMtrr,
  OUT  UINT32             *UsedMtrr
  )
{
  VariableMtrr[Index].Valid = FALSE;
  *UsedMtrr = *UsedMtrr - 1;
}


/**
  Combine memory attributes.

  If overlap exists between given memory range and MTRRs, try to combine them.

  @param  Attributes             The memory type to set.
  @param  Base                   The base address of memory range.
  @param  Length                 The length of memory range.
  @param  VariableMtrr           The array to shadow variable MTRRs content
  @param  UsedMtrr               The number of MTRRs which has already been used
  @param  OverwriteExistingMtrr  Returns whether an existing MTRR was used

  @retval EFI_SUCCESS            Memory region successfully combined.
  @retval EFI_ACCESS_DENIED      Memory region cannot be combined.

**/
RETURN_STATUS
CombineMemoryAttribute (
  IN     UINT64             Attributes,
  IN OUT UINT64             *Base,
  IN OUT UINT64             *Length,
  IN     VARIABLE_MTRR      *VariableMtrr,
  IN OUT UINT32             *UsedMtrr,
  OUT    BOOLEAN            *OverwriteExistingMtrr
  )
{
  UINT32  Index;
  UINT64  CombineStart;
  UINT64  CombineEnd;
  UINT64  MtrrEnd;
  UINT64  EndAddress;
  UINT32  FirmwareVariableMtrrCount;
  BOOLEAN CoveredByExistingMtrr;

  FirmwareVariableMtrrCount = GetFirmwareVariableMtrrCount ();

  *OverwriteExistingMtrr = FALSE;
  CoveredByExistingMtrr = FALSE;
  EndAddress = *Base +*Length - 1;

  for (Index = 0; Index < FirmwareVariableMtrrCount; Index++) {

    MtrrEnd = VariableMtrr[Index].BaseAddress + VariableMtrr[Index].Length - 1;
    if (
         !VariableMtrr[Index].Valid ||
         (
           *Base > (MtrrEnd) ||
           (EndAddress < VariableMtrr[Index].BaseAddress)
         )
       ) {
      continue;
    }

    //
    // Combine same attribute MTRR range
    //
    if (Attributes == VariableMtrr[Index].Type) {
      //
      // if the Mtrr range contain the request range, set a flag, then continue to 
      // invalidate any MTRR of the same request range with higher priority cache type.
      //
      if (VariableMtrr[Index].BaseAddress <= *Base && MtrrEnd >= EndAddress) {
        CoveredByExistingMtrr = TRUE;
        continue;
      }
      //
      // invalid this MTRR, and program the combine range
      //
      CombineStart  =
        (*Base) < VariableMtrr[Index].BaseAddress ?
          (*Base) :
          VariableMtrr[Index].BaseAddress;
      CombineEnd    = EndAddress > MtrrEnd ? EndAddress : MtrrEnd;

      //
      // Record the MTRR usage status in VariableMtrr array.
      //
      InvalidateShadowMtrr (Index, VariableMtrr, UsedMtrr);
      *Base       = CombineStart;
      *Length     = CombineEnd - CombineStart + 1;
      EndAddress  = CombineEnd;
      *OverwriteExistingMtrr = TRUE;
      continue;
    } else {
      //
      // The cache type is different, but the range is convered by one MTRR
      //
      if (VariableMtrr[Index].BaseAddress == *Base && MtrrEnd == EndAddress) {
        InvalidateShadowMtrr (Index, VariableMtrr, UsedMtrr);
        continue;
      }

    }

    if ((Attributes== MTRR_CACHE_WRITE_THROUGH &&
         VariableMtrr[Index].Type == MTRR_CACHE_WRITE_BACK) ||
        (Attributes == MTRR_CACHE_WRITE_BACK &&
         VariableMtrr[Index].Type == MTRR_CACHE_WRITE_THROUGH) ||
        (Attributes == MTRR_CACHE_UNCACHEABLE) ||
        (VariableMtrr[Index].Type == MTRR_CACHE_UNCACHEABLE)
     ) {
      *OverwriteExistingMtrr = TRUE;
      continue;
    }
    //
    // Other type memory overlap is invalid
    //
    return RETURN_ACCESS_DENIED;
  }

  if (CoveredByExistingMtrr) {
    *Length = 0;
  }

  return RETURN_SUCCESS;
}


/**
  Calculate the maximum value which is a power of 2, but less the MemoryLength.

  @param  MemoryLength        The number to pass in.
  @return The maximum value which is align to power of 2 and less the MemoryLength

**/
UINT64
Power2MaxMemory (
  IN UINT64                     MemoryLength
  )
{
  UINT64  Result;

  if (RShiftU64 (MemoryLength, 32) != 0) {
    Result = LShiftU64 (
               (UINT64) GetPowerOfTwo32 (
                          (UINT32) RShiftU64 (MemoryLength, 32)
                          ),
               32
               );
  } else {
    Result = (UINT64) GetPowerOfTwo32 ((UINT32) MemoryLength);
  }

  return Result;
}


/**
  Determine the MTRR numbers used to program a memory range.

  This function first checks the alignment of the base address. If the alignment of the base address <= Length,
  cover the memory range (BaseAddress, alignment) by a MTRR, then BaseAddress += alignment and Length -= alignment.
  Repeat the step until alignment > Length.

  Then this function determines which direction of programming the variable MTRRs for the remaining length
  will use fewer MTRRs.

  @param  BaseAddress Length of Memory to program MTRR
  @param  Length      Length of Memory to program MTRR
  @param  MtrrNumber  Pointer to the number of necessary MTRRs

  @retval TRUE        Positive direction is better.
          FALSE       Negtive direction is better.

**/
BOOLEAN
GetMtrrNumberAndDirection (
  IN UINT64      BaseAddress,
  IN UINT64      Length,
  IN UINTN       *MtrrNumber
  )
{
  UINT64  TempQword;
  UINT64  Alignment;
  UINT32  Positive;
  UINT32  Subtractive;

  *MtrrNumber = 0;

  if (BaseAddress != 0) {
    do {
      //
      // Calculate the alignment of the base address.
      //
      Alignment = LShiftU64 (1, (UINTN)LowBitSet64 (BaseAddress));

      if (Alignment > Length) {
        break;
      }

      (*MtrrNumber)++;
      BaseAddress += Alignment;
      Length -= Alignment;
    } while (TRUE);

    if (Length == 0) {
      return TRUE;
    }
  }

  TempQword   = Length;
  Positive    = 0;
  Subtractive = 0;

  do {
    TempQword -= Power2MaxMemory (TempQword);
    Positive++;
  } while (TempQword != 0);

  TempQword = Power2MaxMemory (LShiftU64 (Length, 1)) - Length;
  Subtractive++;
  do {
    TempQword -= Power2MaxMemory (TempQword);
    Subtractive++;
  } while (TempQword != 0);

  if (Positive <= Subtractive) {
    *MtrrNumber += Positive;
    return TRUE;
  } else {
    *MtrrNumber += Subtractive;
    return FALSE;
  }
}

/**
  Invalid variable MTRRs according to the value in the shadow array.

  This function programs MTRRs according to the values specified
  in the shadow array.

  @param  VariableMtrr   The array to shadow variable MTRRs content

**/
VOID
InvalidateMtrr (
   IN     VARIABLE_MTRR      *VariableMtrr
   )
{
  UINTN         Index;
  UINTN         VariableMtrrCount;
  MTRR_CONTEXT  MtrrContext;

  PreMtrrChange (&MtrrContext);
  Index = 0;
  VariableMtrrCount = GetVariableMtrrCount ();
  while (Index < VariableMtrrCount) {
    if (!VariableMtrr[Index].Valid && VariableMtrr[Index].Used) {
       AsmWriteMsr64 (VariableMtrr[Index].Msr, 0);
       AsmWriteMsr64 (VariableMtrr[Index].Msr + 1, 0);
       VariableMtrr[Index].Used = FALSE;
    }
    Index ++;
  }
  PostMtrrChange (&MtrrContext);
}


/**
  Programs variable MTRRs

  This function programs variable MTRRs

  @param  MtrrNumber            Index of MTRR to program.
  @param  BaseAddress           Base address of memory region.
  @param  Length                Length of memory region.
  @param  MemoryCacheType       Memory type to set.
  @param  MtrrValidAddressMask  The valid address mask for MTRR

**/
VOID
ProgramVariableMtrr (
  IN UINTN                    MtrrNumber,
  IN PHYSICAL_ADDRESS         BaseAddress,
  IN UINT64                   Length,
  IN UINT64                   MemoryCacheType,
  IN UINT64                   MtrrValidAddressMask
  )
{
  UINT64        TempQword;
  MTRR_CONTEXT  MtrrContext;

  PreMtrrChange (&MtrrContext);

  //
  // MTRR Physical Base
  //
  TempQword = (BaseAddress & MtrrValidAddressMask) | MemoryCacheType;
  AsmWriteMsr64 ((UINT32) MtrrNumber, TempQword);

  //
  // MTRR Physical Mask
  //
  TempQword = ~(Length - 1);
  AsmWriteMsr64 (
    (UINT32) (MtrrNumber + 1),
    (TempQword & MtrrValidAddressMask) | MTRR_LIB_CACHE_MTRR_ENABLED
    );

  PostMtrrChange (&MtrrContext);
}


/**
  Convert the Memory attibute value to MTRR_MEMORY_CACHE_TYPE.

  @param  MtrrType  MTRR memory type

  @return The enum item in MTRR_MEMORY_CACHE_TYPE

**/
MTRR_MEMORY_CACHE_TYPE
GetMemoryCacheTypeFromMtrrType (
  IN UINT64                MtrrType
  )
{
  switch (MtrrType) {
  case MTRR_CACHE_UNCACHEABLE:
    return CacheUncacheable;
  case MTRR_CACHE_WRITE_COMBINING:
    return CacheWriteCombining;
  case MTRR_CACHE_WRITE_THROUGH:
    return CacheWriteThrough;
  case MTRR_CACHE_WRITE_PROTECTED:
    return CacheWriteProtected;
  case MTRR_CACHE_WRITE_BACK:
    return CacheWriteBack;
  default:
    //
    // MtrrType is MTRR_CACHE_INVALID_TYPE, that means
    // no mtrr covers the range
    //
    return MtrrGetDefaultMemoryType ();
  }
}

/**
  Initializes the valid bits mask and valid address mask for MTRRs.

  This function initializes the valid bits mask and valid address mask for MTRRs.

  @param  MtrrValidBitsMask     The mask for the valid bit of the MTRR
  @param  MtrrValidAddressMask  The valid address mask for the MTRR

**/
VOID
MtrrLibInitializeMtrrMask (
  OUT UINT64 *MtrrValidBitsMask,
  OUT UINT64 *MtrrValidAddressMask
  )
{
  UINT32  RegEax;
  UINT8   PhysicalAddressBits;

  AsmCpuid (0x80000000, &RegEax, NULL, NULL, NULL);

  if (RegEax >= 0x80000008) {
    AsmCpuid (0x80000008, &RegEax, NULL, NULL, NULL);

    PhysicalAddressBits = (UINT8) RegEax;

    *MtrrValidBitsMask    = LShiftU64 (1, PhysicalAddressBits) - 1;
    *MtrrValidAddressMask = *MtrrValidBitsMask & 0xfffffffffffff000ULL;
  } else {
    *MtrrValidBitsMask    = MTRR_LIB_CACHE_VALID_ADDRESS;
    *MtrrValidAddressMask = 0xFFFFFFFF;
  }
}


/**
  Determing the real attribute of a memory range.

  This function is to arbitrate the real attribute of the memory when
  there are 2 MTRR covers the same memory range.  For further details,
  please refer the IA32 Software Developer's Manual, Volume 3,
  Section 10.11.4.1.

  @param  MtrrType1    the first kind of Memory type
  @param  MtrrType2    the second kind of memory type

**/
UINT64
MtrrPrecedence (
  UINT64    MtrrType1,
  UINT64    MtrrType2
  )
{
  UINT64 MtrrType;

  MtrrType = MTRR_CACHE_INVALID_TYPE;
  switch (MtrrType1) {
  case MTRR_CACHE_UNCACHEABLE:
    MtrrType = MTRR_CACHE_UNCACHEABLE;
    break;
  case MTRR_CACHE_WRITE_COMBINING:
    if (
         MtrrType2==MTRR_CACHE_WRITE_COMBINING ||
         MtrrType2==MTRR_CACHE_UNCACHEABLE
       ) {
      MtrrType = MtrrType2;
    }
    break;
  case MTRR_CACHE_WRITE_THROUGH:
    if (
         MtrrType2==MTRR_CACHE_WRITE_THROUGH ||
         MtrrType2==MTRR_CACHE_WRITE_BACK
       ) {
      MtrrType = MTRR_CACHE_WRITE_THROUGH;
    } else if(MtrrType2==MTRR_CACHE_UNCACHEABLE) {
      MtrrType = MTRR_CACHE_UNCACHEABLE;
    }
    break;
  case MTRR_CACHE_WRITE_PROTECTED:
    if (MtrrType2 == MTRR_CACHE_WRITE_PROTECTED ||
        MtrrType2 == MTRR_CACHE_UNCACHEABLE) {
      MtrrType = MtrrType2;
    }
    break;
  case MTRR_CACHE_WRITE_BACK:
    if (
         MtrrType2== MTRR_CACHE_UNCACHEABLE ||
         MtrrType2==MTRR_CACHE_WRITE_THROUGH ||
         MtrrType2== MTRR_CACHE_WRITE_BACK
       ) {
      MtrrType = MtrrType2;
    }
    break;
  case MTRR_CACHE_INVALID_TYPE:
    MtrrType = MtrrType2;
    break;
  default:
    break;
  }

  if (MtrrType2 == MTRR_CACHE_INVALID_TYPE) {
    MtrrType = MtrrType1;
  }
  return MtrrType;
}


/**
  This function attempts to set the attributes for a memory range.

  @param  BaseAddress            The physical address that is the start
                                 address of a memory region.
  @param  Length                 The size in bytes of the memory region.
  @param  Attributes             The bit mask of attributes to set for the
                                 memory region.

  @retval RETURN_SUCCESS            The attributes were set for the memory
                                    region.
  @retval RETURN_INVALID_PARAMETER  Length is zero.
  @retval RETURN_UNSUPPORTED        The processor does not support one or
                                    more bytes of the memory resource range
                                    specified by BaseAddress and Length.
  @retval RETURN_UNSUPPORTED        The bit mask of attributes is not support
                                    for the memory resource range specified
                                    by BaseAddress and Length.
  @retval RETURN_ACCESS_DENIED      The attributes for the memory resource
                                    range specified by BaseAddress and Length
                                    cannot be modified.
  @retval RETURN_OUT_OF_RESOURCES   There are not enough system resources to
                                    modify the attributes of the memory
                                    resource range.

**/
RETURN_STATUS
EFIAPI
MtrrSetMemoryAttribute (
  IN PHYSICAL_ADDRESS        BaseAddress,
  IN UINT64                  Length,
  IN MTRR_MEMORY_CACHE_TYPE  Attribute
  )
{
  UINT64                    TempQword;
  RETURN_STATUS             Status;
  UINT64                    MemoryType;
  UINT64                    Alignment;
  BOOLEAN                   OverLap;
  BOOLEAN                   Positive;
  UINT32                    MsrNum;
  UINTN                     MtrrNumber;
  VARIABLE_MTRR             VariableMtrr[MTRR_NUMBER_OF_VARIABLE_MTRR];
  UINT32                    UsedMtrr;
  UINT64                    MtrrValidBitsMask;
  UINT64                    MtrrValidAddressMask;
  BOOLEAN                   OverwriteExistingMtrr;
  UINT32                    FirmwareVariableMtrrCount;
  UINT32                    VariableMtrrEnd;
  MTRR_CONTEXT              MtrrContext;

  DEBUG((DEBUG_CACHE, "MtrrSetMemoryAttribute() %a:%016lx-%016lx\n", mMtrrMemoryCacheTypeShortName[Attribute], BaseAddress, Length));

  if (!IsMtrrSupported ()) {
    Status = RETURN_UNSUPPORTED;
    goto Done;
  }

  FirmwareVariableMtrrCount = GetFirmwareVariableMtrrCount ();
  VariableMtrrEnd = MTRR_LIB_IA32_VARIABLE_MTRR_BASE + (2 * GetVariableMtrrCount ()) - 1;

  MtrrLibInitializeMtrrMask(&MtrrValidBitsMask, &MtrrValidAddressMask);

  TempQword = 0;
  MemoryType = (UINT64)Attribute;
  OverwriteExistingMtrr = FALSE;

  //
  // Check for an invalid parameter
  //
  if (Length == 0) {
    Status = RETURN_INVALID_PARAMETER;
    goto Done;
  }

  if (
       (BaseAddress & ~MtrrValidAddressMask) != 0 ||
       (Length & ~MtrrValidAddressMask) != 0
     ) {
    Status = RETURN_UNSUPPORTED;
    goto Done;
  }

  //
  // Check if Fixed MTRR
  //
  Status = RETURN_SUCCESS;
  while ((BaseAddress < BASE_1MB) && (Length > 0) && Status == RETURN_SUCCESS) {
    PreMtrrChange (&MtrrContext);
    Status = ProgramFixedMtrr (MemoryType, &BaseAddress, &Length);
    PostMtrrChange (&MtrrContext);
    if (RETURN_ERROR (Status)) {
      goto Done;
    }
  }

  if (Length == 0) {
    //
    // A Length of 0 can only make sense for fixed MTTR ranges.
    // Since we just handled the fixed MTRRs, we can skip the
    // variable MTRR section.
    //
    goto Done;
  }

  //
  // Since memory ranges below 1MB will be overridden by the fixed MTRRs,
  // we can set the base to 0 to save variable MTRRs.
  //
  if (BaseAddress == BASE_1MB) {
    BaseAddress = 0;
    Length += SIZE_1MB;
  }

  //
  // Check for overlap
  //
  UsedMtrr = MtrrGetMemoryAttributeInVariableMtrr (MtrrValidBitsMask, MtrrValidAddressMask, VariableMtrr);
  OverLap = CheckMemoryAttributeOverlap (BaseAddress, BaseAddress + Length - 1, VariableMtrr);
  if (OverLap) {
    Status = CombineMemoryAttribute (MemoryType, &BaseAddress, &Length, VariableMtrr, &UsedMtrr, &OverwriteExistingMtrr);
    if (RETURN_ERROR (Status)) {
      goto Done;
    }

    if (Length == 0) {
      //
      // Combined successfully, invalidate the now-unused MTRRs
      //
      InvalidateMtrr(VariableMtrr);
      Status = RETURN_SUCCESS;
      goto Done;
    }
  }

  //
  // The memory type is the same with the type specified by
  // MTRR_LIB_IA32_MTRR_DEF_TYPE.
  //
  if ((!OverwriteExistingMtrr) && (Attribute == MtrrGetDefaultMemoryType ())) {
    //
    // Invalidate the now-unused MTRRs
    //
    InvalidateMtrr(VariableMtrr);
    goto Done;
  }

  Positive = GetMtrrNumberAndDirection (BaseAddress, Length, &MtrrNumber);

  if ((UsedMtrr + MtrrNumber) > FirmwareVariableMtrrCount) {
    Status = RETURN_OUT_OF_RESOURCES;
    goto Done;
  }

  //
  // Invalidate the now-unused MTRRs
  //
  InvalidateMtrr(VariableMtrr);

  //
  // Find first unused MTRR
  //
  for (MsrNum = MTRR_LIB_IA32_VARIABLE_MTRR_BASE;
       MsrNum < VariableMtrrEnd;
       MsrNum += 2
      ) {
    if ((AsmReadMsr64 (MsrNum + 1) & MTRR_LIB_CACHE_MTRR_ENABLED) == 0) {
      break;
    }
  }

  if (BaseAddress != 0) {
    do {
      //
      // Calculate the alignment of the base address.
      //
      Alignment = LShiftU64 (1, (UINTN)LowBitSet64 (BaseAddress));

      if (Alignment > Length) {
        break;
      }

      //
      // Find unused MTRR
      //
      for (; MsrNum < VariableMtrrEnd; MsrNum += 2) {
        if ((AsmReadMsr64 (MsrNum + 1) & MTRR_LIB_CACHE_MTRR_ENABLED) == 0) {
          break;
        }
      }

      ProgramVariableMtrr (
        MsrNum,
        BaseAddress,
        Alignment,
        MemoryType,
        MtrrValidAddressMask
        );
      BaseAddress += Alignment;
      Length -= Alignment;
    } while (TRUE);

    if (Length == 0) {
      goto Done;
    }
  }

  TempQword = Length;

  if (!Positive) {
    Length = Power2MaxMemory (LShiftU64 (TempQword, 1));

    //
    // Find unused MTRR
    //
    for (; MsrNum < VariableMtrrEnd; MsrNum += 2) {
      if ((AsmReadMsr64 (MsrNum + 1) & MTRR_LIB_CACHE_MTRR_ENABLED) == 0) {
        break;
      }
    }

    ProgramVariableMtrr (
      MsrNum,
      BaseAddress,
      Length,
      MemoryType,
      MtrrValidAddressMask
      );
    BaseAddress += Length;
    TempQword   = Length - TempQword;
    MemoryType  = MTRR_CACHE_UNCACHEABLE;
  }

  do {
    //
    // Find unused MTRR
    //
    for (; MsrNum < VariableMtrrEnd; MsrNum += 2) {
      if ((AsmReadMsr64 (MsrNum + 1) & MTRR_LIB_CACHE_MTRR_ENABLED) == 0) {
        break;
      }
    }

    Length = Power2MaxMemory (TempQword);
    if (!Positive) {
      BaseAddress -= Length;
    }

    ProgramVariableMtrr (
      MsrNum,
      BaseAddress,
      Length,
      MemoryType,
      MtrrValidAddressMask
      );

    if (Positive) {
      BaseAddress += Length;
    }
    TempQword -= Length;

  } while (TempQword > 0);

Done:
  DEBUG((DEBUG_CACHE, "  Status = %r\n", Status));
  if (!RETURN_ERROR (Status)) {
    MtrrDebugPrintAllMtrrs ();
  }

  return Status;
}


/**
  This function will get the memory cache type of the specific address.

  This function is mainly for debug purpose.

  @param  Address   The specific address

  @return Memory cache type of the sepcific address

**/
MTRR_MEMORY_CACHE_TYPE
EFIAPI
MtrrGetMemoryAttribute (
  IN PHYSICAL_ADDRESS   Address
  )
{
  UINT64                  TempQword;
  UINTN                   Index;
  UINTN                   SubIndex;
  UINT64                  MtrrType;
  UINT64                  TempMtrrType;
  MTRR_MEMORY_CACHE_TYPE  CacheType;
  VARIABLE_MTRR           VariableMtrr[MTRR_NUMBER_OF_VARIABLE_MTRR];
  UINT64                  MtrrValidBitsMask;
  UINT64                  MtrrValidAddressMask;
  UINTN                   VariableMtrrCount;

  if (!IsMtrrSupported ()) {
    return CacheUncacheable;
  }

  //
  // Check if MTRR is enabled, if not, return UC as attribute
  //
  TempQword = AsmReadMsr64 (MTRR_LIB_IA32_MTRR_DEF_TYPE);
  MtrrType = MTRR_CACHE_INVALID_TYPE;

  if ((TempQword & MTRR_LIB_CACHE_MTRR_ENABLED) == 0) {
    return CacheUncacheable;
  }

  //
  // If address is less than 1M, then try to go through the fixed MTRR
  //
  if (Address < BASE_1MB) {
    if ((TempQword & MTRR_LIB_CACHE_FIXED_MTRR_ENABLED) != 0) {
      //
      // Go through the fixed MTRR
      //
      for (Index = 0; Index < MTRR_NUMBER_OF_FIXED_MTRR; Index++) {
         if (Address >= mMtrrLibFixedMtrrTable[Index].BaseAddress &&
             Address  < (
                          mMtrrLibFixedMtrrTable[Index].BaseAddress +
                          (mMtrrLibFixedMtrrTable[Index].Length * 8)
                        )
            ) {
           SubIndex =
             ((UINTN)Address - mMtrrLibFixedMtrrTable[Index].BaseAddress) /
               mMtrrLibFixedMtrrTable[Index].Length;
           TempQword = AsmReadMsr64 (mMtrrLibFixedMtrrTable[Index].Msr);
           MtrrType =  RShiftU64 (TempQword, SubIndex * 8) & 0xFF;
           return GetMemoryCacheTypeFromMtrrType (MtrrType);
         }
      }
    }
  }
  MtrrLibInitializeMtrrMask(&MtrrValidBitsMask, &MtrrValidAddressMask);
  MtrrGetMemoryAttributeInVariableMtrr(
    MtrrValidBitsMask,
    MtrrValidAddressMask,
    VariableMtrr
    );

  //
  // Go through the variable MTRR
  //
  VariableMtrrCount = GetVariableMtrrCount ();
  ASSERT (VariableMtrrCount <= MTRR_NUMBER_OF_VARIABLE_MTRR);

  for (Index = 0; Index < VariableMtrrCount; Index++) {
    if (VariableMtrr[Index].Valid) {
      if (Address >= VariableMtrr[Index].BaseAddress &&
          Address < VariableMtrr[Index].BaseAddress+VariableMtrr[Index].Length) {
        TempMtrrType = VariableMtrr[Index].Type;
        MtrrType = MtrrPrecedence (MtrrType, TempMtrrType);
      }
    }
  }
  CacheType = GetMemoryCacheTypeFromMtrrType (MtrrType);

  return CacheType;
}


/**
  This function will get the raw value in variable MTRRs

  @param  VariableSettings   A buffer to hold variable MTRRs content.

  @return The VariableSettings input pointer

**/
MTRR_VARIABLE_SETTINGS*
EFIAPI
MtrrGetVariableMtrr (
  OUT MTRR_VARIABLE_SETTINGS         *VariableSettings
  )
{
  UINT32  Index;
  UINT32  VariableMtrrCount;

  if (!IsMtrrSupported ()) {
    return VariableSettings;
  }

  VariableMtrrCount = GetVariableMtrrCount ();
  ASSERT (VariableMtrrCount <= MTRR_NUMBER_OF_VARIABLE_MTRR);

  for (Index = 0; Index < VariableMtrrCount; Index++) {
    VariableSettings->Mtrr[Index].Base =
      AsmReadMsr64 (MTRR_LIB_IA32_VARIABLE_MTRR_BASE + (Index << 1));
    VariableSettings->Mtrr[Index].Mask =
      AsmReadMsr64 (MTRR_LIB_IA32_VARIABLE_MTRR_BASE + (Index << 1) + 1);
  }

  return  VariableSettings;
}


/**
  Worker function setting variable MTRRs

  @param  VariableSettings   A buffer to hold variable MTRRs content.

**/
VOID
MtrrSetVariableMtrrWorker (
  IN MTRR_VARIABLE_SETTINGS         *VariableSettings
  )
{
  UINT32  Index;
  UINT32  VariableMtrrCount;

  VariableMtrrCount = GetVariableMtrrCount ();
  ASSERT (VariableMtrrCount <= MTRR_NUMBER_OF_VARIABLE_MTRR);

  for (Index = 0; Index < VariableMtrrCount; Index++) {
    AsmWriteMsr64 (
      MTRR_LIB_IA32_VARIABLE_MTRR_BASE + (Index << 1),
      VariableSettings->Mtrr[Index].Base
      );
    AsmWriteMsr64 (
      MTRR_LIB_IA32_VARIABLE_MTRR_BASE + (Index << 1) + 1,
      VariableSettings->Mtrr[Index].Mask
      );
  }
}


/**
  This function sets variable MTRRs

  @param  VariableSettings   A buffer to hold variable MTRRs content.

  @return The pointer of VariableSettings

**/
MTRR_VARIABLE_SETTINGS*
EFIAPI
MtrrSetVariableMtrr (
  IN MTRR_VARIABLE_SETTINGS         *VariableSettings
  )
{
  MTRR_CONTEXT  MtrrContext;

  if (!IsMtrrSupported ()) {
    return VariableSettings;
  }

  PreMtrrChange (&MtrrContext);
  MtrrSetVariableMtrrWorker (VariableSettings);
  PostMtrrChange (&MtrrContext);
  return  VariableSettings;
}


/**
  This function gets the content in fixed MTRRs

  @param  FixedSettings  A buffer to hold fixed Mtrrs content.

  @retval The pointer of FixedSettings

**/
MTRR_FIXED_SETTINGS*
EFIAPI
MtrrGetFixedMtrr (
  OUT MTRR_FIXED_SETTINGS         *FixedSettings
  )
{
  UINT32  Index;

  if (!IsMtrrSupported ()) {
    return FixedSettings;
  }

  for (Index = 0; Index < MTRR_NUMBER_OF_FIXED_MTRR; Index++) {
      FixedSettings->Mtrr[Index] =
        AsmReadMsr64 (mMtrrLibFixedMtrrTable[Index].Msr);
  };

  return FixedSettings;
}

/**
  Worker function setting fixed MTRRs

  @param  FixedSettings  A buffer to hold fixed Mtrrs content.

**/
VOID
MtrrSetFixedMtrrWorker (
  IN MTRR_FIXED_SETTINGS          *FixedSettings
  )
{
  UINT32  Index;

  for (Index = 0; Index < MTRR_NUMBER_OF_FIXED_MTRR; Index++) {
     AsmWriteMsr64 (
       mMtrrLibFixedMtrrTable[Index].Msr,
       FixedSettings->Mtrr[Index]
       );
  }
}


/**
  This function sets fixed MTRRs

  @param  FixedSettings  A buffer to hold fixed Mtrrs content.

  @retval The pointer of FixedSettings

**/
MTRR_FIXED_SETTINGS*
EFIAPI
MtrrSetFixedMtrr (
  IN MTRR_FIXED_SETTINGS          *FixedSettings
  )
{
  MTRR_CONTEXT  MtrrContext;

  if (!IsMtrrSupported ()) {
    return FixedSettings;
  }

  PreMtrrChange (&MtrrContext);
  MtrrSetFixedMtrrWorker (FixedSettings);
  PostMtrrChange (&MtrrContext);

  return FixedSettings;
}


/**
  This function gets the content in all MTRRs (variable and fixed)

  @param  MtrrSetting  A buffer to hold all Mtrrs content.

  @retval the pointer of MtrrSetting

**/
MTRR_SETTINGS *
EFIAPI
MtrrGetAllMtrrs (
  OUT MTRR_SETTINGS                *MtrrSetting
  )
{
  if (!IsMtrrSupported ()) {
    return MtrrSetting;
  }

  //
  // Get fixed MTRRs
  //
  MtrrGetFixedMtrr (&MtrrSetting->Fixed);

  //
  // Get variable MTRRs
  //
  MtrrGetVariableMtrr (&MtrrSetting->Variables);

  //
  // Get MTRR_DEF_TYPE value
  //
  MtrrSetting->MtrrDefType = AsmReadMsr64 (MTRR_LIB_IA32_MTRR_DEF_TYPE);

  return MtrrSetting;
}


/**
  This function sets all MTRRs (variable and fixed)

  @param  MtrrSetting  A buffer holding all MTRRs content.

  @retval The pointer of MtrrSetting

**/
MTRR_SETTINGS *
EFIAPI
MtrrSetAllMtrrs (
  IN MTRR_SETTINGS                *MtrrSetting
  )
{
  MTRR_CONTEXT  MtrrContext;

  if (!IsMtrrSupported ()) {
    return MtrrSetting;
  }

  PreMtrrChange (&MtrrContext);

  //
  // Set fixed MTRRs
  //
  MtrrSetFixedMtrrWorker (&MtrrSetting->Fixed);

  //
  // Set variable MTRRs
  //
  MtrrSetVariableMtrrWorker (&MtrrSetting->Variables);

  //
  // Set MTRR_DEF_TYPE value
  //
  AsmWriteMsr64 (MTRR_LIB_IA32_MTRR_DEF_TYPE, MtrrSetting->MtrrDefType);

  PostMtrrChangeEnableCache (&MtrrContext);

  return MtrrSetting;
}

/**
  This function prints all MTRRs for debugging.
**/
VOID
EFIAPI
MtrrDebugPrintAllMtrrs (
  VOID
  )
{
  DEBUG_CODE (
    MTRR_SETTINGS  MtrrSettings;
    UINTN          Index;
    UINTN          Index1;
    UINTN          VariableMtrrCount;
    UINT64         Base;
    UINT64         Limit;
    UINT64         MtrrBase;
    UINT64         MtrrLimit;
    UINT64         RangeBase;
    UINT64         RangeLimit;
    UINT64         NoRangeBase;
    UINT64         NoRangeLimit;
    UINT32         RegEax;
    UINTN          MemoryType;
    UINTN          PreviousMemoryType;
    BOOLEAN        Found;

    if (!IsMtrrSupported ()) {
      return;
    }

    DEBUG((DEBUG_CACHE, "MTRR Settings\n"));
    DEBUG((DEBUG_CACHE, "=============\n"));
    
    MtrrGetAllMtrrs (&MtrrSettings);
    DEBUG((DEBUG_CACHE, "MTRR Default Type: %016lx\n", MtrrSettings.MtrrDefType));
    for (Index = 0; Index < MTRR_NUMBER_OF_FIXED_MTRR; Index++) {
      DEBUG((DEBUG_CACHE, "Fixed MTRR[%02d]   : %016lx\n", Index, MtrrSettings.Fixed.Mtrr[Index]));
    }

    VariableMtrrCount = GetVariableMtrrCount ();
    for (Index = 0; Index < VariableMtrrCount; Index++) {
      DEBUG((DEBUG_CACHE, "Variable MTRR[%02d]: Base=%016lx Mask=%016lx\n",
        Index,
        MtrrSettings.Variables.Mtrr[Index].Base,
        MtrrSettings.Variables.Mtrr[Index].Mask
        ));
    }
    DEBUG((DEBUG_CACHE, "\n"));
    DEBUG((DEBUG_CACHE, "MTRR Ranges\n"));
    DEBUG((DEBUG_CACHE, "====================================\n"));

    Base = 0;
    PreviousMemoryType = MTRR_CACHE_INVALID_TYPE;
    for (Index = 0; Index < MTRR_NUMBER_OF_FIXED_MTRR; Index++) {
      Base = mMtrrLibFixedMtrrTable[Index].BaseAddress;
      for (Index1 = 0; Index1 < 8; Index1++) {
      MemoryType = (UINTN)(RShiftU64 (MtrrSettings.Fixed.Mtrr[Index], Index1 * 8) & 0xff);
        if (MemoryType > CacheWriteBack) {
          MemoryType = MTRR_CACHE_INVALID_TYPE;
        }            
        if (MemoryType != PreviousMemoryType) {
          if (PreviousMemoryType != MTRR_CACHE_INVALID_TYPE) {
            DEBUG((DEBUG_CACHE, "%016lx\n", Base - 1));
          }
          PreviousMemoryType = MemoryType;
          DEBUG((DEBUG_CACHE, "%a:%016lx-", mMtrrMemoryCacheTypeShortName[MemoryType], Base));
        }
        Base += mMtrrLibFixedMtrrTable[Index].Length;
      }
    }
    DEBUG((DEBUG_CACHE, "%016lx\n", Base - 1));

    VariableMtrrCount = GetVariableMtrrCount ();

    Limit        = BIT36 - 1;
    AsmCpuid (0x80000000, &RegEax, NULL, NULL, NULL);
    if (RegEax >= 0x80000008) {
      AsmCpuid (0x80000008, &RegEax, NULL, NULL, NULL);
      Limit = LShiftU64 (1, RegEax & 0xff) - 1;
    }
    Base = BASE_1MB;
    PreviousMemoryType = MTRR_CACHE_INVALID_TYPE;
    do {
      MemoryType = MtrrGetMemoryAttribute (Base);
      if (MemoryType > CacheWriteBack) {
        MemoryType = MTRR_CACHE_INVALID_TYPE;
      }

      if (MemoryType != PreviousMemoryType) {
        if (PreviousMemoryType != MTRR_CACHE_INVALID_TYPE) {
          DEBUG((DEBUG_CACHE, "%016lx\n", Base - 1));
        }
        PreviousMemoryType = MemoryType;
        DEBUG((DEBUG_CACHE, "%a:%016lx-", mMtrrMemoryCacheTypeShortName[MemoryType], Base));
      }
      
      RangeBase    = BASE_1MB;        
      NoRangeBase  = BASE_1MB;
      RangeLimit   = Limit;
      NoRangeLimit = Limit;
      
      for (Index = 0, Found = FALSE; Index < VariableMtrrCount; Index++) {
        if ((MtrrSettings.Variables.Mtrr[Index].Mask & BIT11) == 0) {
          //
          // If mask is not valid, then do not display range
          //
          continue;
        }
        MtrrBase  = (MtrrSettings.Variables.Mtrr[Index].Base & (~(SIZE_4KB - 1)));
        MtrrLimit = MtrrBase + ((~(MtrrSettings.Variables.Mtrr[Index].Mask & (~(SIZE_4KB - 1)))) & Limit);

        if (Base >= MtrrBase && Base < MtrrLimit) {
          Found = TRUE;
        }
        
        if (Base >= MtrrBase && MtrrBase > RangeBase) {
          RangeBase = MtrrBase;
        }
        if (Base > MtrrLimit && MtrrLimit > RangeBase) {
          RangeBase = MtrrLimit + 1;
        }
        if (Base < MtrrBase && MtrrBase < RangeLimit) {
          RangeLimit = MtrrBase - 1;
        }
        if (Base < MtrrLimit && MtrrLimit <= RangeLimit) {
          RangeLimit = MtrrLimit;
        }
        
        if (Base > MtrrLimit && NoRangeBase < MtrrLimit) {
          NoRangeBase = MtrrLimit + 1;
        }
        if (Base < MtrrBase && NoRangeLimit > MtrrBase) {
          NoRangeLimit = MtrrBase - 1;
        }
      }
      
      if (Found) {
        Base = RangeLimit + 1;
      } else {
        Base = NoRangeLimit + 1;
      }
    } while (Base < Limit);
    DEBUG((DEBUG_CACHE, "%016lx\n\n", Base - 1));
  );
}

/**
  Checks if MTRR is supported.

  @retval TRUE  MTRR is supported.
  @retval FALSE MTRR is not supported.

**/
BOOLEAN
EFIAPI
IsMtrrSupported (
  VOID
  )
{
  UINT32  RegEdx;
  UINT64  MtrrCap;

  //
  // Check CPUID(1).EDX[12] for MTRR capability
  //
  AsmCpuid (1, NULL, NULL, NULL, &RegEdx);
  if (BitFieldRead32 (RegEdx, 12, 12) == 0) {
    return FALSE;
  }

  //
  // Check IA32_MTRRCAP.[0..7] for number of variable MTRRs and IA32_MTRRCAP[8] for
  // fixed MTRRs existence. If number of variable MTRRs is zero, or fixed MTRRs do not
  // exist, return false.
  //
  MtrrCap = AsmReadMsr64 (MTRR_LIB_IA32_MTRR_CAP);
  if  ((BitFieldRead64 (MtrrCap, 0, 7) == 0) || (BitFieldRead64 (MtrrCap, 8, 8) == 0)) {
    return FALSE;
  }

  return TRUE;
}
