/** @file
  MTRR setting library

  @par Note:
    Most of services in this library instance are suggested to be invoked by BSP only,
    except for MtrrSetAllMtrrs() which is used to sync BSP's MTRR setting to APs.

  Copyright (c) 2008 - 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Register/Intel/Cpuid.h>
#include <Register/Intel/Msr.h>

#include <Library/MtrrLib.h>
#include <Library/BaseLib.h>
#include <Library/CpuLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

#define OR_SEED              0x0101010101010101ull
#define CLEAR_SEED           0xFFFFFFFFFFFFFFFFull
#define MAX_WEIGHT           MAX_UINT8
#define SCRATCH_BUFFER_SIZE  (4 * SIZE_4KB)
#define MTRR_LIB_ASSERT_ALIGNED(B, L)  ASSERT ((B & ~(L - 1)) == B);

#define M(x, y)  ((x) * VertexCount + (y))
#define O(x, y)  ((y) * VertexCount + (x))

//
// Context to save and restore when MTRRs are programmed
//
typedef struct {
  UINTN                              Cr4;
  BOOLEAN                            InterruptState;
  MSR_IA32_MTRR_DEF_TYPE_REGISTER    DefType;
} MTRR_CONTEXT;

typedef struct {
  UINT64                    Address;
  UINT64                    Alignment;
  UINT64                    Length;
  MTRR_MEMORY_CACHE_TYPE    Type    : 7;

  //
  // Temprary use for calculating the best MTRR settings.
  //
  BOOLEAN                   Visited : 1;
  UINT8                     Weight;
  UINT16                    Previous;
} MTRR_LIB_ADDRESS;

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
GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR8  *mMtrrMemoryCacheTypeShortName[] = {
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
  Worker function prints all MTRRs for debugging.

  If MtrrSetting is not NULL, print MTRR settings from input MTRR
  settings buffer.
  If MtrrSetting is NULL, print MTRR settings from MTRRs.

  @param  MtrrSetting    A buffer holding all MTRRs content.
**/
VOID
MtrrDebugPrintAllMtrrsWorker (
  IN MTRR_SETTINGS  *MtrrSetting
  );

/**
  Return whether MTRR is supported.

  @param[out]  FixedMtrrSupported   Return whether fixed MTRR is supported.
  @param[out]  VariableMtrrCount    Return the max number of variable MTRRs.

  @retval TRUE  MTRR is supported when either fixed MTRR is supported or max number
                of variable MTRRs is not 0.
  @retval FALSE MTRR is not supported when both fixed MTRR is not supported and max
                number of variable MTRRs is 0.
**/
BOOLEAN
MtrrLibIsMtrrSupported (
  OUT BOOLEAN  *FixedMtrrSupported  OPTIONAL,
  OUT UINT32   *VariableMtrrCount   OPTIONAL
  )
{
  CPUID_VERSION_INFO_EDX     Edx;
  MSR_IA32_MTRRCAP_REGISTER  MtrrCap;

  //
  // MTRR is not supported in TD-Guest.
  //
  if (TdIsEnabled ()) {
    return FALSE;
  }

  //
  // Check CPUID(1).EDX[12] for MTRR capability
  //
  AsmCpuid (CPUID_VERSION_INFO, NULL, NULL, NULL, &Edx.Uint32);
  if (Edx.Bits.MTRR == 0) {
    if (FixedMtrrSupported != NULL) {
      *FixedMtrrSupported = FALSE;
    }

    if (VariableMtrrCount != NULL) {
      *VariableMtrrCount = 0;
    }

    return FALSE;
  }

  //
  // Check the number of variable MTRRs and determine whether fixed MTRRs exist.
  // If the count of variable MTRRs is zero and there are no fixed MTRRs,
  // then return false
  //
  MtrrCap.Uint64 = AsmReadMsr64 (MSR_IA32_MTRRCAP);
  ASSERT (MtrrCap.Bits.VCNT <= ARRAY_SIZE (((MTRR_VARIABLE_SETTINGS *)0)->Mtrr));
  if (FixedMtrrSupported != NULL) {
    *FixedMtrrSupported = (BOOLEAN)(MtrrCap.Bits.FIX == 1);
  }

  if (VariableMtrrCount != NULL) {
    *VariableMtrrCount = MtrrCap.Bits.VCNT;
  }

  if ((MtrrCap.Bits.VCNT == 0) && (MtrrCap.Bits.FIX == 0)) {
    return FALSE;
  }

  return TRUE;
}

/**
  Worker function returns the variable MTRR count for the CPU.

  @return Variable MTRR count

**/
UINT32
GetVariableMtrrCountWorker (
  VOID
  )
{
  MSR_IA32_MTRRCAP_REGISTER  MtrrCap;

  MtrrCap.Uint64 = AsmReadMsr64 (MSR_IA32_MTRRCAP);
  ASSERT (MtrrCap.Bits.VCNT <= ARRAY_SIZE (((MTRR_VARIABLE_SETTINGS *)0)->Mtrr));
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

  VariableMtrrCount  = GetVariableMtrrCountWorker ();
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
  IN CONST MTRR_SETTINGS  *MtrrSetting
  )
{
  MSR_IA32_MTRR_DEF_TYPE_REGISTER  DefType;

  if (MtrrSetting == NULL) {
    DefType.Uint64 = AsmReadMsr64 (MSR_IA32_MTRR_DEF_TYPE);
  } else {
    DefType.Uint64 = MtrrSetting->MtrrDefType;
  }

  return (MTRR_MEMORY_CACHE_TYPE)DefType.Bits.Type;
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
  MtrrContext->InterruptState = SaveAndDisableInterrupts ();

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
  // Save current MTRR default type and disable MTRRs
  //
  MtrrContext->DefType.Uint64 = AsmReadMsr64 (MSR_IA32_MTRR_DEF_TYPE);
  DefType.Uint64              = MtrrContext->DefType.Uint64;
  DefType.Bits.E              = 0;
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
  //
  // Enable Cache MTRR
  // Note: It's possible that MTRR was not enabled earlier.
  //       But it will be enabled here unconditionally.
  //
  MtrrContext->DefType.Bits.E = 1;
  AsmWriteMsr64 (MSR_IA32_MTRR_DEF_TYPE, MtrrContext->DefType.Uint64);

  MtrrLibPostMtrrChangeEnableCache (MtrrContext);
}

/**
  Worker function gets the content in fixed MTRRs

  @param[out]  FixedSettings  A buffer to hold fixed MTRRs content.

  @retval The pointer of FixedSettings

**/
MTRR_FIXED_SETTINGS *
MtrrGetFixedMtrrWorker (
  OUT MTRR_FIXED_SETTINGS  *FixedSettings
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
MTRR_FIXED_SETTINGS *
EFIAPI
MtrrGetFixedMtrr (
  OUT MTRR_FIXED_SETTINGS  *FixedSettings
  )
{
  BOOLEAN  FixedMtrrSupported;

  MtrrLibIsMtrrSupported (&FixedMtrrSupported, NULL);

  if (!FixedMtrrSupported) {
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
MTRR_VARIABLE_SETTINGS *
MtrrGetVariableMtrrWorker (
  IN  MTRR_SETTINGS           *MtrrSetting,
  IN  UINT32                  VariableMtrrCount,
  OUT MTRR_VARIABLE_SETTINGS  *VariableSettings
  )
{
  UINT32  Index;

  ASSERT (VariableMtrrCount <= ARRAY_SIZE (VariableSettings->Mtrr));

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

  return VariableSettings;
}

/**
  Programs fixed MTRRs registers.

  @param[in]      Type             The memory type to set.
  @param[in, out] Base             The base address of memory range.
  @param[in, out] Length           The length of memory range.
  @param[in, out] LastMsrIndex     On input, the last index of the fixed MTRR MSR to program.
                                   On return, the current index of the fixed MTRR MSR to program.
  @param[out]     ClearMask        The bits to clear in the fixed MTRR MSR.
  @param[out]     OrMask           The bits to set in the fixed MTRR MSR.

  @retval RETURN_SUCCESS      The cache type was updated successfully
  @retval RETURN_UNSUPPORTED  The requested range or cache type was invalid
                              for the fixed MTRRs.

**/
RETURN_STATUS
MtrrLibProgramFixedMtrr (
  IN     MTRR_MEMORY_CACHE_TYPE  Type,
  IN OUT UINT64                  *Base,
  IN OUT UINT64                  *Length,
  IN OUT UINT32                  *LastMsrIndex,
  OUT    UINT64                  *ClearMask,
  OUT    UINT64                  *OrMask
  )
{
  UINT32  MsrIndex;
  UINT32  LeftByteShift;
  UINT32  RightByteShift;
  UINT64  SubLength;

  //
  // Find the fixed MTRR index to be programmed
  //
  for (MsrIndex = *LastMsrIndex + 1; MsrIndex < ARRAY_SIZE (mMtrrLibFixedMtrrTable); MsrIndex++) {
    if ((*Base >= mMtrrLibFixedMtrrTable[MsrIndex].BaseAddress) &&
        (*Base <
         (
          mMtrrLibFixedMtrrTable[MsrIndex].BaseAddress +
          (8 * mMtrrLibFixedMtrrTable[MsrIndex].Length)
         )
        )
        )
    {
      break;
    }
  }

  ASSERT (MsrIndex != ARRAY_SIZE (mMtrrLibFixedMtrrTable));

  //
  // Find the begin offset in fixed MTRR and calculate byte offset of left shift
  //
  if ((((UINT32)*Base - mMtrrLibFixedMtrrTable[MsrIndex].BaseAddress) % mMtrrLibFixedMtrrTable[MsrIndex].Length) != 0) {
    //
    // Base address should be aligned to the begin of a certain Fixed MTRR range.
    //
    return RETURN_UNSUPPORTED;
  }

  LeftByteShift = ((UINT32)*Base - mMtrrLibFixedMtrrTable[MsrIndex].BaseAddress) / mMtrrLibFixedMtrrTable[MsrIndex].Length;
  ASSERT (LeftByteShift < 8);

  //
  // Find the end offset in fixed MTRR and calculate byte offset of right shift
  //
  SubLength = mMtrrLibFixedMtrrTable[MsrIndex].Length * (8 - LeftByteShift);
  if (*Length >= SubLength) {
    RightByteShift = 0;
  } else {
    if (((UINT32)(*Length) % mMtrrLibFixedMtrrTable[MsrIndex].Length) != 0) {
      //
      // Length should be aligned to the end of a certain Fixed MTRR range.
      //
      return RETURN_UNSUPPORTED;
    }

    RightByteShift = 8 - LeftByteShift - (UINT32)(*Length) / mMtrrLibFixedMtrrTable[MsrIndex].Length;
    //
    // Update SubLength by actual length
    //
    SubLength = *Length;
  }

  *ClearMask = CLEAR_SEED;
  *OrMask    = MultU64x32 (OR_SEED, (UINT32)Type);

  if (LeftByteShift != 0) {
    //
    // Clear the low bits by LeftByteShift
    //
    *ClearMask &= LShiftU64 (*ClearMask, LeftByteShift * 8);
    *OrMask    &= LShiftU64 (*OrMask, LeftByteShift * 8);
  }

  if (RightByteShift != 0) {
    //
    // Clear the high bits by RightByteShift
    //
    *ClearMask &= RShiftU64 (*ClearMask, RightByteShift * 8);
    *OrMask    &= RShiftU64 (*OrMask, RightByteShift * 8);
  }

  *Length -= SubLength;
  *Base   += SubLength;

  *LastMsrIndex = MsrIndex;

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

  ZeroMem (VariableMtrr, sizeof (VARIABLE_MTRR) * ARRAY_SIZE (VariableSettings->Mtrr));
  for (Index = 0, UsedMtrr = 0; Index < VariableMtrrCount; Index++) {
    if (((MSR_IA32_MTRR_PHYSMASK_REGISTER *)&VariableSettings->Mtrr[Index].Mask)->Bits.V != 0) {
      VariableMtrr[Index].Msr         = (UINT32)Index;
      VariableMtrr[Index].BaseAddress = (VariableSettings->Mtrr[Index].Base & MtrrValidAddressMask);
      VariableMtrr[Index].Length      =
        ((~(VariableSettings->Mtrr[Index].Mask & MtrrValidAddressMask)) & MtrrValidBitsMask) + 1;
      VariableMtrr[Index].Type  = (VariableSettings->Mtrr[Index].Base & 0x0ff);
      VariableMtrr[Index].Valid = TRUE;
      VariableMtrr[Index].Used  = TRUE;
      UsedMtrr++;
    }
  }

  return UsedMtrr;
}

/**
  Convert variable MTRRs to a RAW MTRR_MEMORY_RANGE array.
  One MTRR_MEMORY_RANGE element is created for each MTRR setting.
  The routine doesn't remove the overlap or combine the near-by region.

  @param[in]   VariableSettings      The variable MTRR values to shadow
  @param[in]   VariableMtrrCount     The number of variable MTRRs
  @param[in]   MtrrValidBitsMask     The mask for the valid bit of the MTRR
  @param[in]   MtrrValidAddressMask  The valid address mask for MTRR
  @param[out]  VariableMtrr          The array to shadow variable MTRRs content

  @return      Number of MTRRs which has been used.

**/
UINT32
MtrrLibGetRawVariableRanges (
  IN  CONST MTRR_VARIABLE_SETTINGS  *VariableSettings,
  IN  UINTN                         VariableMtrrCount,
  IN  UINT64                        MtrrValidBitsMask,
  IN  UINT64                        MtrrValidAddressMask,
  OUT MTRR_MEMORY_RANGE             *VariableMtrr
  )
{
  UINTN   Index;
  UINT32  UsedMtrr;

  ZeroMem (VariableMtrr, sizeof (MTRR_MEMORY_RANGE) * ARRAY_SIZE (VariableSettings->Mtrr));
  for (Index = 0, UsedMtrr = 0; Index < VariableMtrrCount; Index++) {
    if (((MSR_IA32_MTRR_PHYSMASK_REGISTER *)&VariableSettings->Mtrr[Index].Mask)->Bits.V != 0) {
      VariableMtrr[Index].BaseAddress = (VariableSettings->Mtrr[Index].Base & MtrrValidAddressMask);
      VariableMtrr[Index].Length      =
        ((~(VariableSettings->Mtrr[Index].Mask & MtrrValidAddressMask)) & MtrrValidBitsMask) + 1;
      VariableMtrr[Index].Type = (MTRR_MEMORY_CACHE_TYPE)(VariableSettings->Mtrr[Index].Base & 0x0ff);
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
  IN  UINT64         MtrrValidBitsMask,
  IN  UINT64         MtrrValidAddressMask,
  OUT VARIABLE_MTRR  *VariableMtrr
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
  Return the biggest alignment (lowest set bit) of address.
  The function is equivalent to: 1 << LowBitSet64 (Address).

  @param Address    The address to return the alignment.
  @param Alignment0 The alignment to return when Address is 0.

  @return The least alignment of the Address.
**/
UINT64
MtrrLibBiggestAlignment (
  UINT64  Address,
  UINT64  Alignment0
  )
{
  if (Address == 0) {
    return Alignment0;
  }

  return Address & ((~Address) + 1);
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
  return (BOOLEAN)(Left == CacheUncacheable || (Left == CacheWriteThrough && Right == CacheWriteBack));
}

/**
  Initializes the valid bits mask and valid address mask for MTRRs.

  This function initializes the valid bits mask and valid address mask for MTRRs.

  @param[out]  MtrrValidBitsMask     The mask for the valid bit of the MTRR
  @param[out]  MtrrValidAddressMask  The valid address mask for the MTRR

**/
VOID
MtrrLibInitializeMtrrMask (
  OUT UINT64  *MtrrValidBitsMask,
  OUT UINT64  *MtrrValidAddressMask
  )
{
  UINT32                                       MaxExtendedFunction;
  CPUID_VIR_PHY_ADDRESS_SIZE_EAX               VirPhyAddressSize;
  UINT32                                       MaxFunction;
  CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS_ECX  ExtendedFeatureFlagsEcx;
  MSR_IA32_TME_ACTIVATE_REGISTER               TmeActivate;

  AsmCpuid (CPUID_EXTENDED_FUNCTION, &MaxExtendedFunction, NULL, NULL, NULL);

  if (MaxExtendedFunction >= CPUID_VIR_PHY_ADDRESS_SIZE) {
    AsmCpuid (CPUID_VIR_PHY_ADDRESS_SIZE, &VirPhyAddressSize.Uint32, NULL, NULL, NULL);
  } else {
    VirPhyAddressSize.Bits.PhysicalAddressBits = 36;
  }

  //
  // CPUID enumeration of MAX_PA is unaffected by TME-MK activation and will continue
  // to report the maximum physical address bits available for software to use,
  // irrespective of the number of KeyID bits.
  // So, we need to check if TME is enabled and adjust the PA size accordingly.
  //
  AsmCpuid (CPUID_SIGNATURE, &MaxFunction, NULL, NULL, NULL);
  if (MaxFunction >= CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS) {
    AsmCpuidEx (CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS, 0, NULL, NULL, &ExtendedFeatureFlagsEcx.Uint32, NULL);
    if (ExtendedFeatureFlagsEcx.Bits.TME_EN == 1) {
      TmeActivate.Uint64 = AsmReadMsr64 (MSR_IA32_TME_ACTIVATE);
      if (TmeActivate.Bits.TmeEnable == 1) {
        VirPhyAddressSize.Bits.PhysicalAddressBits -= TmeActivate.Bits.MkTmeKeyidBits;
      }
    }
  }

  *MtrrValidBitsMask    = LShiftU64 (1, VirPhyAddressSize.Bits.PhysicalAddressBits) - 1;
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
  IN MTRR_MEMORY_CACHE_TYPE  MtrrType1,
  IN MTRR_MEMORY_CACHE_TYPE  MtrrType2
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
  IN MTRR_SETTINGS     *MtrrSetting,
  IN PHYSICAL_ADDRESS  Address
  )
{
  MSR_IA32_MTRR_DEF_TYPE_REGISTER  DefType;
  UINT64                           FixedMtrr;
  UINTN                            Index;
  UINTN                            SubIndex;
  MTRR_MEMORY_CACHE_TYPE           MtrrType;
  MTRR_MEMORY_RANGE                VariableMtrr[ARRAY_SIZE (MtrrSetting->Variables.Mtrr)];
  UINT64                           MtrrValidBitsMask;
  UINT64                           MtrrValidAddressMask;
  UINT32                           VariableMtrrCount;
  MTRR_VARIABLE_SETTINGS           VariableSettings;

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
        if ((Address >= mMtrrLibFixedMtrrTable[Index].BaseAddress) &&
            (Address < mMtrrLibFixedMtrrTable[Index].BaseAddress +
             (mMtrrLibFixedMtrrTable[Index].Length * 8)))
        {
          SubIndex =
            ((UINTN)Address - mMtrrLibFixedMtrrTable[Index].BaseAddress) /
            mMtrrLibFixedMtrrTable[Index].Length;
          if (MtrrSetting == NULL) {
            FixedMtrr = AsmReadMsr64 (mMtrrLibFixedMtrrTable[Index].Msr);
          } else {
            FixedMtrr = MtrrSetting->Fixed.Mtrr[Index];
          }

          return (MTRR_MEMORY_CACHE_TYPE)(RShiftU64 (FixedMtrr, SubIndex * 8) & 0xFF);
        }
      }
    }
  }

  VariableMtrrCount = GetVariableMtrrCountWorker ();
  ASSERT (VariableMtrrCount <= ARRAY_SIZE (MtrrSetting->Variables.Mtrr));
  MtrrGetVariableMtrrWorker (MtrrSetting, VariableMtrrCount, &VariableSettings);

  MtrrLibInitializeMtrrMask (&MtrrValidBitsMask, &MtrrValidAddressMask);
  MtrrLibGetRawVariableRanges (
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
    if (VariableMtrr[Index].Length != 0) {
      if ((Address >= VariableMtrr[Index].BaseAddress) &&
          (Address < VariableMtrr[Index].BaseAddress + VariableMtrr[Index].Length))
      {
        if (MtrrType == CacheInvalid) {
          MtrrType = (MTRR_MEMORY_CACHE_TYPE)VariableMtrr[Index].Type;
        } else {
          MtrrType = MtrrLibPrecedence (MtrrType, (MTRR_MEMORY_CACHE_TYPE)VariableMtrr[Index].Type);
        }
      }
    }
  }

  //
  // If there is no MTRR which covers the Address, use the default MTRR type.
  //
  if (MtrrType == CacheInvalid) {
    MtrrType = (MTRR_MEMORY_CACHE_TYPE)DefType.Bits.Type;
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
  IN PHYSICAL_ADDRESS  Address
  )
{
  if (!IsMtrrSupported ()) {
    return CacheUncacheable;
  }

  return MtrrGetMemoryAttributeByAddressWorker (NULL, Address);
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
  @retval RETURN_ALREADY_STARTED  The type of the specified memory range equals
                                  to the desired type.
  @retval RETURN_OUT_OF_RESOURCES The new type set causes the count of memory
                                  range exceeds capacity.
**/
RETURN_STATUS
MtrrLibSetMemoryType (
  IN MTRR_MEMORY_RANGE       *Ranges,
  IN UINTN                   Capacity,
  IN OUT UINTN               *Count,
  IN UINT64                  BaseAddress,
  IN UINT64                  Length,
  IN MTRR_MEMORY_CACHE_TYPE  Type
  )
{
  UINTN   Index;
  UINT64  Limit;
  UINT64  LengthLeft;
  UINT64  LengthRight;
  UINTN   StartIndex;
  UINTN   EndIndex;
  UINTN   DeltaCount;

  ASSERT (Length != 0);

  LengthRight = 0;
  LengthLeft  = 0;
  Limit       = BaseAddress + Length;
  StartIndex  = *Count;
  EndIndex    = *Count;
  for (Index = 0; Index < *Count; Index++) {
    if ((StartIndex == *Count) &&
        (Ranges[Index].BaseAddress <= BaseAddress) &&
        (BaseAddress < Ranges[Index].BaseAddress + Ranges[Index].Length))
    {
      StartIndex = Index;
      LengthLeft = BaseAddress - Ranges[Index].BaseAddress;
    }

    if ((EndIndex == *Count) &&
        (Ranges[Index].BaseAddress < Limit) &&
        (Limit <= Ranges[Index].BaseAddress + Ranges[Index].Length))
    {
      EndIndex    = Index;
      LengthRight = Ranges[Index].BaseAddress + Ranges[Index].Length - Limit;
      break;
    }
  }

  ASSERT (StartIndex != *Count && EndIndex != *Count);
  if ((StartIndex == EndIndex) && (Ranges[StartIndex].Type == Type)) {
    return RETURN_ALREADY_STARTED;
  }

  //
  // The type change may cause merging with previous range or next range.
  // Update the StartIndex, EndIndex, BaseAddress, Length so that following
  // logic doesn't need to consider merging.
  //
  if (StartIndex != 0) {
    if ((LengthLeft == 0) && (Ranges[StartIndex - 1].Type == Type)) {
      StartIndex--;
      Length      += Ranges[StartIndex].Length;
      BaseAddress -= Ranges[StartIndex].Length;
    }
  }

  if (EndIndex != (*Count) - 1) {
    if ((LengthRight == 0) && (Ranges[EndIndex + 1].Type == Type)) {
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
    Ranges[EndIndex - DeltaCount].Length      = LengthRight;
    Ranges[EndIndex - DeltaCount].Type        = Ranges[EndIndex].Type;
  }

  Ranges[StartIndex].BaseAddress = BaseAddress;
  Ranges[StartIndex].Length      = Length;
  Ranges[StartIndex].Type        = Type;
  return RETURN_SUCCESS;
}

/**
  Return the number of memory types in range [BaseAddress, BaseAddress + Length).

  @param Ranges      Array holding memory type settings for all memory regions.
  @param RangeCount  The count of memory ranges the array holds.
  @param BaseAddress Base address.
  @param Length      Length.
  @param Types       Return bit mask to indicate all memory types in the specified range.

  @retval  Number of memory types.
**/
UINT8
MtrrLibGetNumberOfTypes (
  IN CONST MTRR_MEMORY_RANGE  *Ranges,
  IN UINTN                    RangeCount,
  IN UINT64                   BaseAddress,
  IN UINT64                   Length,
  IN OUT UINT8                *Types  OPTIONAL
  )
{
  UINTN  Index;
  UINT8  TypeCount;
  UINT8  LocalTypes;

  TypeCount  = 0;
  LocalTypes = 0;
  for (Index = 0; Index < RangeCount; Index++) {
    if ((Ranges[Index].BaseAddress <= BaseAddress) &&
        (BaseAddress < Ranges[Index].BaseAddress + Ranges[Index].Length)
        )
    {
      if ((LocalTypes & (1 << Ranges[Index].Type)) == 0) {
        LocalTypes |= (UINT8)(1 << Ranges[Index].Type);
        TypeCount++;
      }

      if (BaseAddress + Length > Ranges[Index].BaseAddress + Ranges[Index].Length) {
        Length     -= Ranges[Index].BaseAddress + Ranges[Index].Length - BaseAddress;
        BaseAddress = Ranges[Index].BaseAddress + Ranges[Index].Length;
      } else {
        break;
      }
    }
  }

  if (Types != NULL) {
    *Types = LocalTypes;
  }

  return TypeCount;
}

/**
  Calculate the least MTRR number from vertex Start to Stop and update
  the Previous of all vertices from Start to Stop is updated to reflect
  how the memory range is covered by MTRR.

  @param VertexCount     The count of vertices in the graph.
  @param Vertices        Array holding all vertices.
  @param Weight          2-dimention array holding weights between vertices.
  @param Start           Start vertex.
  @param Stop            Stop vertex.
  @param IncludeOptional TRUE to count the optional weight.
**/
VOID
MtrrLibCalculateLeastMtrrs (
  IN UINT16            VertexCount,
  IN MTRR_LIB_ADDRESS  *Vertices,
  IN OUT CONST UINT8   *Weight,
  IN UINT16            Start,
  IN UINT16            Stop,
  IN BOOLEAN           IncludeOptional
  )
{
  UINT16  Index;
  UINT8   MinWeight;
  UINT16  MinI;
  UINT8   Mandatory;
  UINT8   Optional;

  for (Index = Start; Index <= Stop; Index++) {
    Vertices[Index].Visited = FALSE;
    Mandatory               = Weight[M (Start, Index)];
    Vertices[Index].Weight  = Mandatory;
    if (Mandatory != MAX_WEIGHT) {
      Optional                = IncludeOptional ? Weight[O (Start, Index)] : 0;
      Vertices[Index].Weight += Optional;
      ASSERT (Vertices[Index].Weight >= Optional);
    }
  }

  MinI      = Start;
  MinWeight = 0;
  while (!Vertices[Stop].Visited) {
    //
    // Update the weight from the shortest vertex to other unvisited vertices
    //
    for (Index = Start + 1; Index <= Stop; Index++) {
      if (!Vertices[Index].Visited) {
        Mandatory = Weight[M (MinI, Index)];
        if (Mandatory != MAX_WEIGHT) {
          Optional = IncludeOptional ? Weight[O (MinI, Index)] : 0;
          if (MinWeight + Mandatory + Optional <= Vertices[Index].Weight) {
            Vertices[Index].Weight   = MinWeight + Mandatory + Optional;
            Vertices[Index].Previous = MinI; // Previous is Start based.
          }
        }
      }
    }

    //
    // Find the shortest vertex from Start
    //
    MinI      = VertexCount;
    MinWeight = MAX_WEIGHT;
    for (Index = Start + 1; Index <= Stop; Index++) {
      if (!Vertices[Index].Visited && (MinWeight > Vertices[Index].Weight)) {
        MinI      = Index;
        MinWeight = Vertices[Index].Weight;
      }
    }

    //
    // Mark the shortest vertex from Start as visited
    //
    Vertices[MinI].Visited = TRUE;
  }
}

/**
  Append the MTRR setting to MTRR setting array.

  @param Mtrrs        Array holding all MTRR settings.
  @param MtrrCapacity Capacity of the MTRR array.
  @param MtrrCount    The count of MTRR settings in array.
  @param BaseAddress  Base address.
  @param Length       Length.
  @param Type         Memory type.

  @retval RETURN_SUCCESS          MTRR setting is appended to array.
  @retval RETURN_OUT_OF_RESOURCES Array is full.
**/
RETURN_STATUS
MtrrLibAppendVariableMtrr (
  IN OUT MTRR_MEMORY_RANGE       *Mtrrs,
  IN     UINT32                  MtrrCapacity,
  IN OUT UINT32                  *MtrrCount,
  IN     UINT64                  BaseAddress,
  IN     UINT64                  Length,
  IN     MTRR_MEMORY_CACHE_TYPE  Type
  )
{
  if (*MtrrCount == MtrrCapacity) {
    return RETURN_OUT_OF_RESOURCES;
  }

  Mtrrs[*MtrrCount].BaseAddress = BaseAddress;
  Mtrrs[*MtrrCount].Length      = Length;
  Mtrrs[*MtrrCount].Type        = Type;
  (*MtrrCount)++;
  return RETURN_SUCCESS;
}

/**
  Return the memory type that has the least precedence.

  @param TypeBits  Bit mask of memory type.

  @retval  Memory type that has the least precedence.
**/
MTRR_MEMORY_CACHE_TYPE
MtrrLibLowestType (
  IN      UINT8  TypeBits
  )
{
  INT8  Type;

  ASSERT (TypeBits != 0);
  for (Type = 7; (INT8)TypeBits > 0; Type--, TypeBits <<= 1) {
  }

  return (MTRR_MEMORY_CACHE_TYPE)Type;
}

/**
  Calculate the subtractive path from vertex Start to Stop.

  @param DefaultType  Default memory type.
  @param A0           Alignment to use when base address is 0.
  @param Ranges       Array holding memory type settings for all memory regions.
  @param RangeCount   The count of memory ranges the array holds.
  @param VertexCount  The count of vertices in the graph.
  @param Vertices     Array holding all vertices.
  @param Weight       2-dimention array holding weights between vertices.
  @param Start        Start vertex.
  @param Stop         Stop vertex.
  @param Types        Type bit mask of memory range from Start to Stop.
  @param TypeCount    Number of different memory types from Start to Stop.
  @param Mtrrs        Array holding all MTRR settings.
  @param MtrrCapacity Capacity of the MTRR array.
  @param MtrrCount    The count of MTRR settings in array.

  @retval RETURN_SUCCESS          The subtractive path is calculated successfully.
  @retval RETURN_OUT_OF_RESOURCES The MTRR setting array is full.

**/
RETURN_STATUS
MtrrLibCalculateSubtractivePath (
  IN MTRR_MEMORY_CACHE_TYPE   DefaultType,
  IN UINT64                   A0,
  IN CONST MTRR_MEMORY_RANGE  *Ranges,
  IN UINTN                    RangeCount,
  IN UINT16                   VertexCount,
  IN MTRR_LIB_ADDRESS         *Vertices,
  IN OUT UINT8                *Weight,
  IN UINT16                   Start,
  IN UINT16                   Stop,
  IN UINT8                    Types,
  IN UINT8                    TypeCount,
  IN OUT MTRR_MEMORY_RANGE    *Mtrrs        OPTIONAL,
  IN UINT32                   MtrrCapacity  OPTIONAL,
  IN OUT UINT32               *MtrrCount    OPTIONAL
  )
{
  RETURN_STATUS           Status;
  UINT64                  Base;
  UINT64                  Length;
  UINT8                   PrecedentTypes;
  UINTN                   Index;
  UINT64                  HBase;
  UINT64                  HLength;
  UINT64                  SubLength;
  UINT16                  SubStart;
  UINT16                  SubStop;
  UINT16                  Cur;
  UINT16                  Pre;
  MTRR_MEMORY_CACHE_TYPE  LowestType;
  MTRR_MEMORY_CACHE_TYPE  LowestPrecedentType;

  Base   = Vertices[Start].Address;
  Length = Vertices[Stop].Address - Base;

  LowestType = MtrrLibLowestType (Types);

  //
  // Clear the lowest type (highest bit) to get the precedent types
  //
  PrecedentTypes      = ~(1 << LowestType) & Types;
  LowestPrecedentType = MtrrLibLowestType (PrecedentTypes);

  if (Mtrrs == NULL) {
    Weight[M (Start, Stop)] = ((LowestType == DefaultType) ? 0 : 1);
    Weight[O (Start, Stop)] = ((LowestType == DefaultType) ? 1 : 0);
  }

  // Add all high level ranges
  HBase   = MAX_UINT64;
  HLength = 0;
  for (Index = 0; Index < RangeCount; Index++) {
    if (Length == 0) {
      break;
    }

    if ((Base < Ranges[Index].BaseAddress) || (Ranges[Index].BaseAddress + Ranges[Index].Length <= Base)) {
      continue;
    }

    //
    // Base is in the Range[Index]
    //
    if (Base + Length > Ranges[Index].BaseAddress + Ranges[Index].Length) {
      SubLength = Ranges[Index].BaseAddress + Ranges[Index].Length - Base;
    } else {
      SubLength = Length;
    }

    if (((1 << Ranges[Index].Type) & PrecedentTypes) != 0) {
      //
      // Meet a range whose types take precedence.
      // Update the [HBase, HBase + HLength) to include the range,
      // [HBase, HBase + HLength) may contain sub ranges with 2 different types, and both take precedence.
      //
      if (HBase == MAX_UINT64) {
        HBase = Base;
      }

      HLength += SubLength;
    }

    Base   += SubLength;
    Length -= SubLength;

    if (HLength == 0) {
      continue;
    }

    if ((Ranges[Index].Type == LowestType) || (Length == 0)) {
      // meet low type or end

      //
      // Add the MTRRs for each high priority type range
      // the range[HBase, HBase + HLength) contains only two types.
      // We might use positive or subtractive, depending on which way uses less MTRR
      //
      for (SubStart = Start; SubStart <= Stop; SubStart++) {
        if (Vertices[SubStart].Address == HBase) {
          break;
        }
      }

      for (SubStop = SubStart; SubStop <= Stop; SubStop++) {
        if (Vertices[SubStop].Address == HBase + HLength) {
          break;
        }
      }

      ASSERT (Vertices[SubStart].Address == HBase);
      ASSERT (Vertices[SubStop].Address == HBase + HLength);

      if ((TypeCount == 2) || (SubStart == SubStop - 1)) {
        //
        // add subtractive MTRRs for [HBase, HBase + HLength)
        // [HBase, HBase + HLength) contains only one type.
        // while - loop is to split the range to MTRR - compliant aligned range.
        //
        if (Mtrrs == NULL) {
          Weight[M (Start, Stop)] += (UINT8)(SubStop - SubStart);
        } else {
          while (SubStart != SubStop) {
            Status = MtrrLibAppendVariableMtrr (
                       Mtrrs,
                       MtrrCapacity,
                       MtrrCount,
                       Vertices[SubStart].Address,
                       Vertices[SubStart].Length,
                       Vertices[SubStart].Type
                       );
            if (RETURN_ERROR (Status)) {
              return Status;
            }

            SubStart++;
          }
        }
      } else {
        ASSERT (TypeCount == 3);
        MtrrLibCalculateLeastMtrrs (VertexCount, Vertices, Weight, SubStart, SubStop, TRUE);

        if (Mtrrs == NULL) {
          Weight[M (Start, Stop)] += Vertices[SubStop].Weight;
        } else {
          // When we need to collect the optimal path from SubStart to SubStop
          while (SubStop != SubStart) {
            Cur     = SubStop;
            Pre     = Vertices[Cur].Previous;
            SubStop = Pre;

            if (Weight[M (Pre, Cur)] + Weight[O (Pre, Cur)] != 0) {
              Status = MtrrLibAppendVariableMtrr (
                         Mtrrs,
                         MtrrCapacity,
                         MtrrCount,
                         Vertices[Pre].Address,
                         Vertices[Cur].Address - Vertices[Pre].Address,
                         (Pre != Cur - 1) ? LowestPrecedentType : Vertices[Pre].Type
                         );
              if (RETURN_ERROR (Status)) {
                return Status;
              }
            }

            if (Pre != Cur - 1) {
              Status = MtrrLibCalculateSubtractivePath (
                         DefaultType,
                         A0,
                         Ranges,
                         RangeCount,
                         VertexCount,
                         Vertices,
                         Weight,
                         Pre,
                         Cur,
                         PrecedentTypes,
                         2,
                         Mtrrs,
                         MtrrCapacity,
                         MtrrCount
                         );
              if (RETURN_ERROR (Status)) {
                return Status;
              }
            }
          }
        }
      }

      //
      // Reset HBase, HLength
      //
      HBase   = MAX_UINT64;
      HLength = 0;
    }
  }

  return RETURN_SUCCESS;
}

/**
  Calculate MTRR settings to cover the specified memory ranges.

  @param DefaultType  Default memory type.
  @param A0           Alignment to use when base address is 0.
  @param Ranges       Memory range array holding the memory type
                      settings for all memory address.
  @param RangeCount   Count of memory ranges.
  @param Scratch      A temporary scratch buffer that is used to perform the calculation.
                      This is an optional parameter that may be NULL.
  @param ScratchSize  Pointer to the size in bytes of the scratch buffer.
                      It may be updated to the actual required size when the calculation
                      needs more scratch buffer.
  @param Mtrrs        Array holding all MTRR settings.
  @param MtrrCapacity Capacity of the MTRR array.
  @param MtrrCount    The count of MTRR settings in array.

  @retval RETURN_SUCCESS          Variable MTRRs are allocated successfully.
  @retval RETURN_OUT_OF_RESOURCES Count of variable MTRRs exceeds capacity.
  @retval RETURN_BUFFER_TOO_SMALL The scratch buffer is too small for MTRR calculation.
**/
RETURN_STATUS
MtrrLibCalculateMtrrs (
  IN MTRR_MEMORY_CACHE_TYPE   DefaultType,
  IN UINT64                   A0,
  IN CONST MTRR_MEMORY_RANGE  *Ranges,
  IN UINTN                    RangeCount,
  IN VOID                     *Scratch,
  IN OUT UINTN                *ScratchSize,
  IN OUT MTRR_MEMORY_RANGE    *Mtrrs,
  IN UINT32                   MtrrCapacity,
  IN OUT UINT32               *MtrrCount
  )
{
  UINT64            Base0;
  UINT64            Base1;
  UINTN             Index;
  UINT64            Base;
  UINT64            Length;
  UINT64            Alignment;
  UINT64            SubLength;
  MTRR_LIB_ADDRESS  *Vertices;
  UINT8             *Weight;
  UINT32            VertexIndex;
  UINT32            VertexCount;
  UINTN             RequiredScratchSize;
  UINT8             TypeCount;
  UINT16            Start;
  UINT16            Stop;
  UINT8             Type;
  RETURN_STATUS     Status;

  Base0 = Ranges[0].BaseAddress;
  Base1 = Ranges[RangeCount - 1].BaseAddress + Ranges[RangeCount - 1].Length;
  MTRR_LIB_ASSERT_ALIGNED (Base0, Base1 - Base0);

  //
  // Count the number of vertices.
  //
  Vertices = (MTRR_LIB_ADDRESS *)Scratch;
  for (VertexIndex = 0, Index = 0; Index < RangeCount; Index++) {
    Base   = Ranges[Index].BaseAddress;
    Length = Ranges[Index].Length;
    while (Length != 0) {
      Alignment = MtrrLibBiggestAlignment (Base, A0);
      SubLength = Alignment;
      if (SubLength > Length) {
        SubLength = GetPowerOfTwo64 (Length);
      }

      if (VertexIndex < *ScratchSize / sizeof (*Vertices)) {
        Vertices[VertexIndex].Address   = Base;
        Vertices[VertexIndex].Alignment = Alignment;
        Vertices[VertexIndex].Type      = Ranges[Index].Type;
        Vertices[VertexIndex].Length    = SubLength;
      }

      Base   += SubLength;
      Length -= SubLength;
      VertexIndex++;
    }
  }

  //
  // Vertices[VertexIndex] = Base1, so whole vertex count is (VertexIndex + 1).
  //
  VertexCount = VertexIndex + 1;
  DEBUG ((
    DEBUG_CACHE,
    "  Count of vertices (%016llx - %016llx) = %d\n",
    Ranges[0].BaseAddress,
    Ranges[RangeCount - 1].BaseAddress + Ranges[RangeCount - 1].Length,
    VertexCount
    ));
  ASSERT (VertexCount < MAX_UINT16);

  RequiredScratchSize = VertexCount * sizeof (*Vertices) + VertexCount * VertexCount * sizeof (*Weight);
  if (*ScratchSize < RequiredScratchSize) {
    *ScratchSize = RequiredScratchSize;
    return RETURN_BUFFER_TOO_SMALL;
  }

  Vertices[VertexCount - 1].Address = Base1;

  Weight = (UINT8 *)&Vertices[VertexCount];
  for (VertexIndex = 0; VertexIndex < VertexCount; VertexIndex++) {
    //
    // Set optional weight between vertices and self->self to 0
    //
    SetMem (&Weight[M (VertexIndex, 0)], VertexIndex + 1, 0);
    //
    // Set mandatory weight between vertices to MAX_WEIGHT
    //
    SetMem (&Weight[M (VertexIndex, VertexIndex + 1)], VertexCount - VertexIndex - 1, MAX_WEIGHT);

    // Final result looks like:
    //   00 FF FF FF
    //   00 00 FF FF
    //   00 00 00 FF
    //   00 00 00 00
  }

  //
  // Set mandatory weight and optional weight for adjacent vertices
  //
  for (VertexIndex = 0; VertexIndex < VertexCount - 1; VertexIndex++) {
    if (Vertices[VertexIndex].Type != DefaultType) {
      Weight[M (VertexIndex, VertexIndex + 1)] = 1;
      Weight[O (VertexIndex, VertexIndex + 1)] = 0;
    } else {
      Weight[M (VertexIndex, VertexIndex + 1)] = 0;
      Weight[O (VertexIndex, VertexIndex + 1)] = 1;
    }
  }

  for (TypeCount = 2; TypeCount <= 3; TypeCount++) {
    for (Start = 0; Start < VertexCount; Start++) {
      for (Stop = Start + 2; Stop < VertexCount; Stop++) {
        ASSERT (Vertices[Stop].Address > Vertices[Start].Address);
        Length = Vertices[Stop].Address - Vertices[Start].Address;
        if (Length > Vertices[Start].Alignment) {
          //
          // Pickup a new Start when [Start, Stop) cannot be described by one MTRR.
          //
          break;
        }

        if ((Weight[M (Start, Stop)] == MAX_WEIGHT) && IS_POW2 (Length)) {
          if (MtrrLibGetNumberOfTypes (
                Ranges,
                RangeCount,
                Vertices[Start].Address,
                Vertices[Stop].Address - Vertices[Start].Address,
                &Type
                ) == TypeCount)
          {
            //
            // Update the Weight[Start, Stop] using subtractive path.
            //
            MtrrLibCalculateSubtractivePath (
              DefaultType,
              A0,
              Ranges,
              RangeCount,
              (UINT16)VertexCount,
              Vertices,
              Weight,
              Start,
              Stop,
              Type,
              TypeCount,
              NULL,
              0,
              NULL
              );
          } else if (TypeCount == 2) {
            //
            // Pick up a new Start when we expect 2-type range, but 3-type range is met.
            // Because no matter how Stop is increased, we always meet 3-type range.
            //
            break;
          }
        }
      }
    }
  }

  Status = RETURN_SUCCESS;
  MtrrLibCalculateLeastMtrrs ((UINT16)VertexCount, Vertices, Weight, 0, (UINT16)VertexCount - 1, FALSE);
  Stop = (UINT16)VertexCount - 1;
  while (Stop != 0) {
    Start     = Vertices[Stop].Previous;
    TypeCount = MAX_UINT8;
    Type      = 0;
    if (Weight[M (Start, Stop)] != 0) {
      TypeCount = MtrrLibGetNumberOfTypes (Ranges, RangeCount, Vertices[Start].Address, Vertices[Stop].Address - Vertices[Start].Address, &Type);
      Status    = MtrrLibAppendVariableMtrr (
                    Mtrrs,
                    MtrrCapacity,
                    MtrrCount,
                    Vertices[Start].Address,
                    Vertices[Stop].Address - Vertices[Start].Address,
                    MtrrLibLowestType (Type)
                    );
      if (RETURN_ERROR (Status)) {
        break;
      }
    }

    if (Start != Stop - 1) {
      //
      // substractive path
      //
      if (TypeCount == MAX_UINT8) {
        TypeCount = MtrrLibGetNumberOfTypes (
                      Ranges,
                      RangeCount,
                      Vertices[Start].Address,
                      Vertices[Stop].Address - Vertices[Start].Address,
                      &Type
                      );
      }

      Status = MtrrLibCalculateSubtractivePath (
                 DefaultType,
                 A0,
                 Ranges,
                 RangeCount,
                 (UINT16)VertexCount,
                 Vertices,
                 Weight,
                 Start,
                 Stop,
                 Type,
                 TypeCount,
                 Mtrrs,
                 MtrrCapacity,
                 MtrrCount
                 );
      if (RETURN_ERROR (Status)) {
        break;
      }
    }

    Stop = Start;
  }

  return Status;
}

/**
  Apply the fixed MTRR settings to memory range array.

  @param Fixed             The fixed MTRR settings.
  @param Ranges            Return the memory range array holding memory type
                           settings for all memory address.
  @param RangeCapacity     The capacity of memory range array.
  @param RangeCount        Return the count of memory range.

  @retval RETURN_SUCCESS          The memory range array is returned successfully.
  @retval RETURN_OUT_OF_RESOURCES The count of memory ranges exceeds capacity.
**/
RETURN_STATUS
MtrrLibApplyFixedMtrrs (
  IN     CONST MTRR_FIXED_SETTINGS  *Fixed,
  IN OUT MTRR_MEMORY_RANGE          *Ranges,
  IN     UINTN                      RangeCapacity,
  IN OUT UINTN                      *RangeCount
  )
{
  RETURN_STATUS           Status;
  UINTN                   MsrIndex;
  UINTN                   Index;
  MTRR_MEMORY_CACHE_TYPE  MemoryType;
  UINT64                  Base;

  Base = 0;
  for (MsrIndex = 0; MsrIndex < ARRAY_SIZE (mMtrrLibFixedMtrrTable); MsrIndex++) {
    ASSERT (Base == mMtrrLibFixedMtrrTable[MsrIndex].BaseAddress);
    for (Index = 0; Index < sizeof (UINT64); Index++) {
      MemoryType = (MTRR_MEMORY_CACHE_TYPE)((UINT8 *)(&Fixed->Mtrr[MsrIndex]))[Index];
      Status     = MtrrLibSetMemoryType (
                     Ranges,
                     RangeCapacity,
                     RangeCount,
                     Base,
                     mMtrrLibFixedMtrrTable[MsrIndex].Length,
                     MemoryType
                     );
      if (Status == RETURN_OUT_OF_RESOURCES) {
        return Status;
      }

      Base += mMtrrLibFixedMtrrTable[MsrIndex].Length;
    }
  }

  ASSERT (Base == BASE_1MB);
  return RETURN_SUCCESS;
}

/**
  Apply the variable MTRR settings to memory range array.

  @param VariableMtrr      The variable MTRR array.
  @param VariableMtrrCount The count of variable MTRRs.
  @param Ranges            Return the memory range array with new MTRR settings applied.
  @param RangeCapacity     The capacity of memory range array.
  @param RangeCount        Return the count of memory range.

  @retval RETURN_SUCCESS          The memory range array is returned successfully.
  @retval RETURN_OUT_OF_RESOURCES The count of memory ranges exceeds capacity.
**/
RETURN_STATUS
MtrrLibApplyVariableMtrrs (
  IN     CONST MTRR_MEMORY_RANGE  *VariableMtrr,
  IN     UINT32                   VariableMtrrCount,
  IN OUT MTRR_MEMORY_RANGE        *Ranges,
  IN     UINTN                    RangeCapacity,
  IN OUT UINTN                    *RangeCount
  )
{
  RETURN_STATUS  Status;
  UINTN          Index;

  //
  // WT > WB
  // UC > *
  // UC > * (except WB, UC) > WB
  //

  //
  // 1. Set WB
  //
  for (Index = 0; Index < VariableMtrrCount; Index++) {
    if ((VariableMtrr[Index].Length != 0) && (VariableMtrr[Index].Type == CacheWriteBack)) {
      Status = MtrrLibSetMemoryType (
                 Ranges,
                 RangeCapacity,
                 RangeCount,
                 VariableMtrr[Index].BaseAddress,
                 VariableMtrr[Index].Length,
                 VariableMtrr[Index].Type
                 );
      if (Status == RETURN_OUT_OF_RESOURCES) {
        return Status;
      }
    }
  }

  //
  // 2. Set other types than WB or UC
  //
  for (Index = 0; Index < VariableMtrrCount; Index++) {
    if ((VariableMtrr[Index].Length != 0) &&
        (VariableMtrr[Index].Type != CacheWriteBack) && (VariableMtrr[Index].Type != CacheUncacheable))
    {
      Status = MtrrLibSetMemoryType (
                 Ranges,
                 RangeCapacity,
                 RangeCount,
                 VariableMtrr[Index].BaseAddress,
                 VariableMtrr[Index].Length,
                 VariableMtrr[Index].Type
                 );
      if (Status == RETURN_OUT_OF_RESOURCES) {
        return Status;
      }
    }
  }

  //
  // 3. Set UC
  //
  for (Index = 0; Index < VariableMtrrCount; Index++) {
    if ((VariableMtrr[Index].Length != 0) && (VariableMtrr[Index].Type == CacheUncacheable)) {
      Status = MtrrLibSetMemoryType (
                 Ranges,
                 RangeCapacity,
                 RangeCount,
                 VariableMtrr[Index].BaseAddress,
                 VariableMtrr[Index].Length,
                 VariableMtrr[Index].Type
                 );
      if (Status == RETURN_OUT_OF_RESOURCES) {
        return Status;
      }
    }
  }

  return RETURN_SUCCESS;
}

/**
  Return the memory type bit mask that's compatible to first type in the Ranges.

  @param Ranges     Memory range array holding the memory type
                    settings for all memory address.
  @param RangeCount Count of memory ranges.

  @return Compatible memory type bit mask.
**/
UINT8
MtrrLibGetCompatibleTypes (
  IN CONST MTRR_MEMORY_RANGE  *Ranges,
  IN UINTN                    RangeCount
  )
{
  ASSERT (RangeCount != 0);

  switch (Ranges[0].Type) {
    case CacheWriteBack:
    case CacheWriteThrough:
      return (1 << CacheWriteBack) | (1 << CacheWriteThrough) | (1 << CacheUncacheable);
      break;

    case CacheWriteCombining:
    case CacheWriteProtected:
      return (1 << Ranges[0].Type) | (1 << CacheUncacheable);
      break;

    case CacheUncacheable:
      if (RangeCount == 1) {
        return (1 << CacheUncacheable);
      }

      return MtrrLibGetCompatibleTypes (&Ranges[1], RangeCount - 1);
      break;

    case CacheInvalid:
    default:
      ASSERT (FALSE);
      break;
  }

  return 0;
}

/**
  Overwrite the destination MTRR settings with the source MTRR settings.
  This routine is to make sure the modification to destination MTRR settings
  is as small as possible.

  @param DstMtrrs     Destination MTRR settings.
  @param DstMtrrCount Count of destination MTRR settings.
  @param SrcMtrrs     Source MTRR settings.
  @param SrcMtrrCount Count of source MTRR settings.
  @param Modified     Flag array to indicate which destination MTRR setting is modified.
**/
VOID
MtrrLibMergeVariableMtrr (
  MTRR_MEMORY_RANGE  *DstMtrrs,
  UINT32             DstMtrrCount,
  MTRR_MEMORY_RANGE  *SrcMtrrs,
  UINT32             SrcMtrrCount,
  BOOLEAN            *Modified
  )
{
  UINT32  DstIndex;
  UINT32  SrcIndex;

  ASSERT (SrcMtrrCount <= DstMtrrCount);

  for (DstIndex = 0; DstIndex < DstMtrrCount; DstIndex++) {
    Modified[DstIndex] = FALSE;

    if (DstMtrrs[DstIndex].Length == 0) {
      continue;
    }

    for (SrcIndex = 0; SrcIndex < SrcMtrrCount; SrcIndex++) {
      if ((DstMtrrs[DstIndex].BaseAddress == SrcMtrrs[SrcIndex].BaseAddress) &&
          (DstMtrrs[DstIndex].Length == SrcMtrrs[SrcIndex].Length) &&
          (DstMtrrs[DstIndex].Type == SrcMtrrs[SrcIndex].Type))
      {
        break;
      }
    }

    if (SrcIndex == SrcMtrrCount) {
      //
      // Remove the one from DstMtrrs which is not in SrcMtrrs
      //
      DstMtrrs[DstIndex].Length = 0;
      Modified[DstIndex]        = TRUE;
    } else {
      //
      // Remove the one from SrcMtrrs which is also in DstMtrrs
      //
      SrcMtrrs[SrcIndex].Length = 0;
    }
  }

  //
  // Now valid MTRR only exists in either DstMtrrs or SrcMtrrs.
  // Merge MTRRs from SrcMtrrs to DstMtrrs
  //
  DstIndex = 0;
  for (SrcIndex = 0; SrcIndex < SrcMtrrCount; SrcIndex++) {
    if (SrcMtrrs[SrcIndex].Length != 0) {
      //
      // Find the empty slot in DstMtrrs
      //
      while (DstIndex < DstMtrrCount) {
        if (DstMtrrs[DstIndex].Length == 0) {
          break;
        }

        DstIndex++;
      }

      ASSERT (DstIndex < DstMtrrCount);
      CopyMem (&DstMtrrs[DstIndex], &SrcMtrrs[SrcIndex], sizeof (SrcMtrrs[0]));
      Modified[DstIndex] = TRUE;
    }
  }
}

/**
  Calculate the variable MTRR settings for all memory ranges.

  @param DefaultType          Default memory type.
  @param A0                   Alignment to use when base address is 0.
  @param Ranges               Memory range array holding the memory type
                              settings for all memory address.
  @param RangeCount           Count of memory ranges.
  @param Scratch              Scratch buffer to be used in MTRR calculation.
  @param ScratchSize          Pointer to the size of scratch buffer.
  @param VariableMtrr         Array holding all MTRR settings.
  @param VariableMtrrCapacity Capacity of the MTRR array.
  @param VariableMtrrCount    The count of MTRR settings in array.

  @retval RETURN_SUCCESS          Variable MTRRs are allocated successfully.
  @retval RETURN_OUT_OF_RESOURCES Count of variable MTRRs exceeds capacity.
  @retval RETURN_BUFFER_TOO_SMALL The scratch buffer is too small for MTRR calculation.
                                  The required scratch buffer size is returned through ScratchSize.
**/
RETURN_STATUS
MtrrLibSetMemoryRanges (
  IN MTRR_MEMORY_CACHE_TYPE  DefaultType,
  IN UINT64                  A0,
  IN MTRR_MEMORY_RANGE       *Ranges,
  IN UINTN                   RangeCount,
  IN VOID                    *Scratch,
  IN OUT UINTN               *ScratchSize,
  OUT MTRR_MEMORY_RANGE      *VariableMtrr,
  IN UINT32                  VariableMtrrCapacity,
  OUT UINT32                 *VariableMtrrCount
  )
{
  RETURN_STATUS  Status;
  UINT32         Index;
  UINT64         Base0;
  UINT64         Base1;
  UINT64         Alignment;
  UINT8          CompatibleTypes;
  UINT64         Length;
  UINT32         End;
  UINTN          ActualScratchSize;
  UINTN          BiggestScratchSize;

  *VariableMtrrCount = 0;

  //
  // Since the whole ranges need multiple calls of MtrrLibCalculateMtrrs().
  // Each call needs different scratch buffer size.
  // When the provided scratch buffer size is not sufficient in any call,
  // set the GetActualScratchSize to TRUE, and following calls will only
  // calculate the actual scratch size for the caller.
  //
  BiggestScratchSize = 0;

  for (Index = 0; Index < RangeCount;) {
    Base0 = Ranges[Index].BaseAddress;

    //
    // Full step is optimal
    //
    while (Index < RangeCount) {
      ASSERT (Ranges[Index].BaseAddress == Base0);
      Alignment = MtrrLibBiggestAlignment (Base0, A0);
      while (Base0 + Alignment <= Ranges[Index].BaseAddress + Ranges[Index].Length) {
        if ((BiggestScratchSize <= *ScratchSize) && (Ranges[Index].Type != DefaultType)) {
          Status = MtrrLibAppendVariableMtrr (
                     VariableMtrr,
                     VariableMtrrCapacity,
                     VariableMtrrCount,
                     Base0,
                     Alignment,
                     Ranges[Index].Type
                     );
          if (RETURN_ERROR (Status)) {
            return Status;
          }
        }

        Base0    += Alignment;
        Alignment = MtrrLibBiggestAlignment (Base0, A0);
      }

      //
      // Remove the above range from Ranges[Index]
      //
      Ranges[Index].Length     -= Base0 - Ranges[Index].BaseAddress;
      Ranges[Index].BaseAddress = Base0;
      if (Ranges[Index].Length != 0) {
        break;
      } else {
        Index++;
      }
    }

    if (Index == RangeCount) {
      break;
    }

    //
    // Find continous ranges [Base0, Base1) which could be combined by MTRR.
    // Per SDM, the compatible types between[B0, B1) are:
    //   UC, *
    //   WB, WT
    //   UC, WB, WT
    //
    CompatibleTypes = MtrrLibGetCompatibleTypes (&Ranges[Index], RangeCount - Index);

    End = Index; // End points to last one that matches the CompatibleTypes.
    while (End + 1 < RangeCount) {
      if (((1 << Ranges[End + 1].Type) & CompatibleTypes) == 0) {
        break;
      }

      End++;
    }

    Alignment = MtrrLibBiggestAlignment (Base0, A0);
    Length    = GetPowerOfTwo64 (Ranges[End].BaseAddress + Ranges[End].Length - Base0);
    Base1     = Base0 + MIN (Alignment, Length);

    //
    // Base1 may not in Ranges[End]. Update End to the range Base1 belongs to.
    //
    End = Index;
    while (End + 1 < RangeCount) {
      if (Base1 <= Ranges[End + 1].BaseAddress) {
        break;
      }

      End++;
    }

    Length             = Ranges[End].Length;
    Ranges[End].Length = Base1 - Ranges[End].BaseAddress;
    ActualScratchSize  = *ScratchSize;
    Status             = MtrrLibCalculateMtrrs (
                           DefaultType,
                           A0,
                           &Ranges[Index],
                           End + 1 - Index,
                           Scratch,
                           &ActualScratchSize,
                           VariableMtrr,
                           VariableMtrrCapacity,
                           VariableMtrrCount
                           );
    if (Status == RETURN_BUFFER_TOO_SMALL) {
      BiggestScratchSize = MAX (BiggestScratchSize, ActualScratchSize);
      //
      // Ignore this error, because we need to calculate the biggest
      // scratch buffer size.
      //
      Status = RETURN_SUCCESS;
    }

    if (RETURN_ERROR (Status)) {
      return Status;
    }

    if (Length != Ranges[End].Length) {
      Ranges[End].BaseAddress = Base1;
      Ranges[End].Length      = Length - Ranges[End].Length;
      Index                   = End;
    } else {
      Index = End + 1;
    }
  }

  if (*ScratchSize < BiggestScratchSize) {
    *ScratchSize = BiggestScratchSize;
    return RETURN_BUFFER_TOO_SMALL;
  }

  return RETURN_SUCCESS;
}

/**
  Set the below-1MB memory attribute to fixed MTRR buffer.
  Modified flag array indicates which fixed MTRR is modified.

  @param [in, out] ClearMasks    The bits (when set) to clear in the fixed MTRR MSR.
  @param [in, out] OrMasks       The bits to set in the fixed MTRR MSR.
  @param [in]      BaseAddress   Base address.
  @param [in]      Length        Length.
  @param [in]      Type          Memory type.

  @retval RETURN_SUCCESS      The memory attribute is set successfully.
  @retval RETURN_UNSUPPORTED  The requested range or cache type was invalid
                              for the fixed MTRRs.
**/
RETURN_STATUS
MtrrLibSetBelow1MBMemoryAttribute (
  IN OUT UINT64              *ClearMasks,
  IN OUT UINT64              *OrMasks,
  IN PHYSICAL_ADDRESS        BaseAddress,
  IN UINT64                  Length,
  IN MTRR_MEMORY_CACHE_TYPE  Type
  )
{
  RETURN_STATUS  Status;
  UINT32         MsrIndex;
  UINT64         ClearMask;
  UINT64         OrMask;

  ASSERT (BaseAddress < BASE_1MB);

  MsrIndex = (UINT32)-1;
  while ((BaseAddress < BASE_1MB) && (Length != 0)) {
    Status = MtrrLibProgramFixedMtrr (Type, &BaseAddress, &Length, &MsrIndex, &ClearMask, &OrMask);
    if (RETURN_ERROR (Status)) {
      return Status;
    }

    ClearMasks[MsrIndex] = ClearMasks[MsrIndex] | ClearMask;
    OrMasks[MsrIndex]    = (OrMasks[MsrIndex] & ~ClearMask) | OrMask;
  }

  return RETURN_SUCCESS;
}

/**
  This function attempts to set the attributes into MTRR setting buffer for multiple memory ranges.

  @param[in, out]  MtrrSetting  MTRR setting buffer to be set.
  @param[in]       Scratch      A temporary scratch buffer that is used to perform the calculation.
  @param[in, out]  ScratchSize  Pointer to the size in bytes of the scratch buffer.
                                It may be updated to the actual required size when the calculation
                                needs more scratch buffer.
  @param[in]       Ranges       Pointer to an array of MTRR_MEMORY_RANGE.
                                When range overlap happens, the last one takes higher priority.
                                When the function returns, either all the attributes are set successfully,
                                or none of them is set.
  @param[in]       RangeCount   Count of MTRR_MEMORY_RANGE.

  @retval RETURN_SUCCESS            The attributes were set for all the memory ranges.
  @retval RETURN_INVALID_PARAMETER  Length in any range is zero.
  @retval RETURN_UNSUPPORTED        The processor does not support one or more bytes of the
                                    memory resource range specified by BaseAddress and Length in any range.
  @retval RETURN_UNSUPPORTED        The bit mask of attributes is not support for the memory resource
                                    range specified by BaseAddress and Length in any range.
  @retval RETURN_OUT_OF_RESOURCES   There are not enough system resources to modify the attributes of
                                    the memory resource ranges.
  @retval RETURN_ACCESS_DENIED      The attributes for the memory resource range specified by
                                    BaseAddress and Length cannot be modified.
  @retval RETURN_BUFFER_TOO_SMALL   The scratch buffer is too small for MTRR calculation.
**/
RETURN_STATUS
EFIAPI
MtrrSetMemoryAttributesInMtrrSettings (
  IN OUT MTRR_SETTINGS            *MtrrSetting,
  IN     VOID                     *Scratch,
  IN OUT UINTN                    *ScratchSize,
  IN     CONST MTRR_MEMORY_RANGE  *Ranges,
  IN     UINTN                    RangeCount
  )
{
  RETURN_STATUS  Status;
  UINT32         Index;
  UINT64         BaseAddress;
  UINT64         Length;
  BOOLEAN        VariableMtrrNeeded;

  UINT64                  MtrrValidBitsMask;
  UINT64                  MtrrValidAddressMask;
  MTRR_MEMORY_CACHE_TYPE  DefaultType;
  MTRR_VARIABLE_SETTINGS  VariableSettings;
  MTRR_MEMORY_RANGE       WorkingRanges[2 * ARRAY_SIZE (MtrrSetting->Variables.Mtrr) + 2];
  UINTN                   WorkingRangeCount;
  BOOLEAN                 Modified;
  MTRR_VARIABLE_SETTING   VariableSetting;
  UINT32                  OriginalVariableMtrrCount;
  UINT32                  FirmwareVariableMtrrCount;
  UINT32                  WorkingVariableMtrrCount;
  MTRR_MEMORY_RANGE       OriginalVariableMtrr[ARRAY_SIZE (MtrrSetting->Variables.Mtrr)];
  MTRR_MEMORY_RANGE       WorkingVariableMtrr[ARRAY_SIZE (MtrrSetting->Variables.Mtrr)];
  BOOLEAN                 VariableSettingModified[ARRAY_SIZE (MtrrSetting->Variables.Mtrr)];

  UINT64   FixedMtrrMemoryLimit;
  BOOLEAN  FixedMtrrSupported;
  UINT64   ClearMasks[ARRAY_SIZE (mMtrrLibFixedMtrrTable)];
  UINT64   OrMasks[ARRAY_SIZE (mMtrrLibFixedMtrrTable)];

  MTRR_CONTEXT  MtrrContext;
  BOOLEAN       MtrrContextValid;

  Status = RETURN_SUCCESS;
  MtrrLibInitializeMtrrMask (&MtrrValidBitsMask, &MtrrValidAddressMask);

  //
  // TRUE indicating the accordingly Variable setting needs modificaiton in OriginalVariableMtrr.
  //
  SetMem (VariableSettingModified, ARRAY_SIZE (VariableSettingModified), FALSE);

  //
  // TRUE indicating the caller requests to set variable MTRRs.
  //
  VariableMtrrNeeded        = FALSE;
  OriginalVariableMtrrCount = 0;

  //
  // 0. Dump the requests.
  //
  DEBUG_CODE_BEGIN ();
  DEBUG ((
    DEBUG_CACHE,
    "Mtrr: Set Mem Attribute to %a, ScratchSize = %x%a",
    (MtrrSetting == NULL) ? "Hardware" : "Buffer",
    *ScratchSize,
    (RangeCount <= 1) ? "," : "\n"
    ));
  for (Index = 0; Index < RangeCount; Index++) {
    DEBUG ((
      DEBUG_CACHE,
      " %a: [%016lx, %016lx)\n",
      mMtrrMemoryCacheTypeShortName[MIN (Ranges[Index].Type, CacheInvalid)],
      Ranges[Index].BaseAddress,
      Ranges[Index].BaseAddress + Ranges[Index].Length
      ));
  }

  DEBUG_CODE_END ();

  //
  // 1. Validate the parameters.
  //
  if (!MtrrLibIsMtrrSupported (&FixedMtrrSupported, &OriginalVariableMtrrCount)) {
    Status = RETURN_UNSUPPORTED;
    goto Exit;
  }

  FixedMtrrMemoryLimit = FixedMtrrSupported ? BASE_1MB : 0;

  for (Index = 0; Index < RangeCount; Index++) {
    if (Ranges[Index].Length == 0) {
      Status = RETURN_INVALID_PARAMETER;
      goto Exit;
    }

    if (((Ranges[Index].BaseAddress & ~MtrrValidAddressMask) != 0) ||
        ((((Ranges[Index].BaseAddress + Ranges[Index].Length) & ~MtrrValidAddressMask) != 0) &&
         ((Ranges[Index].BaseAddress + Ranges[Index].Length) != MtrrValidBitsMask + 1))
        )
    {
      //
      // Either the BaseAddress or the Limit doesn't follow the alignment requirement.
      // Note: It's still valid if Limit doesn't follow the alignment requirement but equals to MAX Address.
      //
      Status = RETURN_UNSUPPORTED;
      goto Exit;
    }

    if ((Ranges[Index].Type != CacheUncacheable) &&
        (Ranges[Index].Type != CacheWriteCombining) &&
        (Ranges[Index].Type != CacheWriteThrough) &&
        (Ranges[Index].Type != CacheWriteProtected) &&
        (Ranges[Index].Type != CacheWriteBack))
    {
      Status = RETURN_INVALID_PARAMETER;
      goto Exit;
    }

    if (Ranges[Index].BaseAddress + Ranges[Index].Length > FixedMtrrMemoryLimit) {
      VariableMtrrNeeded = TRUE;
    }
  }

  //
  // 2. Apply the above-1MB memory attribute settings.
  //
  if (VariableMtrrNeeded) {
    //
    // 2.1. Read all variable MTRRs and convert to Ranges.
    //
    MtrrGetVariableMtrrWorker (MtrrSetting, OriginalVariableMtrrCount, &VariableSettings);
    MtrrLibGetRawVariableRanges (
      &VariableSettings,
      OriginalVariableMtrrCount,
      MtrrValidBitsMask,
      MtrrValidAddressMask,
      OriginalVariableMtrr
      );

    DefaultType                  = MtrrGetDefaultMemoryTypeWorker (MtrrSetting);
    WorkingRangeCount            = 1;
    WorkingRanges[0].BaseAddress = 0;
    WorkingRanges[0].Length      = MtrrValidBitsMask + 1;
    WorkingRanges[0].Type        = DefaultType;

    Status = MtrrLibApplyVariableMtrrs (
               OriginalVariableMtrr,
               OriginalVariableMtrrCount,
               WorkingRanges,
               ARRAY_SIZE (WorkingRanges),
               &WorkingRangeCount
               );
    ASSERT_RETURN_ERROR (Status);

    ASSERT (OriginalVariableMtrrCount >= PcdGet32 (PcdCpuNumberOfReservedVariableMtrrs));
    FirmwareVariableMtrrCount = OriginalVariableMtrrCount - PcdGet32 (PcdCpuNumberOfReservedVariableMtrrs);
    ASSERT (WorkingRangeCount <= 2 * FirmwareVariableMtrrCount + 1);

    //
    // 2.2. Force [0, 1M) to UC, so that it doesn't impact subtraction algorithm.
    //
    if (FixedMtrrMemoryLimit != 0) {
      Status = MtrrLibSetMemoryType (
                 WorkingRanges,
                 ARRAY_SIZE (WorkingRanges),
                 &WorkingRangeCount,
                 0,
                 FixedMtrrMemoryLimit,
                 CacheUncacheable
                 );
      ASSERT (Status != RETURN_OUT_OF_RESOURCES);
    }

    //
    // 2.3. Apply the new memory attribute settings to Ranges.
    //
    Modified = FALSE;
    for (Index = 0; Index < RangeCount; Index++) {
      BaseAddress = Ranges[Index].BaseAddress;
      Length      = Ranges[Index].Length;
      if (BaseAddress < FixedMtrrMemoryLimit) {
        if (Length <= FixedMtrrMemoryLimit - BaseAddress) {
          continue;
        }

        Length     -= FixedMtrrMemoryLimit - BaseAddress;
        BaseAddress = FixedMtrrMemoryLimit;
      }

      Status = MtrrLibSetMemoryType (
                 WorkingRanges,
                 ARRAY_SIZE (WorkingRanges),
                 &WorkingRangeCount,
                 BaseAddress,
                 Length,
                 Ranges[Index].Type
                 );
      if (Status == RETURN_ALREADY_STARTED) {
        Status = RETURN_SUCCESS;
      } else if (Status == RETURN_OUT_OF_RESOURCES) {
        goto Exit;
      } else {
        ASSERT_RETURN_ERROR (Status);
        Modified = TRUE;
      }
    }

    if (Modified) {
      //
      // 2.4. Calculate the Variable MTRR settings based on the Ranges.
      //      Buffer Too Small may be returned if the scratch buffer size is insufficient.
      //
      Status = MtrrLibSetMemoryRanges (
                 DefaultType,
                 LShiftU64 (1, (UINTN)HighBitSet64 (MtrrValidBitsMask)),
                 WorkingRanges,
                 WorkingRangeCount,
                 Scratch,
                 ScratchSize,
                 WorkingVariableMtrr,
                 FirmwareVariableMtrrCount + 1,
                 &WorkingVariableMtrrCount
                 );
      if (RETURN_ERROR (Status)) {
        goto Exit;
      }

      //
      // 2.5. Remove the [0, 1MB) MTRR if it still exists (not merged with other range)
      //
      for (Index = 0; Index < WorkingVariableMtrrCount; Index++) {
        if ((WorkingVariableMtrr[Index].BaseAddress == 0) && (WorkingVariableMtrr[Index].Length == FixedMtrrMemoryLimit)) {
          ASSERT (WorkingVariableMtrr[Index].Type == CacheUncacheable);
          WorkingVariableMtrrCount--;
          CopyMem (
            &WorkingVariableMtrr[Index],
            &WorkingVariableMtrr[Index + 1],
            (WorkingVariableMtrrCount - Index) * sizeof (WorkingVariableMtrr[0])
            );
          break;
        }
      }

      if (WorkingVariableMtrrCount > FirmwareVariableMtrrCount) {
        Status = RETURN_OUT_OF_RESOURCES;
        goto Exit;
      }

      //
      // 2.6. Merge the WorkingVariableMtrr to OriginalVariableMtrr
      //      Make sure least modification is made to OriginalVariableMtrr.
      //
      MtrrLibMergeVariableMtrr (
        OriginalVariableMtrr,
        OriginalVariableMtrrCount,
        WorkingVariableMtrr,
        WorkingVariableMtrrCount,
        VariableSettingModified
        );
    }
  }

  //
  // 3. Apply the below-1MB memory attribute settings.
  //
  // (Value & ~0 | 0) still equals to (Value)
  //
  ZeroMem (ClearMasks, sizeof (ClearMasks));
  ZeroMem (OrMasks, sizeof (OrMasks));
  for (Index = 0; Index < RangeCount; Index++) {
    if (Ranges[Index].BaseAddress >= FixedMtrrMemoryLimit) {
      continue;
    }

    Status = MtrrLibSetBelow1MBMemoryAttribute (
               ClearMasks,
               OrMasks,
               Ranges[Index].BaseAddress,
               Ranges[Index].Length,
               Ranges[Index].Type
               );
    if (RETURN_ERROR (Status)) {
      goto Exit;
    }
  }

  MtrrContextValid = FALSE;
  //
  // 4. Write fixed MTRRs that have been modified
  //
  for (Index = 0; Index < ARRAY_SIZE (ClearMasks); Index++) {
    if (ClearMasks[Index] != 0) {
      if (MtrrSetting != NULL) {
        //
        // Fixed MTRR is modified indicating fixed MTRR should be enabled in the end of MTRR programming.
        //
        ((MSR_IA32_MTRR_DEF_TYPE_REGISTER *)&MtrrSetting->MtrrDefType)->Bits.FE = 1;
        MtrrSetting->Fixed.Mtrr[Index]                                          = (MtrrSetting->Fixed.Mtrr[Index] & ~ClearMasks[Index]) | OrMasks[Index];
      } else {
        if (!MtrrContextValid) {
          MtrrLibPreMtrrChange (&MtrrContext);
          //
          // Fixed MTRR is modified indicating fixed MTRR should be enabled in the end of MTRR programming.
          //
          MtrrContext.DefType.Bits.FE = 1;
          MtrrContextValid            = TRUE;
        }

        AsmMsrAndThenOr64 (mMtrrLibFixedMtrrTable[Index].Msr, ~ClearMasks[Index], OrMasks[Index]);
      }
    }
  }

  //
  // 5. Write variable MTRRs that have been modified
  //
  for (Index = 0; Index < OriginalVariableMtrrCount; Index++) {
    if (VariableSettingModified[Index]) {
      if (OriginalVariableMtrr[Index].Length != 0) {
        VariableSetting.Base = (OriginalVariableMtrr[Index].BaseAddress & MtrrValidAddressMask)
                               | (UINT8)OriginalVariableMtrr[Index].Type;
        VariableSetting.Mask = ((~(OriginalVariableMtrr[Index].Length - 1)) & MtrrValidAddressMask) | BIT11;
      } else {
        VariableSetting.Base = 0;
        VariableSetting.Mask = 0;
      }

      if (MtrrSetting != NULL) {
        CopyMem (&MtrrSetting->Variables.Mtrr[Index], &VariableSetting, sizeof (VariableSetting));
      } else {
        if (!MtrrContextValid) {
          MtrrLibPreMtrrChange (&MtrrContext);
          MtrrContextValid = TRUE;
        }

        AsmWriteMsr64 (
          MSR_IA32_MTRR_PHYSBASE0 + (Index << 1),
          VariableSetting.Base
          );
        AsmWriteMsr64 (
          MSR_IA32_MTRR_PHYSMASK0 + (Index << 1),
          VariableSetting.Mask
          );
      }
    }
  }

  if (MtrrSetting != NULL) {
    //
    // Enable MTRR unconditionally
    //
    ((MSR_IA32_MTRR_DEF_TYPE_REGISTER *)&MtrrSetting->MtrrDefType)->Bits.E = 1;
  } else {
    if (MtrrContextValid) {
      MtrrLibPostMtrrChange (&MtrrContext);
    }
  }

Exit:
  DEBUG ((DEBUG_CACHE, "  Result = %r\n", Status));
  if (!RETURN_ERROR (Status)) {
    MtrrDebugPrintAllMtrrsWorker (MtrrSetting);
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
                                    Multiple memory range attributes setting by calling this API multiple
                                    times may fail with status RETURN_OUT_OF_RESOURCES. It may not mean
                                    the number of CPU MTRRs are too small to set such memory attributes.
                                    Pass the multiple memory range attributes to one call of
                                    MtrrSetMemoryAttributesInMtrrSettings() may succeed.
  @retval RETURN_BUFFER_TOO_SMALL   The fixed internal scratch buffer is too small for MTRR calculation.
                                    Caller should use MtrrSetMemoryAttributesInMtrrSettings() to specify
                                    external scratch buffer.
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
  UINT8              Scratch[SCRATCH_BUFFER_SIZE];
  UINTN              ScratchSize;
  MTRR_MEMORY_RANGE  Range;

  Range.BaseAddress = BaseAddress;
  Range.Length      = Length;
  Range.Type        = Attribute;
  ScratchSize       = sizeof (Scratch);
  return MtrrSetMemoryAttributesInMtrrSettings (MtrrSetting, Scratch, &ScratchSize, &Range, 1);
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
                                    Multiple memory range attributes setting by calling this API multiple
                                    times may fail with status RETURN_OUT_OF_RESOURCES. It may not mean
                                    the number of CPU MTRRs are too small to set such memory attributes.
                                    Pass the multiple memory range attributes to one call of
                                    MtrrSetMemoryAttributesInMtrrSettings() may succeed.
  @retval RETURN_BUFFER_TOO_SMALL   The fixed internal scratch buffer is too small for MTRR calculation.
                                    Caller should use MtrrSetMemoryAttributesInMtrrSettings() to specify
                                    external scratch buffer.
**/
RETURN_STATUS
EFIAPI
MtrrSetMemoryAttribute (
  IN PHYSICAL_ADDRESS        BaseAddress,
  IN UINT64                  Length,
  IN MTRR_MEMORY_CACHE_TYPE  Attribute
  )
{
  return MtrrSetMemoryAttributeInMtrrSettings (NULL, BaseAddress, Length, Attribute);
}

/**
  Worker function setting variable MTRRs

  @param[in]  VariableSettings   A buffer to hold variable MTRRs content.

**/
VOID
MtrrSetVariableMtrrWorker (
  IN MTRR_VARIABLE_SETTINGS  *VariableSettings
  )
{
  UINT32  Index;
  UINT32  VariableMtrrCount;

  VariableMtrrCount = GetVariableMtrrCountWorker ();
  ASSERT (VariableMtrrCount <= ARRAY_SIZE (VariableSettings->Mtrr));

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
  Worker function setting fixed MTRRs

  @param[in]  FixedSettings  A buffer to hold fixed MTRRs content.

**/
VOID
MtrrSetFixedMtrrWorker (
  IN MTRR_FIXED_SETTINGS  *FixedSettings
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
  This function gets the content in all MTRRs (variable and fixed)

  @param[out]  MtrrSetting  A buffer to hold all MTRRs content.

  @retval the pointer of MtrrSetting

**/
MTRR_SETTINGS *
EFIAPI
MtrrGetAllMtrrs (
  OUT MTRR_SETTINGS  *MtrrSetting
  )
{
  BOOLEAN                          FixedMtrrSupported;
  UINT32                           VariableMtrrCount;
  MSR_IA32_MTRR_DEF_TYPE_REGISTER  *MtrrDefType;

  ZeroMem (MtrrSetting, sizeof (*MtrrSetting));

  MtrrDefType = (MSR_IA32_MTRR_DEF_TYPE_REGISTER *)&MtrrSetting->MtrrDefType;
  if (!MtrrLibIsMtrrSupported (&FixedMtrrSupported, &VariableMtrrCount)) {
    return MtrrSetting;
  }

  //
  // Get MTRR_DEF_TYPE value
  //
  MtrrDefType->Uint64 = AsmReadMsr64 (MSR_IA32_MTRR_DEF_TYPE);

  //
  // Enabling the Fixed MTRR bit when unsupported is not allowed.
  //
  ASSERT (FixedMtrrSupported || (MtrrDefType->Bits.FE == 0));

  //
  // Get fixed MTRRs
  //
  if (MtrrDefType->Bits.FE == 1) {
    MtrrGetFixedMtrrWorker (&MtrrSetting->Fixed);
  }

  //
  // Get variable MTRRs
  //
  MtrrGetVariableMtrrWorker (
    NULL,
    VariableMtrrCount,
    &MtrrSetting->Variables
    );

  return MtrrSetting;
}

/**
  This function sets all MTRRs includes Variable and Fixed.

  The behavior of this function is to program everything in MtrrSetting to hardware.
  MTRRs might not be enabled because the enable bit is clear in MtrrSetting->MtrrDefType.

  @param[in]  MtrrSetting  A buffer holding all MTRRs content.

  @retval The pointer of MtrrSetting

**/
MTRR_SETTINGS *
EFIAPI
MtrrSetAllMtrrs (
  IN MTRR_SETTINGS  *MtrrSetting
  )
{
  BOOLEAN                          FixedMtrrSupported;
  MSR_IA32_MTRR_DEF_TYPE_REGISTER  *MtrrDefType;
  MTRR_CONTEXT                     MtrrContext;

  MtrrDefType = (MSR_IA32_MTRR_DEF_TYPE_REGISTER *)&MtrrSetting->MtrrDefType;
  if (!MtrrLibIsMtrrSupported (&FixedMtrrSupported, NULL)) {
    return MtrrSetting;
  }

  MtrrLibPreMtrrChange (&MtrrContext);

  //
  // Enabling the Fixed MTRR bit when unsupported is not allowed.
  //
  ASSERT (FixedMtrrSupported || (MtrrDefType->Bits.FE == 0));

  //
  // If the hardware supports Fixed MTRR, it is sufficient
  // to set MTRRs regardless of whether Fixed MTRR bit is enabled.
  //
  if (FixedMtrrSupported) {
    MtrrSetFixedMtrrWorker (&MtrrSetting->Fixed);
  }

  //
  // Set Variable MTRRs
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
  return MtrrLibIsMtrrSupported (NULL, NULL);
}

/**
  This function returns a Ranges array containing the memory cache types
  of all memory addresses.

  @param[in]      MtrrSetting  MTRR setting buffer to parse.
  @param[out]     Ranges       Pointer to an array of MTRR_MEMORY_RANGE.
  @param[in,out]  RangeCount   Count of MTRR_MEMORY_RANGE.
                               On input, the maximum entries the Ranges can hold.
                               On output, the actual entries that the function returns.

  @retval RETURN_INVALID_PARAMETER RangeCount is NULL.
  @retval RETURN_INVALID_PARAMETER *RangeCount is not 0 but Ranges is NULL.
  @retval RETURN_BUFFER_TOO_SMALL  *RangeCount is too small.
  @retval RETURN_SUCCESS           Ranges are successfully returned.
**/
RETURN_STATUS
EFIAPI
MtrrGetMemoryAttributesInMtrrSettings (
  IN CONST MTRR_SETTINGS      *MtrrSetting OPTIONAL,
  OUT      MTRR_MEMORY_RANGE  *Ranges,
  IN OUT   UINTN              *RangeCount
  )
{
  RETURN_STATUS                    Status;
  MTRR_SETTINGS                    LocalMtrrs;
  CONST MTRR_SETTINGS              *Mtrrs;
  MSR_IA32_MTRR_DEF_TYPE_REGISTER  *MtrrDefType;
  UINTN                            LocalRangeCount;
  UINT64                           MtrrValidBitsMask;
  UINT64                           MtrrValidAddressMask;
  UINT32                           VariableMtrrCount;
  MTRR_MEMORY_RANGE                RawVariableRanges[ARRAY_SIZE (Mtrrs->Variables.Mtrr)];
  MTRR_MEMORY_RANGE                LocalRanges[
                                               ARRAY_SIZE (mMtrrLibFixedMtrrTable) * sizeof (UINT64) + 2 * ARRAY_SIZE (Mtrrs->Variables.Mtrr) + 1
  ];

  if (RangeCount == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  if ((*RangeCount != 0) && (Ranges == NULL)) {
    return RETURN_INVALID_PARAMETER;
  }

  if (MtrrSetting != NULL) {
    Mtrrs = MtrrSetting;
  } else {
    MtrrGetAllMtrrs (&LocalMtrrs);
    Mtrrs = &LocalMtrrs;
  }

  MtrrDefType = (MSR_IA32_MTRR_DEF_TYPE_REGISTER *)&Mtrrs->MtrrDefType;

  LocalRangeCount = 1;
  MtrrLibInitializeMtrrMask (&MtrrValidBitsMask, &MtrrValidAddressMask);
  LocalRanges[0].BaseAddress = 0;
  LocalRanges[0].Length      = MtrrValidBitsMask + 1;

  if (MtrrDefType->Bits.E == 0) {
    LocalRanges[0].Type = CacheUncacheable;
  } else {
    LocalRanges[0].Type = MtrrGetDefaultMemoryTypeWorker (Mtrrs);

    VariableMtrrCount = GetVariableMtrrCountWorker ();
    ASSERT (VariableMtrrCount <= ARRAY_SIZE (MtrrSetting->Variables.Mtrr));

    MtrrLibGetRawVariableRanges (
      &Mtrrs->Variables,
      VariableMtrrCount,
      MtrrValidBitsMask,
      MtrrValidAddressMask,
      RawVariableRanges
      );
    Status = MtrrLibApplyVariableMtrrs (
               RawVariableRanges,
               VariableMtrrCount,
               LocalRanges,
               ARRAY_SIZE (LocalRanges),
               &LocalRangeCount
               );
    ASSERT_RETURN_ERROR (Status);

    if (MtrrDefType->Bits.FE == 1) {
      MtrrLibApplyFixedMtrrs (&Mtrrs->Fixed, LocalRanges, ARRAY_SIZE (LocalRanges), &LocalRangeCount);
    }
  }

  if (*RangeCount < LocalRangeCount) {
    *RangeCount = LocalRangeCount;
    return RETURN_BUFFER_TOO_SMALL;
  }

  CopyMem (Ranges, LocalRanges, LocalRangeCount * sizeof (LocalRanges[0]));
  *RangeCount = LocalRangeCount;
  return RETURN_SUCCESS;
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
  IN MTRR_SETTINGS  *MtrrSetting
  )
{
  DEBUG_CODE_BEGIN ();
  UINT32             Index;
  MTRR_SETTINGS      LocalMtrrs;
  MTRR_SETTINGS      *Mtrrs;
  RETURN_STATUS      Status;
  UINTN              RangeCount;
  BOOLEAN            ContainVariableMtrr;
  MTRR_MEMORY_RANGE  Ranges[
                            ARRAY_SIZE (mMtrrLibFixedMtrrTable) * sizeof (UINT64) + 2 * ARRAY_SIZE (Mtrrs->Variables.Mtrr) + 1
  ];

  if (MtrrSetting != NULL) {
    Mtrrs = MtrrSetting;
  } else {
    MtrrGetAllMtrrs (&LocalMtrrs);
    Mtrrs = &LocalMtrrs;
  }

  RangeCount = ARRAY_SIZE (Ranges);
  Status     = MtrrGetMemoryAttributesInMtrrSettings (Mtrrs, Ranges, &RangeCount);
  if (RETURN_ERROR (Status)) {
    DEBUG ((DEBUG_CACHE, "MTRR is not enabled.\n"));
    return;
  }

  //
  // Dump RAW MTRR contents
  //
  DEBUG ((DEBUG_CACHE, "MTRR Settings:\n"));
  DEBUG ((DEBUG_CACHE, "=============\n"));
  DEBUG ((DEBUG_CACHE, "MTRR Default Type: %016lx\n", Mtrrs->MtrrDefType));
  for (Index = 0; Index < ARRAY_SIZE (mMtrrLibFixedMtrrTable); Index++) {
    DEBUG ((DEBUG_CACHE, "Fixed MTRR[%02d]   : %016lx\n", Index, Mtrrs->Fixed.Mtrr[Index]));
  }

  ContainVariableMtrr = FALSE;
  for (Index = 0; Index < ARRAY_SIZE (Mtrrs->Variables.Mtrr); Index++) {
    if ((Mtrrs->Variables.Mtrr[Index].Mask & BIT11) == 0) {
      //
      // If mask is not valid, then do not display range
      //
      continue;
    }

    ContainVariableMtrr = TRUE;
    DEBUG ((
      DEBUG_CACHE,
      "Variable MTRR[%02d]: Base=%016lx Mask=%016lx\n",
      Index,
      Mtrrs->Variables.Mtrr[Index].Base,
      Mtrrs->Variables.Mtrr[Index].Mask
      ));
  }

  if (!ContainVariableMtrr) {
    DEBUG ((DEBUG_CACHE, "Variable MTRR    : None.\n"));
  }

  DEBUG ((DEBUG_CACHE, "\n"));

  //
  // Dump MTRR setting in ranges
  //
  DEBUG ((DEBUG_CACHE, "Memory Ranges:\n"));
  DEBUG ((DEBUG_CACHE, "====================================\n"));
  for (Index = 0; Index < RangeCount; Index++) {
    DEBUG ((
      DEBUG_CACHE,
      "%a:%016lx-%016lx\n",
      mMtrrMemoryCacheTypeShortName[Ranges[Index].Type],
      Ranges[Index].BaseAddress,
      Ranges[Index].BaseAddress + Ranges[Index].Length - 1
      ));
  }

  DEBUG_CODE_END ();
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
