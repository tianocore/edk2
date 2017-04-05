/** @file
  MTRR setting library

  @par Note: 
    Most of services in this library instance are suggested to be invoked by BSP only,
    except for MtrrSetAllMtrrs() which is used to sync BSP's MTRR setting to APs.

  Copyright (c) 2008 - 2017, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>

#include <Register/Cpuid.h>
#include <Register/Msr.h>

#include <Library/MtrrLib.h>
#include <Library/BaseLib.h>
#include <Library/CpuLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

#define OR_SEED      0x0101010101010101ull
#define CLEAR_SEED   0xFFFFFFFFFFFFFFFFull

#define MTRR_LIB_ASSERT_ALIGNED(B, L) ASSERT ((B & ~(L - 1)) == B);
//
// Context to save and restore when MTRRs are programmed
//
typedef struct {
  UINTN    Cr4;
  BOOLEAN  InterruptState;
} MTRR_CONTEXT;

typedef struct {
  UINT64                 BaseAddress;
  UINT64                 Length;
  MTRR_MEMORY_CACHE_TYPE Type;
} MEMORY_RANGE;

//
// This table defines the offset, base and length of the fixed MTRRs
//
CONST FIXED_MTRR  mMtrrLibFixedMtrrTable[] = {
  {
    MSR_IA32_MTRR_FIX64K_00000,
    0,
    SIZE_64KB
  },
  {
    MSR_IA32_MTRR_FIX16K_80000,
    0x80000,
    SIZE_16KB
  },
  {
    MSR_IA32_MTRR_FIX16K_A0000,
    0xA0000,
    SIZE_16KB
  },
  {
    MSR_IA32_MTRR_FIX4K_C0000,
    0xC0000,
    SIZE_4KB
  },
  {
    MSR_IA32_MTRR_FIX4K_C8000,
    0xC8000,
    SIZE_4KB
  },
  {
    MSR_IA32_MTRR_FIX4K_D0000,
    0xD0000,
    SIZE_4KB
  },
  {
    MSR_IA32_MTRR_FIX4K_D8000,
    0xD8000,
    SIZE_4KB
  },
  {
    MSR_IA32_MTRR_FIX4K_E0000,
    0xE0000,
    SIZE_4KB
  },
  {
    MSR_IA32_MTRR_FIX4K_E8000,
    0xE8000,
    SIZE_4KB
  },
  {
    MSR_IA32_MTRR_FIX4K_F0000,
    0xF0000,
    SIZE_4KB
  },
  {
    MSR_IA32_MTRR_FIX4K_F8000,
    0xF8000,
    SIZE_4KB
  }
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
  Worker function returns the variable MTRR count for the CPU.

  @return Variable MTRR count

**/
UINT32
GetVariableMtrrCountWorker (
  VOID
  )
{
  MSR_IA32_MTRRCAP_REGISTER MtrrCap;

  MtrrCap.Uint64 = AsmReadMsr64 (MSR_IA32_MTRRCAP);
  ASSERT (MtrrCap.Bits.VCNT <= MTRR_NUMBER_OF_VARIABLE_MTRR);
  return MtrrCap.Bits.VCNT;
}

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
  if (!IsMtrrSupported ()) {
    return 0;
  }
  return GetVariableMtrrCountWorker ();
}

/**
  Worker function returns the firmware usable variable MTRR count for the CPU.

  @return Firmware usable variable MTRR count

**/
UINT32
GetFirmwareVariableMtrrCountWorker (
  VOID
  )
{
  UINT32  VariableMtrrCount;
  UINT32  ReservedMtrrNumber;

  VariableMtrrCount = GetVariableMtrrCountWorker ();
  ReservedMtrrNumber = PcdGet32 (PcdCpuNumberOfReservedVariableMtrrs);
  if (VariableMtrrCount < ReservedMtrrNumber) {
    return 0;
  }

  return VariableMtrrCount - ReservedMtrrNumber;
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
  if (!IsMtrrSupported ()) {
    return 0;
  }
  return GetFirmwareVariableMtrrCountWorker ();
}

/**
  Worker function returns the default MTRR cache type for the system.

  If MtrrSetting is not NULL, returns the default MTRR cache type from input
  MTRR settings buffer.
  If MtrrSetting is NULL, returns the default MTRR cache type from MSR.

  @param[in]  MtrrSetting    A buffer holding all MTRRs content.

  @return  The default MTRR cache type.

**/
MTRR_MEMORY_CACHE_TYPE
MtrrGetDefaultMemoryTypeWorker (
  IN MTRR_SETTINGS      *MtrrSetting
  )
{
  MSR_IA32_MTRR_DEF_TYPE_REGISTER DefType;

  if (MtrrSetting == NULL) {
    DefType.Uint64 = AsmReadMsr64 (MSR_IA32_MTRR_DEF_TYPE);
  } else {
    DefType.Uint64 = MtrrSetting->MtrrDefType;
  }

  return (MTRR_MEMORY_CACHE_TYPE) DefType.Bits.Type;
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
  return MtrrGetDefaultMemoryTypeWorker (NULL);
}

/**
  Preparation before programming MTRR.

  This function will do some preparation for programming MTRRs:
  disable cache, invalid cache and disable MTRR caching functionality

  @param[out] MtrrContext  Pointer to context to save

**/
VOID
MtrrLibPreMtrrChange (
  OUT MTRR_CONTEXT  *MtrrContext
  )
{
  MSR_IA32_MTRR_DEF_TYPE_REGISTER  DefType;
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
  // Disable MTRRs
  //
  DefType.Uint64 = AsmReadMsr64 (MSR_IA32_MTRR_DEF_TYPE);
  DefType.Bits.E = 0;
  AsmWriteMsr64 (MSR_IA32_MTRR_DEF_TYPE, DefType.Uint64);
}

/**
  Cleaning up after programming MTRRs.

  This function will do some clean up after programming MTRRs:
  Flush all TLBs,  re-enable caching, restore CR4.

  @param[in] MtrrContext  Pointer to context to restore

**/
VOID
MtrrLibPostMtrrChangeEnableCache (
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
MtrrLibPostMtrrChange (
  IN MTRR_CONTEXT  *MtrrContext
  )
{
  MSR_IA32_MTRR_DEF_TYPE_REGISTER  DefType;
  //
  // Enable Cache MTRR
  //
  DefType.Uint64 = AsmReadMsr64 (MSR_IA32_MTRR_DEF_TYPE);
  DefType.Bits.E = 1;
  DefType.Bits.FE = 1;
  AsmWriteMsr64 (MSR_IA32_MTRR_DEF_TYPE, DefType.Uint64);

  MtrrLibPostMtrrChangeEnableCache (MtrrContext);
}

/**
  Worker function gets the content in fixed MTRRs

  @param[out]  FixedSettings  A buffer to hold fixed MTRRs content.

  @retval The pointer of FixedSettings

**/
MTRR_FIXED_SETTINGS*
MtrrGetFixedMtrrWorker (
  OUT MTRR_FIXED_SETTINGS         *FixedSettings
  )
{
  UINT32  Index;

  for (Index = 0; Index < MTRR_NUMBER_OF_FIXED_MTRR; Index++) {
      FixedSettings->Mtrr[Index] =
        AsmReadMsr64 (mMtrrLibFixedMtrrTable[Index].Msr);
  }

  return FixedSettings;
}


/**
  This function gets the content in fixed MTRRs

  @param[out]  FixedSettings  A buffer to hold fixed MTRRs content.

  @retval The pointer of FixedSettings

**/
MTRR_FIXED_SETTINGS*
EFIAPI
MtrrGetFixedMtrr (
  OUT MTRR_FIXED_SETTINGS         *FixedSettings
  )
{
  if (!IsMtrrSupported ()) {
    return FixedSettings;
  }

  return MtrrGetFixedMtrrWorker (FixedSettings);
}


/**
  Worker function will get the raw value in variable MTRRs

  If MtrrSetting is not NULL, gets the variable MTRRs raw value from input
  MTRR settings buffer.
  If MtrrSetting is NULL, gets the variable MTRRs raw value from MTRRs.

  @param[in]  MtrrSetting        A buffer holding all MTRRs content.
  @param[in]  VariableMtrrCount  Number of variable MTRRs.
  @param[out] VariableSettings   A buffer to hold variable MTRRs content.

  @return The VariableSettings input pointer

**/
MTRR_VARIABLE_SETTINGS*
MtrrGetVariableMtrrWorker (
  IN  MTRR_SETTINGS           *MtrrSetting,
  IN  UINT32                  VariableMtrrCount,
  OUT MTRR_VARIABLE_SETTINGS  *VariableSettings
  )
{
  UINT32  Index;

  ASSERT (VariableMtrrCount <= MTRR_NUMBER_OF_VARIABLE_MTRR);

  for (Index = 0; Index < VariableMtrrCount; Index++) {
    if (MtrrSetting == NULL) {
      VariableSettings->Mtrr[Index].Base =
        AsmReadMsr64 (MSR_IA32_MTRR_PHYSBASE0 + (Index << 1));
      VariableSettings->Mtrr[Index].Mask =
        AsmReadMsr64 (MSR_IA32_MTRR_PHYSMASK0 + (Index << 1));
    } else {
      VariableSettings->Mtrr[Index].Base = MtrrSetting->Variables.Mtrr[Index].Base;
      VariableSettings->Mtrr[Index].Mask = MtrrSetting->Variables.Mtrr[Index].Mask;
    }
  }

  return  VariableSettings;
}

/**
  This function will get the raw value in variable MTRRs

  @param[out]  VariableSettings   A buffer to hold variable MTRRs content.

  @return The VariableSettings input pointer

**/
MTRR_VARIABLE_SETTINGS*
EFIAPI
MtrrGetVariableMtrr (
  OUT MTRR_VARIABLE_SETTINGS         *VariableSettings
  )
{
  if (!IsMtrrSupported ()) {
    return VariableSettings;
  }

  return MtrrGetVariableMtrrWorker (
           NULL,
           GetVariableMtrrCountWorker (),
           VariableSettings
           );
}

/**
  Programs fixed MTRRs registers.

  @param[in]      Type             The memory type to set.
  @param[in, out] Base             The base address of memory range.
  @param[in, out] Length           The length of memory range.
  @param[in, out] LastMsrNum       On input, the last index of the fixed MTRR MSR to program.
                                   On return, the current index of the fixed MTRR MSR to program.
  @param[out]     ReturnClearMask  The bits to clear in the fixed MTRR MSR.
  @param[out]     ReturnOrMask     The bits to set in the fixed MTRR MSR.

  @retval RETURN_SUCCESS      The cache type was updated successfully
  @retval RETURN_UNSUPPORTED  The requested range or cache type was invalid
                              for the fixed MTRRs.

**/
RETURN_STATUS
MtrrLibProgramFixedMtrr (
  IN     MTRR_MEMORY_CACHE_TYPE  Type,
  IN OUT UINT64                  *Base,
  IN OUT UINT64                  *Length,
  IN OUT UINT32                  *LastMsrNum,
  OUT    UINT64                  *ReturnClearMask,
  OUT    UINT64                  *ReturnOrMask
  )
{
  UINT32  MsrNum;
  UINT32  LeftByteShift;
  UINT32  RightByteShift;
  UINT64  OrMask;
  UINT64  ClearMask;
  UINT64  SubLength;

  //
  // Find the fixed MTRR index to be programmed
  //
  for (MsrNum = *LastMsrNum + 1; MsrNum < MTRR_NUMBER_OF_FIXED_MTRR; MsrNum++) {
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
  // Find the begin offset in fixed MTRR and calculate byte offset of left shift
  //
  LeftByteShift = ((UINT32)*Base - mMtrrLibFixedMtrrTable[MsrNum].BaseAddress)
               / mMtrrLibFixedMtrrTable[MsrNum].Length;

  if (LeftByteShift >= 8) {
    return RETURN_UNSUPPORTED;
  }

  //
  // Find the end offset in fixed MTRR and calculate byte offset of right shift
  //
  SubLength = mMtrrLibFixedMtrrTable[MsrNum].Length * (8 - LeftByteShift);
  if (*Length >= SubLength) {
    RightByteShift = 0;
  } else {
    RightByteShift = 8 - LeftByteShift -
                (UINT32)(*Length) / mMtrrLibFixedMtrrTable[MsrNum].Length;
    if ((LeftByteShift >= 8) ||
        (((UINT32)(*Length) % mMtrrLibFixedMtrrTable[MsrNum].Length) != 0)
        ) {
      return RETURN_UNSUPPORTED;
    }
    //
    // Update SubLength by actual length
    //
    SubLength = *Length;
  }

  ClearMask = CLEAR_SEED;
  OrMask    = MultU64x32 (OR_SEED, (UINT32) Type);

  if (LeftByteShift != 0) {
    //
    // Clear the low bits by LeftByteShift
    //
    ClearMask &= LShiftU64 (ClearMask, LeftByteShift * 8);
    OrMask    &= LShiftU64 (OrMask, LeftByteShift * 8);
  }

  if (RightByteShift != 0) {
    //
    // Clear the high bits by RightByteShift
    //
    ClearMask &= RShiftU64 (ClearMask, RightByteShift * 8);
    OrMask    &= RShiftU64 (OrMask, RightByteShift * 8);
  }

  *Length -= SubLength;
  *Base   += SubLength;

  *LastMsrNum      = MsrNum;
  *ReturnClearMask = ClearMask;
  *ReturnOrMask    = OrMask;

  return RETURN_SUCCESS;
}


/**
  Worker function gets the attribute of variable MTRRs.

  This function shadows the content of variable MTRRs into an
  internal array: VariableMtrr.

  @param[in]   VariableSettings      The variable MTRR values to shadow
  @param[in]   VariableMtrrCount     The number of variable MTRRs
  @param[in]   MtrrValidBitsMask     The mask for the valid bit of the MTRR
  @param[in]   MtrrValidAddressMask  The valid address mask for MTRR
  @param[out]  VariableMtrr          The array to shadow variable MTRRs content

  @return      Number of MTRRs which has been used.

**/
UINT32
MtrrGetMemoryAttributeInVariableMtrrWorker (
  IN  MTRR_VARIABLE_SETTINGS  *VariableSettings,
  IN  UINTN                   VariableMtrrCount,
  IN  UINT64                  MtrrValidBitsMask,
  IN  UINT64                  MtrrValidAddressMask,
  OUT VARIABLE_MTRR           *VariableMtrr
  )
{
  UINTN   Index;
  UINT32  UsedMtrr;

  ZeroMem (VariableMtrr, sizeof (VARIABLE_MTRR) * MTRR_NUMBER_OF_VARIABLE_MTRR);
  for (Index = 0, UsedMtrr = 0; Index < VariableMtrrCount; Index++) {
    if (((MSR_IA32_MTRR_PHYSMASK_REGISTER *) &VariableSettings->Mtrr[Index].Mask)->Bits.V != 0) {
      VariableMtrr[Index].Msr         = (UINT32)Index;
      VariableMtrr[Index].BaseAddress = (VariableSettings->Mtrr[Index].Base & MtrrValidAddressMask);
      VariableMtrr[Index].Length      = ((~(VariableSettings->Mtrr[Index].Mask & MtrrValidAddressMask)) & MtrrValidBitsMask) + 1;
      VariableMtrr[Index].Type        = (VariableSettings->Mtrr[Index].Base & 0x0ff);
      VariableMtrr[Index].Valid       = TRUE;
      VariableMtrr[Index].Used        = TRUE;
      UsedMtrr++;
    }
  }
  return UsedMtrr;
}


/**
  Gets the attribute of variable MTRRs.

  This function shadows the content of variable MTRRs into an
  internal array: VariableMtrr.

  @param[in]   MtrrValidBitsMask     The mask for the valid bit of the MTRR
  @param[in]   MtrrValidAddressMask  The valid address mask for MTRR
  @param[out]  VariableMtrr          The array to shadow variable MTRRs content

  @return                       The return value of this parameter indicates the
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
  MTRR_VARIABLE_SETTINGS  VariableSettings;

  if (!IsMtrrSupported ()) {
    return 0;
  }

  MtrrGetVariableMtrrWorker (
    NULL,
    GetVariableMtrrCountWorker (),
    &VariableSettings
    );

  return MtrrGetMemoryAttributeInVariableMtrrWorker (
           &VariableSettings,
           GetFirmwareVariableMtrrCountWorker (),
           MtrrValidBitsMask,
           MtrrValidAddressMask,
           VariableMtrr
           );
}

/**
  Return the least alignment of address.

  @param Address    The address to return the alignment.
  @param Alignment0 The alignment to return when Address is 0.

  @return The least alignment of the Address.
**/
UINT64
MtrrLibLeastAlignment (
  UINT64    Address,
  UINT64    Alignment0
)
{
  if (Address == 0) {
    return Alignment0;
  }

  return LShiftU64 (1, (UINTN) LowBitSet64 (Address));
}

/**
  Return the number of required variable MTRRs to positively cover the
  specified range.

  @param BaseAddress Base address of the range.
  @param Length      Length of the range.
  @param Alignment0  Alignment of 0.

  @return The number of the required variable MTRRs.
**/
UINT32
MtrrLibGetPositiveMtrrNumber (
  IN UINT64      BaseAddress,
  IN UINT64      Length,
  IN UINT64      Alignment0
)
{
  UINT64         SubLength;
  UINT32         MtrrNumber;
  BOOLEAN        UseLeastAlignment;

  UseLeastAlignment = TRUE;
  SubLength = 0;

  //
  // Calculate the alignment of the base address.
  //
  for (MtrrNumber = 0; Length != 0; MtrrNumber++) {
    if (UseLeastAlignment) {
      SubLength = MtrrLibLeastAlignment (BaseAddress, Alignment0);

      if (SubLength > Length) {
        //
        // Set a flag when remaining length is too small
        //  so that MtrrLibLeastAlignment() is not called in following loops.
        //
        UseLeastAlignment = FALSE;
      }
    }

    if (!UseLeastAlignment) {
      SubLength = GetPowerOfTwo64 (Length);
    }

    BaseAddress += SubLength;
    Length -= SubLength;
  }

  return MtrrNumber;
}

/**
  Return whether the left MTRR type precedes the right MTRR type.

  The MTRR type precedence rules are:
    1. UC precedes any other type
    2. WT precedes WB
  For further details, please refer the IA32 Software Developer's Manual,
  Volume 3, Section "MTRR Precedences".

  @param Left  The left MTRR type.
  @param Right The right MTRR type.

  @retval TRUE  Left precedes Right.
  @retval FALSE Left doesn't precede Right.
**/
BOOLEAN
MtrrLibTypeLeftPrecedeRight (
  IN MTRR_MEMORY_CACHE_TYPE  Left,
  IN MTRR_MEMORY_CACHE_TYPE  Right
)
{
  return (BOOLEAN) (Left == CacheUncacheable || (Left == CacheWriteThrough && Right == CacheWriteBack));
}


/**
  Return whether the type of the specified range can precede the specified type.

  @param Ranges     Memory range array holding memory type settings for all
                    the memory address.
  @param RangeCount Count of memory ranges.
  @param Type       Type to check precedence.
  @param SubBase    Base address of the specified range.
  @param SubLength  Length of the specified range.

  @retval TRUE  The type of the specified range can precede the Type.
  @retval FALSE The type of the specified range cannot precede the Type.
                So the subtraction is not applicable.
**/
BOOLEAN
MtrrLibSubstractable (
  IN CONST MEMORY_RANGE     *Ranges,
  IN UINT32                  RangeCount,
  IN MTRR_MEMORY_CACHE_TYPE  Type,
  IN UINT64                  SubBase,
  IN UINT64                  SubLength
)
{
  UINT32                     Index;
  UINT64                     Length;
  // WT > WB
  // UC > *
  for (Index = 0; Index < RangeCount; Index++) {
    if (Ranges[Index].BaseAddress <= SubBase && SubBase < Ranges[Index].BaseAddress + Ranges[Index].Length) {

      if (Ranges[Index].BaseAddress + Ranges[Index].Length >= SubBase + SubLength) {
        return MtrrLibTypeLeftPrecedeRight (Ranges[Index].Type, Type);

      } else {
        if (!MtrrLibTypeLeftPrecedeRight (Ranges[Index].Type, Type)) {
          return FALSE;
        }

        Length = Ranges[Index].BaseAddress + Ranges[Index].Length - SubBase;
        SubBase += Length;
        SubLength -= Length;
      }
    }
  }

  ASSERT (FALSE);
  return FALSE;
}

/**
  Return the number of required variable MTRRs to cover the specified range.

  The routine considers subtraction in the both side of the range to find out
  the most optimal solution (which uses the least MTRRs).

  @param Ranges            Array holding memory type settings of all memory
                           address.
  @param RangeCount        Count of memory ranges.
  @param VariableMtrr      Array holding allocated variable MTRRs.
  @param VariableMtrrCount Count of allocated variable MTRRs.
  @param BaseAddress       Base address of the specified range.
  @param Length            Length of the specified range.
  @param Type              MTRR type of the specified range.
  @param Alignment0        Alignment of 0.
  @param SubLeft           Return the count of left subtraction.
  @param SubRight          Return the count of right subtraction.

  @return Number of required variable MTRRs.
**/
UINT32
MtrrLibGetMtrrNumber (
  IN CONST MEMORY_RANGE    *Ranges,
  IN UINT32                 RangeCount,
  IN CONST VARIABLE_MTRR    *VariableMtrr,
  IN UINT32                 VariableMtrrCount,
  IN UINT64                 BaseAddress,
  IN UINT64                 Length,
  IN MTRR_MEMORY_CACHE_TYPE Type,
  IN UINT64                 Alignment0,
  OUT UINT32                *SubLeft, // subtractive from BaseAddress to get more aligned address, to save MTRR
  OUT UINT32                *SubRight // subtractive from BaseAddress + Length, to save MTRR
  )
{
  UINT64  Alignment;
  UINT32  LeastLeftMtrrNumber;
  UINT32  MiddleMtrrNumber;
  UINT32  LeastRightMtrrNumber;
  UINT32  CurrentMtrrNumber;
  UINT32  SubtractiveCount;
  UINT32  SubtractiveMtrrNumber;
  UINT32  LeastSubtractiveMtrrNumber;
  UINT64  SubtractiveBaseAddress;
  UINT64  SubtractiveLength;
  UINT64  BaseAlignment;
  UINT32  Index;

  *SubLeft = 0;
  *SubRight = 0;
  LeastSubtractiveMtrrNumber = 0;
  BaseAlignment = 0;

  //
  // Get the optimal left subtraction solution.
  //
  if (BaseAddress != 0) {
    SubtractiveBaseAddress = 0;
    SubtractiveLength      = 0;
    //
    // Get the MTRR number needed without left subtraction.
    //
    LeastLeftMtrrNumber = MtrrLibGetPositiveMtrrNumber (BaseAddress, Length, Alignment0);

    //
    // Left subtraction bit by bit, to find the optimal left subtraction solution.
    //
    for (SubtractiveMtrrNumber = 0, SubtractiveCount = 1; BaseAddress != 0; SubtractiveCount++) {
      Alignment = MtrrLibLeastAlignment (BaseAddress, Alignment0);

      //
      // Check whether the memory type of [BaseAddress - Alignment, BaseAddress) can override Type.
      // IA32 Manual defines the following override rules:
      //   WT > WB
      //   UC > * (any)
      //
      if (!MtrrLibSubstractable (Ranges, RangeCount, Type, BaseAddress - Alignment, Alignment)) {
        break;
      }

      for (Index = 0; Index < VariableMtrrCount; Index++) {
        if ((VariableMtrr[Index].BaseAddress == BaseAddress - Alignment) &&
            (VariableMtrr[Index].Length == Alignment)) {
          break;
        }
      }
      if (Index == VariableMtrrCount) {
        //
        // Increment SubtractiveMtrrNumber when [BaseAddress - Alignment, BaseAddress) is not be planed as a MTRR
        //
        SubtractiveMtrrNumber++;
      }

      BaseAddress -= Alignment;
      Length += Alignment;

      CurrentMtrrNumber = SubtractiveMtrrNumber + MtrrLibGetPositiveMtrrNumber (BaseAddress, Length, Alignment0);
      if (CurrentMtrrNumber <= LeastLeftMtrrNumber) {
        LeastLeftMtrrNumber = CurrentMtrrNumber;
        LeastSubtractiveMtrrNumber = SubtractiveMtrrNumber;
        *SubLeft = SubtractiveCount;
        SubtractiveBaseAddress = BaseAddress;
        SubtractiveLength = Length;
      }
    }

    //
    // If left subtraction is better, subtract BaseAddress to left, and enlarge Length
    //
    if (*SubLeft != 0) {
      BaseAddress = SubtractiveBaseAddress;
      Length = SubtractiveLength;
    }
  }

  //
  // Increment BaseAddress greedily until (BaseAddress + Alignment) exceeds (BaseAddress + Length)
  //
  MiddleMtrrNumber = 0;
  while (Length != 0) {
    BaseAlignment = MtrrLibLeastAlignment (BaseAddress, Alignment0);
    if (BaseAlignment > Length) {
      break;
    }
    BaseAddress += BaseAlignment;
    Length -= BaseAlignment;
    MiddleMtrrNumber++;
  }


  if (Length == 0) {
    return LeastSubtractiveMtrrNumber + MiddleMtrrNumber;
  }


  //
  // Get the optimal right subtraction solution.
  //

  //
  // Get the MTRR number needed without right subtraction.
  //
  LeastRightMtrrNumber = MtrrLibGetPositiveMtrrNumber (BaseAddress, Length, Alignment0);

  for (SubtractiveCount = 1; Length < BaseAlignment; SubtractiveCount++) {
    Alignment = MtrrLibLeastAlignment (BaseAddress + Length, Alignment0);
    if (!MtrrLibSubstractable (Ranges, RangeCount, Type, BaseAddress + Length, Alignment)) {
      break;
    }

    Length += Alignment;

    //
    // SubtractiveCount = Number of MTRRs used for subtraction
    //
    CurrentMtrrNumber = SubtractiveCount + MtrrLibGetPositiveMtrrNumber (BaseAddress, Length, Alignment0);
    if (CurrentMtrrNumber <= LeastRightMtrrNumber) {
      LeastRightMtrrNumber = CurrentMtrrNumber;
      *SubRight = SubtractiveCount;
      SubtractiveLength = Length;
    }
  }

  return LeastSubtractiveMtrrNumber + MiddleMtrrNumber + LeastRightMtrrNumber;
}

/**
  Initializes the valid bits mask and valid address mask for MTRRs.

  This function initializes the valid bits mask and valid address mask for MTRRs.

  @param[out]  MtrrValidBitsMask     The mask for the valid bit of the MTRR
  @param[out]  MtrrValidAddressMask  The valid address mask for the MTRR

**/
VOID
MtrrLibInitializeMtrrMask (
  OUT UINT64 *MtrrValidBitsMask,
  OUT UINT64 *MtrrValidAddressMask
  )
{
  UINT32                          MaxExtendedFunction;
  CPUID_VIR_PHY_ADDRESS_SIZE_EAX  VirPhyAddressSize;


  AsmCpuid (CPUID_EXTENDED_FUNCTION, &MaxExtendedFunction, NULL, NULL, NULL);

  if (MaxExtendedFunction >= CPUID_VIR_PHY_ADDRESS_SIZE) {
    AsmCpuid (CPUID_VIR_PHY_ADDRESS_SIZE, &VirPhyAddressSize.Uint32, NULL, NULL, NULL);
  } else {
    VirPhyAddressSize.Bits.PhysicalAddressBits = 36;
  }

  *MtrrValidBitsMask = LShiftU64 (1, VirPhyAddressSize.Bits.PhysicalAddressBits) - 1;
  *MtrrValidAddressMask = *MtrrValidBitsMask & 0xfffffffffffff000ULL;
}


/**
  Determines the real attribute of a memory range.

  This function is to arbitrate the real attribute of the memory when
  there are 2 MTRRs covers the same memory range. For further details,
  please refer the IA32 Software Developer's Manual, Volume 3,
  Section "MTRR Precedences".

  @param[in]  MtrrType1    The first kind of Memory type
  @param[in]  MtrrType2    The second kind of memory type

**/
MTRR_MEMORY_CACHE_TYPE
MtrrLibPrecedence (
  IN MTRR_MEMORY_CACHE_TYPE    MtrrType1,
  IN MTRR_MEMORY_CACHE_TYPE    MtrrType2
  )
{
  if (MtrrType1 == MtrrType2) {
    return MtrrType1;
  }

  ASSERT (
    MtrrLibTypeLeftPrecedeRight (MtrrType1, MtrrType2) ||
    MtrrLibTypeLeftPrecedeRight (MtrrType2, MtrrType1)
  );

  if (MtrrLibTypeLeftPrecedeRight (MtrrType1, MtrrType2)) {
    return MtrrType1;
  } else {
    return MtrrType2;
  }
}

/**
  Worker function will get the memory cache type of the specific address.

  If MtrrSetting is not NULL, gets the memory cache type from input
  MTRR settings buffer.
  If MtrrSetting is NULL, gets the memory cache type from MTRRs.

  @param[in]  MtrrSetting        A buffer holding all MTRRs content.
  @param[in]  Address            The specific address

  @return Memory cache type of the specific address

**/
MTRR_MEMORY_CACHE_TYPE
MtrrGetMemoryAttributeByAddressWorker (
  IN MTRR_SETTINGS      *MtrrSetting,
  IN PHYSICAL_ADDRESS   Address
  )
{
  MSR_IA32_MTRR_DEF_TYPE_REGISTER DefType;
  UINT64                          FixedMtrr;
  UINTN                           Index;
  UINTN                           SubIndex;
  MTRR_MEMORY_CACHE_TYPE          MtrrType;
  VARIABLE_MTRR                   VariableMtrr[MTRR_NUMBER_OF_VARIABLE_MTRR];
  UINT64                          MtrrValidBitsMask;
  UINT64                          MtrrValidAddressMask;
  UINT32                          VariableMtrrCount;
  MTRR_VARIABLE_SETTINGS          VariableSettings;

  //
  // Check if MTRR is enabled, if not, return UC as attribute
  //
  if (MtrrSetting == NULL) {
    DefType.Uint64 = AsmReadMsr64 (MSR_IA32_MTRR_DEF_TYPE);
  } else {
    DefType.Uint64 = MtrrSetting->MtrrDefType;
  }

  if (DefType.Bits.E == 0) {
    return CacheUncacheable;
  }

  //
  // If address is less than 1M, then try to go through the fixed MTRR
  //
  if (Address < BASE_1MB) {
    if (DefType.Bits.FE != 0) {
      //
      // Go through the fixed MTRR
      //
      for (Index = 0; Index < MTRR_NUMBER_OF_FIXED_MTRR; Index++) {
        if (Address >= mMtrrLibFixedMtrrTable[Index].BaseAddress &&
            Address < mMtrrLibFixedMtrrTable[Index].BaseAddress +
            (mMtrrLibFixedMtrrTable[Index].Length * 8)) {
          SubIndex =
            ((UINTN) Address - mMtrrLibFixedMtrrTable[Index].BaseAddress) /
            mMtrrLibFixedMtrrTable[Index].Length;
          if (MtrrSetting == NULL) {
            FixedMtrr = AsmReadMsr64 (mMtrrLibFixedMtrrTable[Index].Msr);
          } else {
            FixedMtrr = MtrrSetting->Fixed.Mtrr[Index];
          }
          return (MTRR_MEMORY_CACHE_TYPE) (RShiftU64 (FixedMtrr, SubIndex * 8) & 0xFF);
        }
      }
    }
  }

  VariableMtrrCount = GetVariableMtrrCountWorker ();
  ASSERT (VariableMtrrCount <= MTRR_NUMBER_OF_VARIABLE_MTRR);
  MtrrGetVariableMtrrWorker (MtrrSetting, VariableMtrrCount, &VariableSettings);

  MtrrLibInitializeMtrrMask (&MtrrValidBitsMask, &MtrrValidAddressMask);
  MtrrGetMemoryAttributeInVariableMtrrWorker (
    &VariableSettings,
    VariableMtrrCount,
    MtrrValidBitsMask,
    MtrrValidAddressMask,
    VariableMtrr
  );

  //
  // Go through the variable MTRR
  //
  MtrrType = CacheInvalid;
  for (Index = 0; Index < VariableMtrrCount; Index++) {
    if (VariableMtrr[Index].Valid) {
      if (Address >= VariableMtrr[Index].BaseAddress &&
          Address < VariableMtrr[Index].BaseAddress + VariableMtrr[Index].Length) {
        if (MtrrType == CacheInvalid) {
          MtrrType = (MTRR_MEMORY_CACHE_TYPE) VariableMtrr[Index].Type;
        } else {
          MtrrType = MtrrLibPrecedence (MtrrType, (MTRR_MEMORY_CACHE_TYPE) VariableMtrr[Index].Type);
        }
      }
    }
  }

  //
  // If there is no MTRR which covers the Address, use the default MTRR type.
  //
  if (MtrrType == CacheInvalid) {
    MtrrType = (MTRR_MEMORY_CACHE_TYPE) DefType.Bits.Type;
  }

  return MtrrType;
}


/**
  This function will get the memory cache type of the specific address.

  This function is mainly for debug purpose.

  @param[in]  Address   The specific address

  @return Memory cache type of the specific address

**/
MTRR_MEMORY_CACHE_TYPE
EFIAPI
MtrrGetMemoryAttribute (
  IN PHYSICAL_ADDRESS   Address
  )
{
  if (!IsMtrrSupported ()) {
    return CacheUncacheable;
  }

  return MtrrGetMemoryAttributeByAddressWorker (NULL, Address);
}

/**
  Worker function prints all MTRRs for debugging.

  If MtrrSetting is not NULL, print MTRR settings from input MTRR
  settings buffer.
  If MtrrSetting is NULL, print MTRR settings from MTRRs.

  @param  MtrrSetting    A buffer holding all MTRRs content.
**/
VOID
MtrrDebugPrintAllMtrrsWorker (
  IN MTRR_SETTINGS    *MtrrSetting
  )
{
  DEBUG_CODE (
    MTRR_SETTINGS  LocalMtrrs;
    MTRR_SETTINGS  *Mtrrs;
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

    if (MtrrSetting != NULL) {
      Mtrrs = MtrrSetting;
    } else {
      MtrrGetAllMtrrs (&LocalMtrrs);
      Mtrrs = &LocalMtrrs;
    }

    DEBUG((DEBUG_CACHE, "MTRR Default Type: %016lx\n", Mtrrs->MtrrDefType));
    for (Index = 0; Index < MTRR_NUMBER_OF_FIXED_MTRR; Index++) {
      DEBUG((DEBUG_CACHE, "Fixed MTRR[%02d]   : %016lx\n", Index, Mtrrs->Fixed.Mtrr[Index]));
    }

    VariableMtrrCount = GetVariableMtrrCount ();
    for (Index = 0; Index < VariableMtrrCount; Index++) {
      DEBUG((DEBUG_CACHE, "Variable MTRR[%02d]: Base=%016lx Mask=%016lx\n",
        Index,
        Mtrrs->Variables.Mtrr[Index].Base,
        Mtrrs->Variables.Mtrr[Index].Mask
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
      MemoryType = (UINTN)(RShiftU64 (Mtrrs->Fixed.Mtrr[Index], Index1 * 8) & 0xff);
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
      MemoryType = MtrrGetMemoryAttributeByAddressWorker (Mtrrs, Base);
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
        if ((Mtrrs->Variables.Mtrr[Index].Mask & BIT11) == 0) {
          //
          // If mask is not valid, then do not display range
          //
          continue;
        }
        MtrrBase  = (Mtrrs->Variables.Mtrr[Index].Base & (~(SIZE_4KB - 1)));
        MtrrLimit = MtrrBase + ((~(Mtrrs->Variables.Mtrr[Index].Mask & (~(SIZE_4KB - 1)))) & Limit);

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
  This function prints all MTRRs for debugging.
**/
VOID
EFIAPI
MtrrDebugPrintAllMtrrs (
  VOID
  )
{
  MtrrDebugPrintAllMtrrsWorker (NULL);
}

/**
  Update the Ranges array to change the specified range identified by
  BaseAddress and Length to Type.

  @param Ranges      Array holding memory type settings for all memory regions.
  @param Capacity    The maximum count of memory ranges the array can hold.
  @param Count       Return the new memory range count in the array.
  @param BaseAddress The base address of the memory range to change type.
  @param Length      The length of the memory range to change type.
  @param Type        The new type of the specified memory range.

  @retval RETURN_SUCCESS          The type of the specified memory range is
                                  changed successfully.
  @retval RETURN_OUT_OF_RESOURCES The new type set causes the count of memory
                                  range exceeds capacity.
**/
RETURN_STATUS
MtrrLibSetMemoryType (
  IN MEMORY_RANGE                 *Ranges,
  IN UINT32                        Capacity,
  IN OUT UINT32                    *Count,
  IN UINT64                        BaseAddress,
  IN UINT64                        Length,
  IN MTRR_MEMORY_CACHE_TYPE        Type
  )
{
  UINT32                           Index;
  UINT64                           Limit;
  UINT64                           LengthLeft;
  UINT64                           LengthRight;
  UINT32                           StartIndex;
  UINT32                           EndIndex;
  UINT32                           DeltaCount;

  LengthRight = 0;
  LengthLeft  = 0;
  Limit = BaseAddress + Length;
  StartIndex = *Count;
  EndIndex = *Count;
  for (Index = 0; Index < *Count; Index++) {
    if ((StartIndex == *Count) &&
        (Ranges[Index].BaseAddress <= BaseAddress) &&
        (BaseAddress < Ranges[Index].BaseAddress + Ranges[Index].Length)) {
      StartIndex = Index;
      LengthLeft = BaseAddress - Ranges[Index].BaseAddress;
    }

    if ((EndIndex == *Count) &&
        (Ranges[Index].BaseAddress < Limit) &&
        (Limit <= Ranges[Index].BaseAddress + Ranges[Index].Length)) {
      EndIndex = Index;
      LengthRight = Ranges[Index].BaseAddress + Ranges[Index].Length - Limit;
      break;
    }
  }

  ASSERT (StartIndex != *Count && EndIndex != *Count);
  if (StartIndex == EndIndex && Ranges[StartIndex].Type == Type) {
    return RETURN_SUCCESS;
  }

  //
  // The type change may cause merging with previous range or next range.
  // Update the StartIndex, EndIndex, BaseAddress, Length so that following
  // logic doesn't need to consider merging.
  //
  if (StartIndex != 0) {
    if (LengthLeft == 0 && Ranges[StartIndex - 1].Type == Type) {
      StartIndex--;
      Length += Ranges[StartIndex].Length;
      BaseAddress -= Ranges[StartIndex].Length;
    }
  }
  if (EndIndex != (*Count) - 1) {
    if (LengthRight == 0 && Ranges[EndIndex + 1].Type == Type) {
      EndIndex++;
      Length += Ranges[EndIndex].Length;
    }
  }

  //
  // |- 0 -|- 1 -|- 2 -|- 3 -| StartIndex EndIndex DeltaCount  Count (Count = 4)
  //   |++++++++++++++++++|    0          3         1=3-0-2    3
  //   |+++++++|               0          1        -1=1-0-2    5
  //   |+|                     0          0        -2=0-0-2    6
  // |+++|                     0          0        -1=0-0-2+1  5
  //
  //
  DeltaCount = EndIndex - StartIndex - 2;
  if (LengthLeft == 0) {
    DeltaCount++;
  }
  if (LengthRight == 0) {
    DeltaCount++;
  }
  if (*Count - DeltaCount > Capacity) {
    return RETURN_OUT_OF_RESOURCES;
  }

  //
  // Reserve (-DeltaCount) space
  //
  CopyMem (&Ranges[EndIndex + 1 - DeltaCount], &Ranges[EndIndex + 1], (*Count - EndIndex - 1) * sizeof (Ranges[0]));
  *Count -= DeltaCount;

  if (LengthLeft != 0) {
    Ranges[StartIndex].Length = LengthLeft;
    StartIndex++;
  }
  if (LengthRight != 0) {
    Ranges[EndIndex - DeltaCount].BaseAddress = BaseAddress + Length;
    Ranges[EndIndex - DeltaCount].Length = LengthRight;
    Ranges[EndIndex - DeltaCount].Type = Ranges[EndIndex].Type;
  }
  Ranges[StartIndex].BaseAddress = BaseAddress;
  Ranges[StartIndex].Length = Length;
  Ranges[StartIndex].Type = Type;
  return RETURN_SUCCESS;
}

/**
  Allocate one or more variable MTRR to cover the range identified by
  BaseAddress and Length.

  @param Ranges               Memory range array holding the memory type
                              settings for all memory address.
  @param RangeCount           Count of memory ranges.
  @param VariableMtrr         Variable MTRR array.
  @param VariableMtrrCapacity Capacity of variable MTRR array.
  @param VariableMtrrCount    Count of variable MTRR.
  @param BaseAddress          Base address of the memory range.
  @param Length               Length of the memory range.
  @param Type                 MTRR type of the memory range.
  @param Alignment0           Alignment of 0.

  @retval RETURN_SUCCESS          Variable MTRRs are allocated successfully.
  @retval RETURN_OUT_OF_RESOURCES Count of variable MTRRs exceeds capacity.
**/
RETURN_STATUS
MtrrLibSetMemoryAttributeInVariableMtrr (
  IN CONST MEMORY_RANGE           *Ranges,
  IN UINT32                        RangeCount,
  IN OUT VARIABLE_MTRR             *VariableMtrr,
  IN UINT32                        VariableMtrrCapacity,
  IN OUT UINT32                    *VariableMtrrCount,
  IN UINT64                        BaseAddress,
  IN UINT64                        Length,
  IN MTRR_MEMORY_CACHE_TYPE        Type,
  IN UINT64                        Alignment0
  );

/**
  Allocate one or more variable MTRR to cover the range identified by
  BaseAddress and Length.

  The routine recursively calls MtrrLibSetMemoryAttributeInVariableMtrr()
  to allocate variable MTRRs when the range contains several sub-ranges
  with different attributes.

  @param Ranges               Memory range array holding the memory type
                              settings for all memory address.
  @param RangeCount           Count of memory ranges.
  @param VariableMtrr         Variable MTRR array.
  @param VariableMtrrCapacity Capacity of variable MTRR array.
  @param VariableMtrrCount    Count of variable MTRR.
  @param BaseAddress          Base address of the memory range.
  @param Length               Length of the memory range.
  @param Type                 MTRR type of the range.
                              If it's CacheInvalid, the memory range may
                              contains several sub-ranges with different
                              attributes.
  @param Alignment0           Alignment of 0.

  @retval RETURN_SUCCESS          Variable MTRRs are allocated successfully.
  @retval RETURN_OUT_OF_RESOURCES Count of variable MTRRs exceeds capacity.
**/
RETURN_STATUS
MtrrLibAddVariableMtrr (
  IN CONST MEMORY_RANGE           *Ranges,
  IN UINT32                        RangeCount,
  IN OUT VARIABLE_MTRR             *VariableMtrr,
  IN UINT32                        VariableMtrrCapacity,
  IN OUT UINT32                    *VariableMtrrCount,
  IN PHYSICAL_ADDRESS              BaseAddress,
  IN UINT64                        Length,
  IN MTRR_MEMORY_CACHE_TYPE        Type,
  IN UINT64                        Alignment0
)
{
  RETURN_STATUS                    Status;
  UINT32                           Index;
  UINT64                           SubLength;

  MTRR_LIB_ASSERT_ALIGNED (BaseAddress, Length);
  if (Type == CacheInvalid) {
    for (Index = 0; Index < RangeCount; Index++) {
      if (Ranges[Index].BaseAddress <= BaseAddress && BaseAddress < Ranges[Index].BaseAddress + Ranges[Index].Length) {

        //
        // Because the Length may not be aligned to BaseAddress, below code calls
        // MtrrLibSetMemoryAttributeInVariableMtrr() instead of itself.
        // MtrrLibSetMemoryAttributeInVariableMtrr() splits the range to several
        // aligned ranges.
        //
        if (Ranges[Index].BaseAddress + Ranges[Index].Length >= BaseAddress + Length) {
          return MtrrLibSetMemoryAttributeInVariableMtrr (
            Ranges, RangeCount, VariableMtrr, VariableMtrrCapacity, VariableMtrrCount,
            BaseAddress, Length, Ranges[Index].Type, Alignment0
          );
        } else {
          SubLength = Ranges[Index].BaseAddress + Ranges[Index].Length - BaseAddress;
          Status = MtrrLibSetMemoryAttributeInVariableMtrr (
            Ranges, RangeCount, VariableMtrr, VariableMtrrCapacity, VariableMtrrCount,
            BaseAddress, SubLength, Ranges[Index].Type, Alignment0
          );
          if (RETURN_ERROR (Status)) {
            return Status;
          }
          BaseAddress += SubLength;
          Length -= SubLength;
        }
      }
    }

    //
    // Because memory ranges cover all the memory addresses, it's impossible to be here.
    //
    ASSERT (FALSE);
    return RETURN_DEVICE_ERROR;
  } else {
    for (Index = 0; Index < *VariableMtrrCount; Index++) {
      if (VariableMtrr[Index].BaseAddress == BaseAddress && VariableMtrr[Index].Length == Length) {
        ASSERT (VariableMtrr[Index].Type == Type);
        break;
      }
    }
    if (Index == *VariableMtrrCount) {
      if (*VariableMtrrCount == VariableMtrrCapacity) {
        return RETURN_OUT_OF_RESOURCES;
      }
      VariableMtrr[Index].BaseAddress = BaseAddress;
      VariableMtrr[Index].Length = Length;
      VariableMtrr[Index].Type = Type;
      VariableMtrr[Index].Valid = TRUE;
      VariableMtrr[Index].Used = TRUE;
      (*VariableMtrrCount)++;
    }
    return RETURN_SUCCESS;
  }
}

/**
  Allocate one or more variable MTRR to cover the range identified by
  BaseAddress and Length.

  @param Ranges               Memory range array holding the memory type
                              settings for all memory address.
  @param RangeCount           Count of memory ranges.
  @param VariableMtrr         Variable MTRR array.
  @param VariableMtrrCapacity Capacity of variable MTRR array.
  @param VariableMtrrCount    Count of variable MTRR.
  @param BaseAddress          Base address of the memory range.
  @param Length               Length of the memory range.
  @param Type                 MTRR type of the memory range.
  @param Alignment0           Alignment of 0.

  @retval RETURN_SUCCESS          Variable MTRRs are allocated successfully.
  @retval RETURN_OUT_OF_RESOURCES Count of variable MTRRs exceeds capacity.
**/
RETURN_STATUS
MtrrLibSetMemoryAttributeInVariableMtrr (
  IN CONST MEMORY_RANGE     *Ranges,
  IN UINT32                 RangeCount,
  IN OUT VARIABLE_MTRR      *VariableMtrr,
  IN UINT32                 VariableMtrrCapacity,
  IN OUT UINT32             *VariableMtrrCount,
  IN UINT64                 BaseAddress,
  IN UINT64                 Length,
  IN MTRR_MEMORY_CACHE_TYPE Type,
  IN UINT64                 Alignment0
)
{
  UINT64                    Alignment;
  UINT32                    MtrrNumber;
  UINT32                    SubtractiveLeft;
  UINT32                    SubtractiveRight;
  BOOLEAN                   UseLeastAlignment;

  Alignment = 0;

  MtrrNumber = MtrrLibGetMtrrNumber (Ranges, RangeCount, VariableMtrr, *VariableMtrrCount,
                                     BaseAddress, Length, Type, Alignment0, &SubtractiveLeft, &SubtractiveRight);

  if (MtrrNumber + *VariableMtrrCount > VariableMtrrCapacity) {
    return RETURN_OUT_OF_RESOURCES;
  }

  while (SubtractiveLeft-- != 0) {
    Alignment = MtrrLibLeastAlignment (BaseAddress, Alignment0);
    ASSERT (Alignment <= Length);

    MtrrLibAddVariableMtrr (Ranges, RangeCount, VariableMtrr, VariableMtrrCapacity, VariableMtrrCount,
                            BaseAddress - Alignment, Alignment, CacheInvalid, Alignment0);
    BaseAddress -= Alignment;
    Length += Alignment;
  }

  while (Length != 0) {
    Alignment = MtrrLibLeastAlignment (BaseAddress, Alignment0);
    if (Alignment > Length) {
      break;
    }
    MtrrLibAddVariableMtrr (NULL, 0, VariableMtrr, VariableMtrrCapacity, VariableMtrrCount,
                            BaseAddress, Alignment, Type, Alignment0);
    BaseAddress += Alignment;
    Length -= Alignment;
  }

  while (SubtractiveRight-- != 0) {
    Alignment = MtrrLibLeastAlignment (BaseAddress + Length, Alignment0);
    MtrrLibAddVariableMtrr (Ranges, RangeCount, VariableMtrr, VariableMtrrCapacity, VariableMtrrCount,
                            BaseAddress + Length, Alignment, CacheInvalid, Alignment0);
    Length += Alignment;
  }

  UseLeastAlignment = TRUE;
  while (Length != 0) {
    if (UseLeastAlignment) {
      Alignment = MtrrLibLeastAlignment (BaseAddress, Alignment0);
      if (Alignment > Length) {
        UseLeastAlignment = FALSE;
      }
    }

    if (!UseLeastAlignment) {
      Alignment = GetPowerOfTwo64 (Length);
    }

    MtrrLibAddVariableMtrr (NULL, 0, VariableMtrr, VariableMtrrCapacity, VariableMtrrCount,
                            BaseAddress, Alignment, Type, Alignment0);
    BaseAddress += Alignment;
    Length -= Alignment;
  }
  return RETURN_SUCCESS;
}

/**
  Return an array of memory ranges holding memory type settings for all memory
  address.

  @param DefaultType       The default memory type.
  @param TotalLength       The total length of the memory.
  @param VariableMtrr      The variable MTRR array.
  @param VariableMtrrCount The count of variable MTRRs.
  @param Ranges            Return the memory range array holding memory type
                           settings for all memory address.
  @param RangeCapacity     The capacity of memory range array.
  @param RangeCount        Return the count of memory range.

  @retval RETURN_SUCCESS          The memory range array is returned successfully.
  @retval RETURN_OUT_OF_RESOURCES The count of memory ranges exceeds capacity.
**/
RETURN_STATUS
MtrrLibGetMemoryTypes (
  IN MTRR_MEMORY_CACHE_TYPE      DefaultType,
  IN UINT64                      TotalLength,
  IN CONST VARIABLE_MTRR         *VariableMtrr,
  IN UINT32                      VariableMtrrCount,
  OUT MEMORY_RANGE               *Ranges,
  IN UINT32                      RangeCapacity,
  OUT UINT32                     *RangeCount
)
{
  RETURN_STATUS                  Status;
  UINTN                          Index;

  //
  // WT > WB
  // UC > *
  // UC > * (except WB, UC) > WB
  //

  //
  // 0. Set whole range as DefaultType
  //
  *RangeCount = 1;
  Ranges[0].BaseAddress = 0;
  Ranges[0].Length = TotalLength;
  Ranges[0].Type = DefaultType;

  //
  // 1. Set WB
  //
  for (Index = 0; Index < VariableMtrrCount; Index++) {
    if (VariableMtrr[Index].Valid && VariableMtrr[Index].Type == CacheWriteBack) {
      Status = MtrrLibSetMemoryType (
        Ranges, RangeCapacity, RangeCount,
        VariableMtrr[Index].BaseAddress, VariableMtrr[Index].Length, (MTRR_MEMORY_CACHE_TYPE) VariableMtrr[Index].Type
      );
      if (RETURN_ERROR (Status)) {
        return Status;
      }
    }
  }

  //
  // 2. Set other types than WB or UC
  //
  for (Index = 0; Index < VariableMtrrCount; Index++) {
    if (VariableMtrr[Index].Valid && VariableMtrr[Index].Type != CacheWriteBack && VariableMtrr[Index].Type != CacheUncacheable) {
      Status = MtrrLibSetMemoryType (
        Ranges, RangeCapacity, RangeCount,
        VariableMtrr[Index].BaseAddress, VariableMtrr[Index].Length, (MTRR_MEMORY_CACHE_TYPE) VariableMtrr[Index].Type
      );
      if (RETURN_ERROR (Status)) {
        return Status;
      }
    }
  }

  //
  // 3. Set UC
  //
  for (Index = 0; Index < VariableMtrrCount; Index++) {
    if (VariableMtrr[Index].Valid && VariableMtrr[Index].Type == CacheUncacheable) {
      Status = MtrrLibSetMemoryType (
        Ranges, RangeCapacity, RangeCount,
        VariableMtrr[Index].BaseAddress, VariableMtrr[Index].Length, (MTRR_MEMORY_CACHE_TYPE) VariableMtrr[Index].Type
      );
      if (RETURN_ERROR (Status)) {
        return Status;
      }
    }
  }
  return RETURN_SUCCESS;
}

/**
  Worker function attempts to set the attributes for a memory range.

  If MtrrSetting is not NULL, set the attributes into the input MTRR
  settings buffer.
  If MtrrSetting is NULL, set the attributes into MTRRs registers.

  @param[in, out]  MtrrSetting       A buffer holding all MTRRs content.
  @param[in]       BaseAddress       The physical address that is the start
                                     address of a memory range.
  @param[in]       Length            The size in bytes of the memory range.
  @param[in]       Type              The MTRR type to set for the memory range.

  @retval RETURN_SUCCESS            The attributes were set for the memory
                                    range.
  @retval RETURN_INVALID_PARAMETER  Length is zero.
  @retval RETURN_UNSUPPORTED        The processor does not support one or
                                    more bytes of the memory resource range
                                    specified by BaseAddress and Length.
  @retval RETURN_UNSUPPORTED        The MTRR type is not support for the
                                    memory resource range specified
                                    by BaseAddress and Length.
  @retval RETURN_OUT_OF_RESOURCES   There are not enough system resources to
                                    modify the attributes of the memory
                                    resource range.

**/
RETURN_STATUS
MtrrSetMemoryAttributeWorker (
  IN OUT MTRR_SETTINGS           *MtrrSetting,
  IN PHYSICAL_ADDRESS            BaseAddress,
  IN UINT64                      Length,
  IN MTRR_MEMORY_CACHE_TYPE      Type
  )
{
  RETURN_STATUS             Status;
  UINT32                    Index;
  UINT32                    WorkingIndex;
  //
  // N variable MTRRs can maximumly separate (2N + 1) Ranges, plus 1 range for [0, 1M).
  //
  MEMORY_RANGE              Ranges[MTRR_NUMBER_OF_VARIABLE_MTRR * 2 + 2];
  UINT32                    RangeCount;
  UINT64                    MtrrValidBitsMask;
  UINT64                    MtrrValidAddressMask;
  UINT64                    Alignment0;
  MTRR_CONTEXT              MtrrContext;
  BOOLEAN                   MtrrContextValid;

  MTRR_MEMORY_CACHE_TYPE    DefaultType;

  UINT32                    MsrIndex;
  UINT64                    ClearMask;
  UINT64                    OrMask;
  UINT64                    NewValue;
  BOOLEAN                   FixedSettingsValid[MTRR_NUMBER_OF_FIXED_MTRR];
  BOOLEAN                   FixedSettingsModified[MTRR_NUMBER_OF_FIXED_MTRR];
  MTRR_FIXED_SETTINGS       WorkingFixedSettings;

  UINT32                    FirmwareVariableMtrrCount;
  MTRR_VARIABLE_SETTINGS    *VariableSettings;
  MTRR_VARIABLE_SETTINGS    OriginalVariableSettings;
  UINT32                    OriginalVariableMtrrCount;
  VARIABLE_MTRR             OriginalVariableMtrr[MTRR_NUMBER_OF_VARIABLE_MTRR];
  UINT32                    WorkingVariableMtrrCount;
  VARIABLE_MTRR             WorkingVariableMtrr[MTRR_NUMBER_OF_VARIABLE_MTRR];
  BOOLEAN                   VariableSettingModified[MTRR_NUMBER_OF_VARIABLE_MTRR];
  UINTN                     FreeVariableMtrrCount;

  if (Length == 0) {
    return RETURN_INVALID_PARAMETER;
  }

  MtrrLibInitializeMtrrMask (&MtrrValidBitsMask, &MtrrValidAddressMask);
  if (((BaseAddress & ~MtrrValidAddressMask) != 0) || (Length & ~MtrrValidAddressMask) != 0) {
    return RETURN_UNSUPPORTED;
  }
  OriginalVariableMtrrCount = 0;
  VariableSettings          = NULL;

  ZeroMem (&WorkingFixedSettings, sizeof (WorkingFixedSettings));
  for (Index = 0; Index < MTRR_NUMBER_OF_FIXED_MTRR; Index++) {
    FixedSettingsValid[Index]    = FALSE;
    FixedSettingsModified[Index] = FALSE;
  }

  //
  // Check if Fixed MTRR
  //
  if (BaseAddress < BASE_1MB) {
    MsrIndex = (UINT32)-1;
    while ((BaseAddress < BASE_1MB) && (Length != 0)) {
      Status = MtrrLibProgramFixedMtrr (Type, &BaseAddress, &Length, &MsrIndex, &ClearMask, &OrMask);
      if (RETURN_ERROR (Status)) {
        return Status;
      }
      if (MtrrSetting != NULL) {
        MtrrSetting->Fixed.Mtrr[MsrIndex] = (MtrrSetting->Fixed.Mtrr[MsrIndex] & ~ClearMask) | OrMask;
        ((MSR_IA32_MTRR_DEF_TYPE_REGISTER *) &MtrrSetting->MtrrDefType)->Bits.FE = 1;
      } else {
        if (!FixedSettingsValid[MsrIndex]) {
          WorkingFixedSettings.Mtrr[MsrIndex] = AsmReadMsr64 (mMtrrLibFixedMtrrTable[MsrIndex].Msr);
          FixedSettingsValid[MsrIndex] = TRUE;
        }
        NewValue = (WorkingFixedSettings.Mtrr[MsrIndex] & ~ClearMask) | OrMask;
        if (WorkingFixedSettings.Mtrr[MsrIndex] != NewValue) {
          WorkingFixedSettings.Mtrr[MsrIndex] = NewValue;
          FixedSettingsModified[MsrIndex] = TRUE;
        }
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
  }

  //
  // Read the default MTRR type
  //
  DefaultType = MtrrGetDefaultMemoryTypeWorker (MtrrSetting);

  //
  // Read all variable MTRRs and convert to Ranges.
  //
  OriginalVariableMtrrCount = GetVariableMtrrCountWorker ();
  if (MtrrSetting == NULL) {
    ZeroMem (&OriginalVariableSettings, sizeof (OriginalVariableSettings));
    MtrrGetVariableMtrrWorker (NULL, OriginalVariableMtrrCount, &OriginalVariableSettings);
    VariableSettings = &OriginalVariableSettings;
  } else {
    VariableSettings = &MtrrSetting->Variables;
  }
  MtrrGetMemoryAttributeInVariableMtrrWorker (VariableSettings, OriginalVariableMtrrCount, MtrrValidBitsMask, MtrrValidAddressMask, OriginalVariableMtrr);

  Status = MtrrLibGetMemoryTypes (
    DefaultType, MtrrValidBitsMask + 1, OriginalVariableMtrr, OriginalVariableMtrrCount,
    Ranges, 2 * OriginalVariableMtrrCount + 1, &RangeCount
  );
  ASSERT (Status == RETURN_SUCCESS);

  FirmwareVariableMtrrCount = GetFirmwareVariableMtrrCountWorker ();
  ASSERT (RangeCount <= 2 * FirmwareVariableMtrrCount + 1);

  //
  // Force [0, 1M) to UC, so that it doesn't impact left subtraction algorithm.
  //
  Status = MtrrLibSetMemoryType (Ranges, 2 * FirmwareVariableMtrrCount + 2, &RangeCount, 0, SIZE_1MB, CacheUncacheable);
  ASSERT (Status == RETURN_SUCCESS);
  //
  // Apply Type to [BaseAddress, BaseAddress + Length)
  //
  Status = MtrrLibSetMemoryType (Ranges, 2 * FirmwareVariableMtrrCount + 2, &RangeCount, BaseAddress, Length, Type);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  Alignment0 = LShiftU64 (1, (UINTN) HighBitSet64 (MtrrValidBitsMask));
  WorkingVariableMtrrCount = 0;
  ZeroMem (&WorkingVariableMtrr, sizeof (WorkingVariableMtrr));
  for (Index = 0; Index < RangeCount; Index++) {
    if (Ranges[Index].Type != DefaultType) {
      //
      // Maximum allowed MTRR count is (FirmwareVariableMtrrCount + 1)
      // Because potentially the range [0, 1MB) is not merged, but can be ignored because fixed MTRR covers that
      //
      Status = MtrrLibSetMemoryAttributeInVariableMtrr (
        Ranges, RangeCount,
        WorkingVariableMtrr, FirmwareVariableMtrrCount + 1, &WorkingVariableMtrrCount,
        Ranges[Index].BaseAddress, Ranges[Index].Length,
        Ranges[Index].Type, Alignment0
      );
      if (RETURN_ERROR (Status)) {
        return Status;
      }
    }
  }

  //
  // Remove the [0, 1MB) MTRR if it still exists (not merged with other range)
  //
  if (WorkingVariableMtrr[0].BaseAddress == 0 && WorkingVariableMtrr[0].Length == SIZE_1MB) {
    ASSERT (WorkingVariableMtrr[0].Type == CacheUncacheable);
    WorkingVariableMtrrCount--;
    CopyMem (&WorkingVariableMtrr[0], &WorkingVariableMtrr[1], WorkingVariableMtrrCount * sizeof (VARIABLE_MTRR));
  }

  if (WorkingVariableMtrrCount > FirmwareVariableMtrrCount) {
    return RETURN_OUT_OF_RESOURCES;
  }

  for (Index = 0; Index < OriginalVariableMtrrCount; Index++) {
    VariableSettingModified[Index] = FALSE;

    if (!OriginalVariableMtrr[Index].Valid) {
      continue;
    }
    for (WorkingIndex = 0; WorkingIndex < WorkingVariableMtrrCount; WorkingIndex++) {
      if (OriginalVariableMtrr[Index].BaseAddress == WorkingVariableMtrr[WorkingIndex].BaseAddress &&
          OriginalVariableMtrr[Index].Length == WorkingVariableMtrr[WorkingIndex].Length &&
          OriginalVariableMtrr[Index].Type == WorkingVariableMtrr[WorkingIndex].Type) {
        break;
      }
    }

    if (WorkingIndex == WorkingVariableMtrrCount) {
      //
      // Remove the one from OriginalVariableMtrr which is not in WorkingVariableMtrr
      //
      OriginalVariableMtrr[Index].Valid = FALSE;
      VariableSettingModified[Index] = TRUE;
    } else {
      //
      // Remove the one from WorkingVariableMtrr which is also in OriginalVariableMtrr
      //
      WorkingVariableMtrr[WorkingIndex].Valid = FALSE;
    }
    //
    // The above two operations cause that valid MTRR only exists in either OriginalVariableMtrr or WorkingVariableMtrr.
    //
  }

  //
  // Merge remaining MTRRs from WorkingVariableMtrr to OriginalVariableMtrr
  //
  for (FreeVariableMtrrCount = 0, WorkingIndex = 0, Index = 0; Index < OriginalVariableMtrrCount; Index++) {
    if (!OriginalVariableMtrr[Index].Valid) {
      for (; WorkingIndex < WorkingVariableMtrrCount; WorkingIndex++) {
        if (WorkingVariableMtrr[WorkingIndex].Valid) {
          break;
        }
      }
      if (WorkingIndex == WorkingVariableMtrrCount) {
        FreeVariableMtrrCount++;
      } else {
        CopyMem (&OriginalVariableMtrr[Index], &WorkingVariableMtrr[WorkingIndex], sizeof (VARIABLE_MTRR));
        VariableSettingModified[Index] = TRUE;
        WorkingIndex++;
      }
    }
  }
  ASSERT (OriginalVariableMtrrCount - FreeVariableMtrrCount <= FirmwareVariableMtrrCount);

  //
  // Move MTRRs after the FirmwraeVariableMtrrCount position to beginning
  //
  WorkingIndex = FirmwareVariableMtrrCount;
  for (Index = 0; Index < FirmwareVariableMtrrCount; Index++) {
    if (!OriginalVariableMtrr[Index].Valid) {
      //
      // Found an empty MTRR in WorkingIndex position
      //
      for (; WorkingIndex < OriginalVariableMtrrCount; WorkingIndex++) {
        if (OriginalVariableMtrr[WorkingIndex].Valid) {
          break;
        }
      }

      if (WorkingIndex != OriginalVariableMtrrCount) {
        CopyMem (&OriginalVariableMtrr[Index], &OriginalVariableMtrr[WorkingIndex], sizeof (VARIABLE_MTRR));
        VariableSettingModified[Index] = TRUE;
        VariableSettingModified[WorkingIndex] = TRUE;
        OriginalVariableMtrr[WorkingIndex].Valid = FALSE;
      }
    }
  }

  //
  // Convert OriginalVariableMtrr to VariableSettings
  // NOTE: MTRR from FirmwareVariableMtrr to OriginalVariableMtrr need to update as well.
  //
  for (Index = 0; Index < OriginalVariableMtrrCount; Index++) {
    if (VariableSettingModified[Index]) {
      if (OriginalVariableMtrr[Index].Valid) {
        VariableSettings->Mtrr[Index].Base = (OriginalVariableMtrr[Index].BaseAddress & MtrrValidAddressMask) | (UINT8) OriginalVariableMtrr[Index].Type;
        VariableSettings->Mtrr[Index].Mask = ((~(OriginalVariableMtrr[Index].Length - 1)) & MtrrValidAddressMask) | BIT11;
      } else {
        VariableSettings->Mtrr[Index].Base = 0;
        VariableSettings->Mtrr[Index].Mask = 0;
      }
    }
  }

Done:
  if (MtrrSetting != NULL) {
    ((MSR_IA32_MTRR_DEF_TYPE_REGISTER *) &MtrrSetting->MtrrDefType)->Bits.E = 1;
    return RETURN_SUCCESS;
  }

  MtrrContextValid = FALSE;
  //
  // Write fixed MTRRs that have been modified
  //
  for (Index = 0; Index < MTRR_NUMBER_OF_FIXED_MTRR; Index++) {
    if (FixedSettingsModified[Index]) {
      if (!MtrrContextValid) {
        MtrrLibPreMtrrChange (&MtrrContext);
        MtrrContextValid = TRUE;
      }
      AsmWriteMsr64 (
        mMtrrLibFixedMtrrTable[Index].Msr,
        WorkingFixedSettings.Mtrr[Index]
        );
    }
  }

  //
  // Write variable MTRRs
  // When only fixed MTRRs were changed, below loop doesn't run
  // because OriginalVariableMtrrCount equals to 0.
  //
  for (Index = 0; Index < OriginalVariableMtrrCount; Index++) {
    if (VariableSettingModified[Index]) {
      if (!MtrrContextValid) {
        MtrrLibPreMtrrChange (&MtrrContext);
        MtrrContextValid = TRUE;
      }
      AsmWriteMsr64 (
        MSR_IA32_MTRR_PHYSBASE0 + (Index << 1),
        VariableSettings->Mtrr[Index].Base
      );
      AsmWriteMsr64 (
        MSR_IA32_MTRR_PHYSMASK0 + (Index << 1),
        VariableSettings->Mtrr[Index].Mask
      );
    }
  }
  if (MtrrContextValid) {
    MtrrLibPostMtrrChange (&MtrrContext);
  }

  return RETURN_SUCCESS;
}

/**
  This function attempts to set the attributes for a memory range.

  @param[in]  BaseAddress        The physical address that is the start
                                 address of a memory range.
  @param[in]  Length             The size in bytes of the memory range.
  @param[in]  Attributes         The bit mask of attributes to set for the
                                 memory range.

  @retval RETURN_SUCCESS            The attributes were set for the memory
                                    range.
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
  RETURN_STATUS              Status;

  if (!IsMtrrSupported ()) {
    return RETURN_UNSUPPORTED;
  }

  Status = MtrrSetMemoryAttributeWorker (NULL, BaseAddress, Length, Attribute);
  DEBUG ((DEBUG_CACHE, "MtrrSetMemoryAttribute() %a: [%016lx, %016lx) - %r\n",
          mMtrrMemoryCacheTypeShortName[Attribute], BaseAddress, BaseAddress + Length, Status));

  if (!RETURN_ERROR (Status)) {
    MtrrDebugPrintAllMtrrsWorker (NULL);
  }
  return Status;
}

/**
  This function attempts to set the attributes into MTRR setting buffer for a memory range.

  @param[in, out]  MtrrSetting  MTRR setting buffer to be set.
  @param[in]       BaseAddress  The physical address that is the start address
                                of a memory range.
  @param[in]       Length       The size in bytes of the memory range.
  @param[in]       Attribute    The bit mask of attributes to set for the
                                memory range.

  @retval RETURN_SUCCESS            The attributes were set for the memory range.
  @retval RETURN_INVALID_PARAMETER  Length is zero.
  @retval RETURN_UNSUPPORTED        The processor does not support one or more bytes of the
                                    memory resource range specified by BaseAddress and Length.
  @retval RETURN_UNSUPPORTED        The bit mask of attributes is not support for the memory resource
                                    range specified by BaseAddress and Length.
  @retval RETURN_ACCESS_DENIED      The attributes for the memory resource range specified by
                                    BaseAddress and Length cannot be modified.
  @retval RETURN_OUT_OF_RESOURCES   There are not enough system resources to modify the attributes of
                                    the memory resource range.

**/
RETURN_STATUS
EFIAPI
MtrrSetMemoryAttributeInMtrrSettings (
  IN OUT MTRR_SETTINGS       *MtrrSetting,
  IN PHYSICAL_ADDRESS        BaseAddress,
  IN UINT64                  Length,
  IN MTRR_MEMORY_CACHE_TYPE  Attribute
  )
{
  RETURN_STATUS              Status;
  Status = MtrrSetMemoryAttributeWorker (MtrrSetting, BaseAddress, Length, Attribute);
  DEBUG((DEBUG_CACHE, "MtrrSetMemoryAttributeMtrrSettings(%p) %a: [%016lx, %016lx) - %r\n",
         MtrrSetting, mMtrrMemoryCacheTypeShortName[Attribute], BaseAddress, BaseAddress + Length, Status));

  if (!RETURN_ERROR (Status)) {
    MtrrDebugPrintAllMtrrsWorker (MtrrSetting);
  }

  return Status;
}

/**
  Worker function setting variable MTRRs

  @param[in]  VariableSettings   A buffer to hold variable MTRRs content.

**/
VOID
MtrrSetVariableMtrrWorker (
  IN MTRR_VARIABLE_SETTINGS         *VariableSettings
  )
{
  UINT32  Index;
  UINT32  VariableMtrrCount;

  VariableMtrrCount = GetVariableMtrrCountWorker ();
  ASSERT (VariableMtrrCount <= MTRR_NUMBER_OF_VARIABLE_MTRR);

  for (Index = 0; Index < VariableMtrrCount; Index++) {
    AsmWriteMsr64 (
      MSR_IA32_MTRR_PHYSBASE0 + (Index << 1),
      VariableSettings->Mtrr[Index].Base
      );
    AsmWriteMsr64 (
      MSR_IA32_MTRR_PHYSMASK0 + (Index << 1),
      VariableSettings->Mtrr[Index].Mask
      );
  }
}


/**
  This function sets variable MTRRs

  @param[in]  VariableSettings   A buffer to hold variable MTRRs content.

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

  MtrrLibPreMtrrChange (&MtrrContext);
  MtrrSetVariableMtrrWorker (VariableSettings);
  MtrrLibPostMtrrChange (&MtrrContext);
  MtrrDebugPrintAllMtrrs ();

  return  VariableSettings;
}

/**
  Worker function setting fixed MTRRs

  @param[in]  FixedSettings  A buffer to hold fixed MTRRs content.

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

  @param[in]  FixedSettings  A buffer to hold fixed MTRRs content.

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

  MtrrLibPreMtrrChange (&MtrrContext);
  MtrrSetFixedMtrrWorker (FixedSettings);
  MtrrLibPostMtrrChange (&MtrrContext);
  MtrrDebugPrintAllMtrrs ();

  return FixedSettings;
}


/**
  This function gets the content in all MTRRs (variable and fixed)

  @param[out]  MtrrSetting  A buffer to hold all MTRRs content.

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
  MtrrGetFixedMtrrWorker (&MtrrSetting->Fixed);

  //
  // Get variable MTRRs
  //
  MtrrGetVariableMtrrWorker (
    NULL,
    GetVariableMtrrCountWorker (),
    &MtrrSetting->Variables
    );

  //
  // Get MTRR_DEF_TYPE value
  //
  MtrrSetting->MtrrDefType = AsmReadMsr64 (MSR_IA32_MTRR_DEF_TYPE);

  return MtrrSetting;
}


/**
  This function sets all MTRRs (variable and fixed)

  @param[in]  MtrrSetting  A buffer holding all MTRRs content.

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

  MtrrLibPreMtrrChange (&MtrrContext);

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
  AsmWriteMsr64 (MSR_IA32_MTRR_DEF_TYPE, MtrrSetting->MtrrDefType);

  MtrrLibPostMtrrChangeEnableCache (&MtrrContext);

  return MtrrSetting;
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
  CPUID_VERSION_INFO_EDX    Edx;
  MSR_IA32_MTRRCAP_REGISTER MtrrCap;

  //
  // Check CPUID(1).EDX[12] for MTRR capability
  //
  AsmCpuid (CPUID_VERSION_INFO, NULL, NULL, NULL, &Edx.Uint32);
  if (Edx.Bits.MTRR == 0) {
    return FALSE;
  }

  //
  // Check number of variable MTRRs and fixed MTRRs existence.
  // If number of variable MTRRs is zero, or fixed MTRRs do not
  // exist, return false.
  //
  MtrrCap.Uint64 = AsmReadMsr64 (MSR_IA32_MTRRCAP);
  if ((MtrrCap.Bits.VCNT == 0) || (MtrrCap.Bits.FIX == 0)) {
    return FALSE;
  }
  return TRUE;
}

