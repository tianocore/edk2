/** @file

  Copyright (c) 2014 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/CacheLib.h>
#include <Library/CacheAsRamLib.h>
#include "CacheLibInternal.h"

/**
  Search the memory cache type for specific memory from MTRR.

  @param[in]  MemoryAddress         the address of target memory
  @param[in]  MemoryLength          the length of target memory
  @param[in]  ValidMtrrAddressMask  the MTRR address mask
  @param[out] UsedMsrNum            the used MSR number
  @param[out] UsedMemoryCacheType   the cache type for the target memory

  @retval EFI_SUCCESS    The memory is found in MTRR and cache type is returned
  @retval EFI_NOT_FOUND  The memory is not found in MTRR

**/
EFI_STATUS
SearchForExactMtrr (
  IN  EFI_PHYSICAL_ADDRESS   MemoryAddress,
  IN  UINT64                 MemoryLength,
  IN  UINT64                 ValidMtrrAddressMask,
  OUT UINT32                 *UsedMsrNum,
  OUT EFI_MEMORY_CACHE_TYPE  *MemoryCacheType
  );

/**
  Check if CacheType match current default setting.

  @param[in] MemoryCacheType  input cache type to be checked.

  @retval TRUE MemoryCacheType is default MTRR setting.
  @retval FALSE MemoryCacheType is NOT default MTRR setting.
**/
BOOLEAN
IsDefaultType (
  IN  EFI_MEMORY_CACHE_TYPE  MemoryCacheType
  );

/**
  Return MTRR alignment requirement for base address and size.

  @param[in]  BaseAddress     Base address.
  @param[in]  Size            Size.

  @retval Zero      Aligned.
  @retval Non-Zero  Not aligned.

**/
UINT32
CheckMtrrAlignment (
  IN  UINT64  BaseAddress,
  IN  UINT64  Size
  );

typedef struct {
  UINT32    Msr;
  UINT32    BaseAddress;
  UINT32    Length;
} EFI_FIXED_MTRR;

EFI_FIXED_MTRR  mFixedMtrrTable[] = {
  { EFI_MSR_IA32_MTRR_FIX64K_00000, 0,       0x10000 },
  { EFI_MSR_IA32_MTRR_FIX16K_80000, 0x80000, 0x4000  },
  { EFI_MSR_IA32_MTRR_FIX16K_A0000, 0xA0000, 0x4000  },
  { EFI_MSR_IA32_MTRR_FIX4K_C0000,  0xC0000, 0x1000  },
  { EFI_MSR_IA32_MTRR_FIX4K_C8000,  0xC8000, 0x1000  },
  { EFI_MSR_IA32_MTRR_FIX4K_D0000,  0xD0000, 0x1000  },
  { EFI_MSR_IA32_MTRR_FIX4K_D8000,  0xD8000, 0x1000  },
  { EFI_MSR_IA32_MTRR_FIX4K_E0000,  0xE0000, 0x1000  },
  { EFI_MSR_IA32_MTRR_FIX4K_E8000,  0xE8000, 0x1000  },
  { EFI_MSR_IA32_MTRR_FIX4K_F0000,  0xF0000, 0x1000  },
  { EFI_MSR_IA32_MTRR_FIX4K_F8000,  0xF8000, 0x1000  }
};

/**
  Given the input, check if the number of MTRR is lesser.
  if positive or subtractive.

  @param[in]  Input   Length of Memory to program MTRR.

  @retval  Zero      do positive.
  @retval  Non-Zero  do subtractive.

**/
INT8
CheckDirection (
  IN  UINT64  Input
  )
{
  return 0;
}

/**
  Disable cache and its mtrr.

  @param[out]  OldMtrr To return the Old MTRR value

**/
VOID
EfiDisableCacheMtrr (
  OUT UINT64  *OldMtrr
  )
{
  UINT64  TempQword;

  //
  // Disable Cache MTRR
  //
  *OldMtrr  = AsmReadMsr64 (EFI_MSR_CACHE_IA32_MTRR_DEF_TYPE);
  TempQword = (*OldMtrr) & ~B_EFI_MSR_GLOBAL_MTRR_ENABLE & ~B_EFI_MSR_FIXED_MTRR_ENABLE;
  AsmWriteMsr64 (EFI_MSR_CACHE_IA32_MTRR_DEF_TYPE, TempQword);
  AsmDisableCache ();
}

/**
  Recover cache MTRR.

  @param[in] EnableMtrr Whether to enable the MTRR
  @param[in] OldMtrr    The saved old MTRR value to restore when not to enable the MTRR

**/
VOID
EfiRecoverCacheMtrr (
  IN BOOLEAN  EnableMtrr,
  IN UINT64   OldMtrr
  )
{
  UINT64  TempQword;

  //
  // Enable Cache MTRR
  //
  if (EnableMtrr) {
    TempQword  = AsmReadMsr64 (EFI_MSR_CACHE_IA32_MTRR_DEF_TYPE);
    TempQword |= (UINT64)(B_EFI_MSR_GLOBAL_MTRR_ENABLE | B_EFI_MSR_FIXED_MTRR_ENABLE);
  } else {
    TempQword = OldMtrr;
  }

  AsmWriteMsr64 (EFI_MSR_CACHE_IA32_MTRR_DEF_TYPE, TempQword);

  AsmEnableCache ();
}

/**
  Programming MTRR according to Memory address, length, and type.

  @param[in] MtrrNumber           the variable MTRR index number
  @param[in] MemoryAddress        the address of target memory
  @param[in] MemoryLength         the length of target memory
  @param[in] MemoryCacheType      the cache type of target memory
  @param[in] ValidMtrrAddressMask the MTRR address mask

**/
VOID
EfiProgramMtrr (
  IN  UINT32                 MtrrNumber,
  IN  EFI_PHYSICAL_ADDRESS   MemoryAddress,
  IN  UINT64                 MemoryLength,
  IN  EFI_MEMORY_CACHE_TYPE  MemoryCacheType,
  IN  UINT64                 ValidMtrrAddressMask
  )
{
  UINT64  TempQword;
  UINT64  OldMtrr;

  if (MemoryLength == 0) {
    return;
  }

  EfiDisableCacheMtrr (&OldMtrr);

  //
  // MTRR Physical Base
  //
  TempQword = (MemoryAddress & ValidMtrrAddressMask) | MemoryCacheType;
  AsmWriteMsr64 (MtrrNumber, TempQword);

  //
  // MTRR Physical Mask
  //
  TempQword = ~(MemoryLength - 1);
  AsmWriteMsr64 (MtrrNumber + 1, (TempQword & ValidMtrrAddressMask) | B_EFI_MSR_CACHE_MTRR_VALID);

  EfiRecoverCacheMtrr (TRUE, OldMtrr);
}

/**
  Calculate the maximum value which is a power of 2, but less the MemoryLength.

  @param[in]  MemoryAddress       Memory address.
  @param[in]  MemoryLength        The number to pass in.

  @return The maximum value which is align to power of 2 and less the MemoryLength

**/
UINT64
Power2MaxMemory (
  IN UINT64  MemoryAddress,
  IN UINT64  MemoryLength
  )
{
  UINT64  Result;

  if (MemoryLength == 0) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Compute initial power of 2 size to return
  //
  Result = GetPowerOfTwo64 (MemoryLength);

  //
  // Special case base of 0 as all ranges are valid
  //
  if (MemoryAddress == 0) {
    return Result;
  }

  //
  // Loop till a value that can be mapped to this base address is found
  //
  while (CheckMtrrAlignment (MemoryAddress, Result) != 0) {
    //
    // Need to try the next smaller power of 2
    //
    Result = RShiftU64 (Result, 1);
  }

  return Result;
}

/**
  Return MTRR alignment requirement for base address and size.

  @param[in]  BaseAddress     Base address.
  @param[in]  Size            Size.

  @retval Zero      Aligned.
  @retval Non-Zero  Not aligned.

**/
UINT32
CheckMtrrAlignment (
  IN  UINT64  BaseAddress,
  IN  UINT64  Size
  )
{
  UINT32  ShiftedBase;
  UINT32  ShiftedSize;

  //
  // Shift base and size right 12 bits to allow for larger memory sizes.  The
  // MTRRs do not use the first 12 bits so this is safe for now.  Only supports
  // up to 52 bits of physical address space.
  //
  ShiftedBase = (UINT32)RShiftU64 (BaseAddress, 12);
  ShiftedSize = (UINT32)RShiftU64 (Size, 12);

  //
  // Return the results to the caller of the MOD
  //
  return ShiftedBase % ShiftedSize;
}

/**
  Programs fixed MTRRs registers.

  @param[in]  MemoryCacheType  The memory type to set.
  @param[in]  Base             The base address of memory range.
  @param[in]  Length           The length of memory range.

  @retval RETURN_SUCCESS      The cache type was updated successfully
  @retval RETURN_UNSUPPORTED  The requested range or cache type was invalid
                              for the fixed MTRRs.

**/
EFI_STATUS
ProgramFixedMtrr (
  IN  EFI_MEMORY_CACHE_TYPE  MemoryCacheType,
  IN  UINT64                 *Base,
  IN  UINT64                 *Len
  )
{
  UINT32  MsrNum;
  UINT32  ByteShift;
  UINT64  TempQword;
  UINT64  OrMask;
  UINT64  ClearMask;

  TempQword = 0;
  OrMask    =  0;
  ClearMask = 0;

  for (MsrNum = 0; MsrNum < V_EFI_FIXED_MTRR_NUMBER; MsrNum++) {
    if ((*Base >= mFixedMtrrTable[MsrNum].BaseAddress) &&
        (*Base < (mFixedMtrrTable[MsrNum].BaseAddress + 8 * mFixedMtrrTable[MsrNum].Length)))
    {
      break;
    }
  }

  if (MsrNum == V_EFI_FIXED_MTRR_NUMBER ) {
    return EFI_DEVICE_ERROR;
  }

  //
  // We found the fixed MTRR to be programmed
  //
  for (ByteShift = 0; ByteShift < 8; ByteShift++) {
    if ( *Base == (mFixedMtrrTable[MsrNum].BaseAddress + ByteShift * mFixedMtrrTable[MsrNum].Length)) {
      break;
    }
  }

  if (ByteShift == 8 ) {
    return EFI_DEVICE_ERROR;
  }

  for ( ; ((ByteShift < 8) && (*Len >= mFixedMtrrTable[MsrNum].Length)); ByteShift++) {
    OrMask    |= LShiftU64 ((UINT64)MemoryCacheType, (UINT32)(ByteShift* 8));
    ClearMask |= LShiftU64 ((UINT64)0xFF, (UINT32)(ByteShift * 8));
    *Len      -= mFixedMtrrTable[MsrNum].Length;
    *Base     += mFixedMtrrTable[MsrNum].Length;
  }

  TempQword = (AsmReadMsr64 (mFixedMtrrTable[MsrNum].Msr) & (~ClearMask)) | OrMask;
  AsmWriteMsr64 (mFixedMtrrTable[MsrNum].Msr, TempQword);

  return EFI_SUCCESS;
}

/**
  Check if there is a valid variable MTRR that overlaps the given range.

  @param[in]  Start  Base Address of the range to check.
  @param[in]  End    End address of the range to check.

  @retval TRUE   Mtrr overlap.
  @retval FALSE  Mtrr not overlap.
**/
BOOLEAN
CheckMtrrOverlap (
  IN  EFI_PHYSICAL_ADDRESS  Start,
  IN  EFI_PHYSICAL_ADDRESS  End
  )
{
  return FALSE;
}

/**
  Given the memory range and cache type, programs the MTRRs.

  @param[in] MemoryAddress           Base Address of Memory to program MTRR.
  @param[in] MemoryLength            Length of Memory to program MTRR.
  @param[in] MemoryCacheType         Cache Type.

  @retval EFI_SUCCESS            Mtrr are set successfully.
  @retval EFI_LOAD_ERROR         No empty MTRRs to use.
  @retval EFI_INVALID_PARAMETER  The input parameter is not valid.
  @retval others                 An error occurs when setting MTTR.

**/
EFI_STATUS
EFIAPI
SetCacheAttributes (
  IN  EFI_PHYSICAL_ADDRESS   MemoryAddress,
  IN  UINT64                 MemoryLength,
  IN  EFI_MEMORY_CACHE_TYPE  MemoryCacheType
  )
{
  EFI_STATUS             Status;
  UINT32                 MsrNum, MsrNumEnd;
  UINT64                 TempQword;
  UINT32                 LastVariableMtrrForBios;
  UINT64                 OldMtrr;
  UINT32                 UsedMsrNum;
  EFI_MEMORY_CACHE_TYPE  UsedMemoryCacheType;
  UINT64                 ValidMtrrAddressMask;
  UINT32                 Cpuid_RegEax;

  AsmCpuid (CPUID_EXTENDED_FUNCTION, &Cpuid_RegEax, NULL, NULL, NULL);
  if (Cpuid_RegEax >= CPUID_VIR_PHY_ADDRESS_SIZE) {
    AsmCpuid (CPUID_VIR_PHY_ADDRESS_SIZE, &Cpuid_RegEax, NULL, NULL, NULL);
    ValidMtrrAddressMask = (LShiftU64 ((UINT64)1, (Cpuid_RegEax & 0xFF)) - 1) & (~(UINT64)0x0FFF);
  } else {
    ValidMtrrAddressMask = (LShiftU64 ((UINT64)1, 36) - 1) & (~(UINT64)0x0FFF);
  }

  //
  // Check for invalid parameter
  //
  if (((MemoryAddress & ~ValidMtrrAddressMask) != 0) || ((MemoryLength & ~ValidMtrrAddressMask) != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if (MemoryLength == 0) {
    return EFI_INVALID_PARAMETER;
  }

  switch (MemoryCacheType) {
    case EFI_CACHE_UNCACHEABLE:
    case EFI_CACHE_WRITECOMBINING:
    case EFI_CACHE_WRITETHROUGH:
    case EFI_CACHE_WRITEPROTECTED:
    case EFI_CACHE_WRITEBACK:
      break;

    default:
      return EFI_INVALID_PARAMETER;
  }

  //
  // Check if Fixed MTRR
  //
  if ((MemoryAddress + MemoryLength) <= (1 << 20)) {
    Status = EFI_SUCCESS;
    EfiDisableCacheMtrr (&OldMtrr);
    while ((MemoryLength > 0) && (Status == EFI_SUCCESS)) {
      Status = ProgramFixedMtrr (MemoryCacheType, &MemoryAddress, &MemoryLength);
    }

    EfiRecoverCacheMtrr (TRUE, OldMtrr);
    return Status;
  }

  //
  // Search if the range attribute has been set before
  //
  Status = SearchForExactMtrr (
             MemoryAddress,
             MemoryLength,
             ValidMtrrAddressMask,
             &UsedMsrNum,
             &UsedMemoryCacheType
             );

  if (!EFI_ERROR (Status)) {
    //
    // Compare if it has the same type as current setting
    //
    if (UsedMemoryCacheType == MemoryCacheType) {
      return EFI_SUCCESS;
    } else {
      //
      // Different type
      //

      //
      // Check if the set type is the same as Default Type
      //
      if (IsDefaultType (MemoryCacheType)) {
        //
        // Clear the MTRR
        //
        AsmWriteMsr64 (UsedMsrNum, 0);
        AsmWriteMsr64 (UsedMsrNum + 1, 0);

        return EFI_SUCCESS;
      } else {
        //
        // Modify the MTRR type
        //
        EfiProgramMtrr (
          UsedMsrNum,
          MemoryAddress,
          MemoryLength,
          MemoryCacheType,
          ValidMtrrAddressMask
          );
        return EFI_SUCCESS;
      }
    }
  }

 #if 0
  //
  // @bug - Need to create memory map so that when checking for overlap we
  //        can determine if an overlap exists based on all caching requests.
  //
  // Don't waste a variable MTRR if the caching attrib is same as default in MTRR_DEF_TYPE
  //
  if (MemoryCacheType == (AsmReadMsr64 (EFI_MSR_CACHE_IA32_MTRR_DEF_TYPE) & B_EFI_MSR_CACHE_MEMORY_TYPE)) {
    if (!CheckMtrrOverlap (MemoryAddress, MemoryAddress+MemoryLength-1)) {
      return EFI_SUCCESS;
    }
  }

 #endif

  //
  // Find first unused MTRR
  //
  MsrNumEnd = EFI_MSR_CACHE_VARIABLE_MTRR_BASE + (2 * (UINT32)(AsmReadMsr64 (EFI_MSR_IA32_MTRR_CAP) & B_EFI_MSR_IA32_MTRR_CAP_VARIABLE_SUPPORT));
  for (MsrNum = EFI_MSR_CACHE_VARIABLE_MTRR_BASE; MsrNum < MsrNumEnd; MsrNum += 2) {
    if ((AsmReadMsr64 (MsrNum+1) & B_EFI_MSR_CACHE_MTRR_VALID) == 0 ) {
      break;
    }
  }

  //
  // Reserve 1 MTRR pair for OS.
  //
  LastVariableMtrrForBios = MsrNumEnd - 1 - (EFI_CACHE_NUM_VAR_MTRR_PAIRS_FOR_OS * 2);
  if (MsrNum > LastVariableMtrrForBios) {
    return EFI_LOAD_ERROR;
  }

  //
  // Special case for 1 MB base address
  //
  if (MemoryAddress == BASE_1MB) {
    MemoryAddress = 0;
  }

  //
  // Program MTRRs
  //
  TempQword = MemoryLength;

  if (TempQword == Power2MaxMemory (MemoryAddress, TempQword)) {
    EfiProgramMtrr (
      MsrNum,
      MemoryAddress,
      MemoryLength,
      MemoryCacheType,
      ValidMtrrAddressMask
      );
  } else {
    //
    // Fill in MTRRs with values.  Direction can not be checked for this method
    // as we are using WB as the default cache type and only setting areas to UC.
    //
    do {
      //
      // Do boundary check so we don't go past last MTRR register
      // for BIOS use.  Leave one MTRR pair for OS use.
      //
      if (MsrNum > LastVariableMtrrForBios) {
        return EFI_LOAD_ERROR;
      }

      //
      // Set next power of 2 region
      //
      MemoryLength = Power2MaxMemory (MemoryAddress, TempQword);
      EfiProgramMtrr (
        MsrNum,
        MemoryAddress,
        MemoryLength,
        MemoryCacheType,
        ValidMtrrAddressMask
        );
      MemoryAddress += MemoryLength;
      TempQword     -= MemoryLength;
      MsrNum        += 2;
    } while (TempQword != 0);
  }

  return EFI_SUCCESS;
}

/**
 Reset all the MTRRs to a known state.

  @retval  EFI_SUCCESS All MTRRs have been reset successfully.

**/
EFI_STATUS
EFIAPI
ResetCacheAttributes (
  VOID
  )
{
  UINT32   MsrNum, MsrNumEnd;
  UINT16   Index;
  UINT64   OldMtrr;
  UINT64   CacheType;
  BOOLEAN  DisableCar;

  Index      = 0;
  DisableCar = TRUE;

  //
  // Determine default cache type
  //
  CacheType = EFI_CACHE_UNCACHEABLE;

  //
  // Set default cache type
  //
  AsmWriteMsr64 (EFI_MSR_CACHE_IA32_MTRR_DEF_TYPE, CacheType);

  //
  // Disable CAR
  //
  DisableCacheAsRam (DisableCar);

  EfiDisableCacheMtrr (&OldMtrr);

  //
  // Reset Fixed MTRRs
  //
  for (Index = 0; Index < V_EFI_FIXED_MTRR_NUMBER; Index++) {
    AsmWriteMsr64 (mFixedMtrrTable[Index].Msr, 0);
  }

  //
  // Reset Variable MTRRs
  //
  MsrNumEnd = EFI_MSR_CACHE_VARIABLE_MTRR_BASE + (2 * (UINT32)(AsmReadMsr64 (EFI_MSR_IA32_MTRR_CAP) & B_EFI_MSR_IA32_MTRR_CAP_VARIABLE_SUPPORT));
  for (MsrNum = EFI_MSR_CACHE_VARIABLE_MTRR_BASE; MsrNum < MsrNumEnd; MsrNum++) {
    AsmWriteMsr64 (MsrNum, 0);
  }

  //
  // Enable Fixed and Variable MTRRs
  //
  EfiRecoverCacheMtrr (TRUE, OldMtrr);

  return EFI_SUCCESS;
}

/**
  Search the memory cache type for specific memory from MTRR.

  @param[in]  MemoryAddress         the address of target memory
  @param[in]  MemoryLength          the length of target memory
  @param[in]  ValidMtrrAddressMask  the MTRR address mask
  @param[out] UsedMsrNum            the used MSR number
  @param[out] UsedMemoryCacheType   the cache type for the target memory

  @retval EFI_SUCCESS    The memory is found in MTRR and cache type is returned
  @retval EFI_NOT_FOUND  The memory is not found in MTRR

**/
EFI_STATUS
SearchForExactMtrr (
  IN  EFI_PHYSICAL_ADDRESS   MemoryAddress,
  IN  UINT64                 MemoryLength,
  IN  UINT64                 ValidMtrrAddressMask,
  OUT UINT32                 *UsedMsrNum,
  OUT EFI_MEMORY_CACHE_TYPE  *UsedMemoryCacheType
  )
{
  UINT32  MsrNum, MsrNumEnd;
  UINT64  TempQword;

  if (MemoryLength == 0) {
    return EFI_INVALID_PARAMETER;
  }

  MsrNumEnd = EFI_MSR_CACHE_VARIABLE_MTRR_BASE + (2 * (UINT32)(AsmReadMsr64 (EFI_MSR_IA32_MTRR_CAP) & B_EFI_MSR_IA32_MTRR_CAP_VARIABLE_SUPPORT));
  for (MsrNum = EFI_MSR_CACHE_VARIABLE_MTRR_BASE; MsrNum < MsrNumEnd; MsrNum += 2) {
    TempQword = AsmReadMsr64 (MsrNum+1);
    if ((TempQword & B_EFI_MSR_CACHE_MTRR_VALID) == 0) {
      continue;
    }

    if ((TempQword & ValidMtrrAddressMask) != ((~(MemoryLength - 1)) & ValidMtrrAddressMask)) {
      continue;
    }

    TempQword = AsmReadMsr64 (MsrNum);
    if ((TempQword & ValidMtrrAddressMask) != (MemoryAddress & ValidMtrrAddressMask)) {
      continue;
    }

    *UsedMemoryCacheType = (EFI_MEMORY_CACHE_TYPE)(TempQword & B_EFI_MSR_CACHE_MEMORY_TYPE);
    *UsedMsrNum          = MsrNum;

    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

/**
  Check if CacheType match current default setting.

  @param[in] MemoryCacheType  input cache type to be checked.

  @retval TRUE MemoryCacheType is default MTRR setting.
  @retval TRUE MemoryCacheType is NOT default MTRR setting.
**/
BOOLEAN
IsDefaultType (
  IN  EFI_MEMORY_CACHE_TYPE  MemoryCacheType
  )
{
  if ((AsmReadMsr64 (EFI_MSR_CACHE_IA32_MTRR_DEF_TYPE) & B_EFI_MSR_CACHE_MEMORY_TYPE) != MemoryCacheType) {
    return FALSE;
  }

  return TRUE;
}
