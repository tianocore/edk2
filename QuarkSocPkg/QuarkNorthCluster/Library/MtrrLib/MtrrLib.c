/** @file
MTRR setting library

Copyright (c) 2008 - 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>

#include <Library/MtrrLib.h>
#include <Library/BaseLib.h>
#include <Library/CpuLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/QNCAccessLib.h>

#define QUARK_SOC_CPUID_FAMILY_MODEL_STEPPING         0x590

#define CACHE_MTRR_ENABLED                            0x800
#define CACHE_FIXED_MTRR_ENABLED                      0x400
#define IA32_MTRR_CAP_VCNT_MASK                       0xFF

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
  { QUARK_NC_HOST_BRIDGE_MTRR_FIX64K_00000, 0,       SIZE_64KB },
  { QUARK_NC_HOST_BRIDGE_MTRR_FIX16K_80000, 0x80000, SIZE_16KB },
  { QUARK_NC_HOST_BRIDGE_MTRR_FIX16K_A0000, 0xA0000, SIZE_16KB },
  { QUARK_NC_HOST_BRIDGE_MTRR_FIX4K_C0000,  0xC0000, SIZE_4KB  },
  { QUARK_NC_HOST_BRIDGE_MTRR_FIX4K_C8000,  0xC8000, SIZE_4KB  },
  { QUARK_NC_HOST_BRIDGE_MTRR_FIX4K_D0000,  0xD0000, SIZE_4KB  },
  { QUARK_NC_HOST_BRIDGE_MTRR_FIX4K_D8000,  0xD8000, SIZE_4KB  },
  { QUARK_NC_HOST_BRIDGE_MTRR_FIX4K_E0000,  0xE0000, SIZE_4KB  },
  { QUARK_NC_HOST_BRIDGE_MTRR_FIX4K_E8000,  0xE8000, SIZE_4KB  },
  { QUARK_NC_HOST_BRIDGE_MTRR_FIX4K_F0000,  0xF0000, SIZE_4KB  },
  { QUARK_NC_HOST_BRIDGE_MTRR_FIX4K_F8000,  0xF8000, SIZE_4KB  }
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

UINT64
MtrrRegisterRead (
  IN  UINT32  MtrrRegister
  )
{
  UINT64  Result;

  Result = (UINT64)QNCPortRead (QUARK_NC_HOST_BRIDGE_SB_PORT_ID, MtrrRegister);
  if (MtrrRegister >= QUARK_NC_HOST_BRIDGE_MTRR_FIX64K_00000 && MtrrRegister <= QUARK_NC_HOST_BRIDGE_MTRR_FIX4K_F8000) {
    Result = Result | LShiftU64 ((UINT64)QNCPortRead (QUARK_NC_HOST_BRIDGE_SB_PORT_ID, MtrrRegister + 1), 32);
  }
  return Result;
}

UINT64
MtrrRegisterWrite (
  IN  UINT32  MtrrRegister,
  IN  UINT64  Value
  )
{
  QNCPortWrite (QUARK_NC_HOST_BRIDGE_SB_PORT_ID, MtrrRegister, (UINT32)Value);
  if (MtrrRegister >= QUARK_NC_HOST_BRIDGE_MTRR_FIX64K_00000 && MtrrRegister <= QUARK_NC_HOST_BRIDGE_MTRR_FIX4K_F8000) {
    QNCPortWrite (QUARK_NC_HOST_BRIDGE_SB_PORT_ID, MtrrRegister + 1, (UINT32)RShiftU64 (Value, 32));
  }
  return Value;
}

UINT64
MtrrRegisterBitFieldWrite (
  IN  UINT32  MtrrRegister,
  IN  UINTN   StartBit,
  IN  UINTN   EndBit,
  IN  UINT64  Value
  )
{
  return MtrrRegisterWrite (
           MtrrRegister,
           BitFieldWrite64 (
             MtrrRegisterRead (MtrrRegister),
             StartBit,
             EndBit,
             Value
             )
           );
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
  UINT32  VariableMtrrCount;

  VariableMtrrCount = (UINT32)(MtrrRegisterRead (QUARK_NC_HOST_BRIDGE_IA32_MTRR_CAP) & IA32_MTRR_CAP_VCNT_MASK);
  ASSERT (VariableMtrrCount <= MTRR_NUMBER_OF_VARIABLE_MTRR);
  return VariableMtrrCount;
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
  if (MtrrSetting == NULL) {
    return (MTRR_MEMORY_CACHE_TYPE) (MtrrRegisterRead (QUARK_NC_HOST_BRIDGE_IA32_MTRR_DEF_TYPE) & 0x7);
  } else {
    return (MTRR_MEMORY_CACHE_TYPE) (MtrrSetting->MtrrDefType & 0x7);
  }
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
  // Disable MTRRs
  //
  MtrrRegisterBitFieldWrite (QUARK_NC_HOST_BRIDGE_IA32_MTRR_DEF_TYPE, 10, 11, 0);
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
  MtrrRegisterBitFieldWrite (QUARK_NC_HOST_BRIDGE_IA32_MTRR_DEF_TYPE, 10, 11, 3);

  PostMtrrChangeEnableCache (MtrrContext);
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
        MtrrRegisterRead (mMtrrLibFixedMtrrTable[Index].Msr);
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
        MtrrRegisterRead (QUARK_NC_HOST_BRIDGE_IA32_MTRR_PHYSBASE0 + (Index << 1));
      VariableSettings->Mtrr[Index].Mask =
        MtrrRegisterRead (QUARK_NC_HOST_BRIDGE_IA32_MTRR_PHYSBASE0 + (Index << 1) + 1);
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

  @param[in]      MemoryCacheType  The memory type to set.
  @param[in, out] Base             The base address of memory range.
  @param[in, out] Length           The length of memory range.
  @param[out]     ReturnMsrNum     The index of the fixed MTRR MSR to program.
  @param[out]     ReturnClearMask  The bits to clear in the fixed MTRR MSR.
  @param[out]     ReturnOrMask     The bits to set in the fixed MTRR MSR.

  @retval RETURN_SUCCESS      The cache type was updated successfully
  @retval RETURN_UNSUPPORTED  The requested range or cache type was invalid
                              for the fixed MTRRs.

**/
RETURN_STATUS
ProgramFixedMtrr (
  IN     UINT64               MemoryCacheType,
  IN OUT UINT64               *Base,
  IN OUT UINT64               *Length,
  OUT    UINT32               *ReturnMsrNum,
  OUT    UINT64               *ReturnClearMask,
  OUT    UINT64               *ReturnOrMask
  )
{
  UINT32  MsrNum;
  UINT32  ByteShift;
  UINT64  OrMask;
  UINT64  ClearMask;

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

  *ReturnMsrNum    = MsrNum;
  *ReturnClearMask = ClearMask;
  *ReturnOrMask    = OrMask;

  return RETURN_SUCCESS;
}


/**
  Worker function gets the attribute of variable MTRRs.

  This function shadows the content of variable MTRRs into an
  internal array: VariableMtrr.

  @param[in]   VariableSettings           The variable MTRR values to shadow
  @param[in]   FirmwareVariableMtrrCount  The number of variable MTRRs available to firmware
  @param[in]   MtrrValidBitsMask          The mask for the valid bit of the MTRR
  @param[in]   MtrrValidAddressMask       The valid address mask for MTRR
  @param[out]  VariableMtrr               The array to shadow variable MTRRs content

  @return                       The return value of this parameter indicates the
                                number of MTRRs which has been used.

**/
UINT32
MtrrGetMemoryAttributeInVariableMtrrWorker (
  IN  MTRR_VARIABLE_SETTINGS  *VariableSettings,
  IN  UINTN                   FirmwareVariableMtrrCount,
  IN  UINT64                  MtrrValidBitsMask,
  IN  UINT64                  MtrrValidAddressMask,
  OUT VARIABLE_MTRR           *VariableMtrr
  )
{
  UINTN   Index;
  UINT32  UsedMtrr;

  ZeroMem (VariableMtrr, sizeof (VARIABLE_MTRR) * MTRR_NUMBER_OF_VARIABLE_MTRR);
  for (Index = 0, UsedMtrr = 0; Index < FirmwareVariableMtrrCount; Index++) {
    if ((VariableSettings->Mtrr[Index].Mask & CACHE_MTRR_ENABLED) != 0) {
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
  Checks overlap between given memory range and MTRRs.

  @param[in]  FirmwareVariableMtrrCount  The number of variable MTRRs available
                                         to firmware.
  @param[in]  Start                      The start address of memory range.
  @param[in]  End                        The end address of memory range.
  @param[in]  VariableMtrr               The array to shadow variable MTRRs content

  @retval TRUE             Overlap exists.
  @retval FALSE            No overlap.

**/
BOOLEAN
CheckMemoryAttributeOverlap (
  IN UINTN             FirmwareVariableMtrrCount,
  IN PHYSICAL_ADDRESS  Start,
  IN PHYSICAL_ADDRESS  End,
  IN VARIABLE_MTRR     *VariableMtrr
  )
{
  UINT32  Index;

  for (Index = 0; Index < FirmwareVariableMtrrCount; Index++) {
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

  @param[in]   Index         The index of the array VariableMtrr to be invalidated
  @param[in]   VariableMtrr  The array to shadow variable MTRRs content
  @param[out]  UsedMtrr      The number of MTRRs which has already been used

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
  Combines memory attributes.

  If overlap exists between given memory range and MTRRs, try to combine them.

  @param[in]       FirmwareVariableMtrrCount  The number of variable MTRRs
                                              available to firmware.
  @param[in]       Attributes                 The memory type to set.
  @param[in, out]  Base                       The base address of memory range.
  @param[in, out]  Length                     The length of memory range.
  @param[in]       VariableMtrr               The array to shadow variable MTRRs content
  @param[in, out]  UsedMtrr                   The number of MTRRs which has already been used
  @param[out]      OverwriteExistingMtrr      Returns whether an existing MTRR was used

  @retval EFI_SUCCESS            Memory region successfully combined.
  @retval EFI_ACCESS_DENIED      Memory region cannot be combined.

**/
RETURN_STATUS
CombineMemoryAttribute (
  IN     UINT32             FirmwareVariableMtrrCount,
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
  BOOLEAN CoveredByExistingMtrr;

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
      // if the MTRR range contain the request range, set a flag, then continue to
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
  Calculates the maximum value which is a power of 2, but less the MemoryLength.

  @param[in]  MemoryLength        The number to pass in.

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
  Determines the MTRR numbers used to program a memory range.

  This function first checks the alignment of the base address.
  If the alignment of the base address <= Length, cover the memory range
 (BaseAddress, alignment) by a MTRR, then BaseAddress += alignment and
  Length -= alignment. Repeat the step until alignment > Length.

  Then this function determines which direction of programming the variable
  MTRRs for the remaining length will use fewer MTRRs.

  @param[in]  BaseAddress Length of Memory to program MTRR
  @param[in]  Length      Length of Memory to program MTRR
  @param[in]  MtrrNumber  Pointer to the number of necessary MTRRs

  @retval TRUE        Positive direction is better.
          FALSE       Negative direction is better.

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

  @param[in, out]  VariableSettings   Variable MTRR settings
  @param[in]       VariableMtrrCount  Number of variable MTRRs
  @param[in, out]  VariableMtrr       Shadow of variable MTRR contents

**/
VOID
InvalidateMtrr (
  IN OUT MTRR_VARIABLE_SETTINGS  *VariableSettings,
  IN     UINTN                   VariableMtrrCount,
  IN OUT VARIABLE_MTRR           *VariableMtrr
  )
{
  UINTN         Index;

  for (Index = 0; Index < VariableMtrrCount; Index++) {
    if (!VariableMtrr[Index].Valid && VariableMtrr[Index].Used) {
       VariableSettings->Mtrr[Index].Base = 0;
       VariableSettings->Mtrr[Index].Mask = 0;
       VariableMtrr[Index].Used = FALSE;
    }
  }
}


/**
  Programs variable MTRRs

  This function programs variable MTRRs

  @param[in, out]  VariableSettings      Variable MTRR settings.
  @param[in]       MtrrNumber            Index of MTRR to program.
  @param[in]       BaseAddress           Base address of memory region.
  @param[in]       Length                Length of memory region.
  @param[in]       MemoryCacheType       Memory type to set.
  @param[in]       MtrrValidAddressMask  The valid address mask for MTRR

**/
VOID
ProgramVariableMtrr (
  IN OUT MTRR_VARIABLE_SETTINGS  *VariableSettings,
  IN     UINTN                   MtrrNumber,
  IN     PHYSICAL_ADDRESS        BaseAddress,
  IN     UINT64                  Length,
  IN     UINT64                  MemoryCacheType,
  IN     UINT64                  MtrrValidAddressMask
  )
{
  UINT64        TempQword;

  //
  // MTRR Physical Base
  //
  TempQword = (BaseAddress & MtrrValidAddressMask) | MemoryCacheType;
  VariableSettings->Mtrr[MtrrNumber].Base = TempQword;

  //
  // MTRR Physical Mask
  //
  TempQword = ~(Length - 1);
  VariableSettings->Mtrr[MtrrNumber].Mask = (TempQword & MtrrValidAddressMask) | CACHE_MTRR_ENABLED;
}


/**
  Converts the Memory attribute value to MTRR_MEMORY_CACHE_TYPE.

  If MtrrSetting is not NULL, gets the default memory attribute from input
  MTRR settings buffer.
  If MtrrSetting is NULL, gets the default memory attribute from MSR.

  @param[in]  MtrrSetting        A buffer holding all MTRRs content.
  @param[in]  MtrrType           MTRR memory type

  @return The enum item in MTRR_MEMORY_CACHE_TYPE

**/
MTRR_MEMORY_CACHE_TYPE
GetMemoryCacheTypeFromMtrrType (
  IN MTRR_SETTINGS         *MtrrSetting,
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
    // no MTRR covers the range
    //
    return MtrrGetDefaultMemoryTypeWorker (MtrrSetting);
  }
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
  UINT32  RegEax;
  UINT8   PhysicalAddressBits;

  AsmCpuid (0x80000000, &RegEax, NULL, NULL, NULL);

  if (RegEax >= 0x80000008) {
    AsmCpuid (0x80000008, &RegEax, NULL, NULL, NULL);

    PhysicalAddressBits = (UINT8) RegEax;
  } else {
    PhysicalAddressBits = 36;
  }

  *MtrrValidBitsMask    = LShiftU64 (1, PhysicalAddressBits) - 1;
  *MtrrValidAddressMask = *MtrrValidBitsMask & 0xfffffffffffff000ULL;
}


/**
  Determines the real attribute of a memory range.

  This function is to arbitrate the real attribute of the memory when
  there are 2 MTRRs covers the same memory range.  For further details,
  please refer the IA32 Software Developer's Manual, Volume 3,
  Section 10.11.4.1.

  @param[in]  MtrrType1    The first kind of Memory type
  @param[in]  MtrrType2    The second kind of memory type

**/
UINT64
MtrrPrecedence (
  IN UINT64    MtrrType1,
  IN UINT64    MtrrType2
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
  MTRR_VARIABLE_SETTINGS  VariableSettings;

  //
  // Check if MTRR is enabled, if not, return UC as attribute
  //
  if (MtrrSetting == NULL) {
    TempQword = MtrrRegisterRead (QUARK_NC_HOST_BRIDGE_IA32_MTRR_DEF_TYPE);
  } else {
    TempQword = MtrrSetting->MtrrDefType;
  }
  MtrrType = MTRR_CACHE_INVALID_TYPE;

  if ((TempQword & CACHE_MTRR_ENABLED) == 0) {
    return CacheUncacheable;
  }

  //
  // If address is less than 1M, then try to go through the fixed MTRR
  //
  if (Address < BASE_1MB) {
    if ((TempQword & CACHE_FIXED_MTRR_ENABLED) != 0) {
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
           if (MtrrSetting == NULL) {
             TempQword = MtrrRegisterRead (mMtrrLibFixedMtrrTable[Index].Msr);
           } else {
             TempQword = MtrrSetting->Fixed.Mtrr[Index];
           }
           MtrrType =  RShiftU64 (TempQword, SubIndex * 8) & 0xFF;
           return GetMemoryCacheTypeFromMtrrType (MtrrSetting, MtrrType);
         }
      }
    }
  }
  MtrrLibInitializeMtrrMask(&MtrrValidBitsMask, &MtrrValidAddressMask);

  MtrrGetVariableMtrrWorker (
    MtrrSetting,
    GetVariableMtrrCountWorker (),
    &VariableSettings
    );

  MtrrGetMemoryAttributeInVariableMtrrWorker (
           &VariableSettings,
           GetFirmwareVariableMtrrCountWorker (),
           MtrrValidBitsMask,
           MtrrValidAddressMask,
           VariableMtrr
           );

  //
  // Go through the variable MTRR
  //
  VariableMtrrCount = GetVariableMtrrCountWorker ();
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
  CacheType = GetMemoryCacheTypeFromMtrrType (MtrrSetting, MtrrType);

  return CacheType;
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

  If MtrrSetting is not NULL, print MTRR settings from from input MTRR
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
  Worker function attempts to set the attributes for a memory range.

  If MtrrSettings is not NULL, set the attributes into the input MTRR
  settings buffer.
  If MtrrSettings is NULL, set the attributes into MTRRs registers.

  @param[in, out]  MtrrSetting       A buffer holding all MTRRs content.
  @param[in]       BaseAddress       The physical address that is the start
                                     address of a memory region.
  @param[in]       Length            The size in bytes of the memory region.
  @param[in]       Attribute         The bit mask of attributes to set for the
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
MtrrSetMemoryAttributeWorker (
  IN OUT MTRR_SETTINGS           *MtrrSetting,
  IN PHYSICAL_ADDRESS            BaseAddress,
  IN UINT64                      Length,
  IN MTRR_MEMORY_CACHE_TYPE      Attribute
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
  MTRR_CONTEXT              MtrrContext;
  BOOLEAN                   MtrrContextValid;
  BOOLEAN                   FixedSettingsValid[MTRR_NUMBER_OF_FIXED_MTRR];
  BOOLEAN                   FixedSettingsModified[MTRR_NUMBER_OF_FIXED_MTRR];
  MTRR_FIXED_SETTINGS       WorkingFixedSettings;
  UINT32                    VariableMtrrCount;
  MTRR_VARIABLE_SETTINGS    OriginalVariableSettings;
  BOOLEAN                   ProgramVariableSettings;
  MTRR_VARIABLE_SETTINGS    WorkingVariableSettings;
  UINT32                    Index;
  UINT64                    ClearMask;
  UINT64                    OrMask;
  UINT64                    NewValue;
  MTRR_VARIABLE_SETTINGS    *VariableSettings;

  MtrrContextValid  = FALSE;
  VariableMtrrCount = 0;
  ZeroMem (&WorkingFixedSettings, sizeof (WorkingFixedSettings));
  for (Index = 0; Index < MTRR_NUMBER_OF_FIXED_MTRR; Index++) {
    FixedSettingsValid[Index]    = FALSE;
    FixedSettingsModified[Index] = FALSE;
  }
  ProgramVariableSettings = FALSE;

  if (!IsMtrrSupported ()) {
    Status = RETURN_UNSUPPORTED;
    goto Done;
  }

  MtrrLibInitializeMtrrMask (&MtrrValidBitsMask, &MtrrValidAddressMask);

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
  if (BaseAddress < BASE_1MB) {
    while ((BaseAddress < BASE_1MB) && (Length > 0) && Status == RETURN_SUCCESS) {
      Status = ProgramFixedMtrr (MemoryType, &BaseAddress, &Length, &MsrNum, &ClearMask, &OrMask);
      if (RETURN_ERROR (Status)) {
        goto Done;
      }
      if (MtrrSetting != NULL) {
        MtrrSetting->Fixed.Mtrr[MsrNum] = (MtrrSetting->Fixed.Mtrr[MsrNum] & ~ClearMask) | OrMask;
        MtrrSetting->MtrrDefType |= CACHE_FIXED_MTRR_ENABLED;
      } else {
        if (!FixedSettingsValid[MsrNum]) {
          WorkingFixedSettings.Mtrr[MsrNum] = MtrrRegisterRead (mMtrrLibFixedMtrrTable[MsrNum].Msr);
          FixedSettingsValid[MsrNum] = TRUE;
        }
        NewValue = (WorkingFixedSettings.Mtrr[MsrNum] & ~ClearMask) | OrMask;
        if (WorkingFixedSettings.Mtrr[MsrNum] != NewValue) {
          WorkingFixedSettings.Mtrr[MsrNum] = NewValue;
          FixedSettingsModified[MsrNum] = TRUE;
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
  // Since memory ranges below 1MB will be overridden by the fixed MTRRs,
  // we can set the base to 0 to save variable MTRRs.
  //
  if (BaseAddress == BASE_1MB) {
    BaseAddress = 0;
    Length += SIZE_1MB;
  }

  //
  // Read all variable MTRRs
  //
  VariableMtrrCount = GetVariableMtrrCountWorker ();
  FirmwareVariableMtrrCount = GetFirmwareVariableMtrrCountWorker ();
  if (MtrrSetting != NULL) {
    VariableSettings = &MtrrSetting->Variables;
  } else {
    MtrrGetVariableMtrrWorker (NULL, VariableMtrrCount, &OriginalVariableSettings);
    CopyMem (&WorkingVariableSettings, &OriginalVariableSettings, sizeof (WorkingVariableSettings));
    ProgramVariableSettings = TRUE;
    VariableSettings = &WorkingVariableSettings;
  }

  //
  // Check for overlap
  //
  UsedMtrr = MtrrGetMemoryAttributeInVariableMtrrWorker (
               VariableSettings,
               FirmwareVariableMtrrCount,
               MtrrValidBitsMask,
               MtrrValidAddressMask,
               VariableMtrr
               );
  OverLap = CheckMemoryAttributeOverlap (
              FirmwareVariableMtrrCount,
              BaseAddress,
              BaseAddress + Length - 1,
              VariableMtrr
              );
  if (OverLap) {
    Status = CombineMemoryAttribute (
               FirmwareVariableMtrrCount,
               MemoryType,
               &BaseAddress,
               &Length,
               VariableMtrr,
               &UsedMtrr,
               &OverwriteExistingMtrr
               );
    if (RETURN_ERROR (Status)) {
      goto Done;
    }

    if (Length == 0) {
      //
      // Combined successfully, invalidate the now-unused MTRRs
      //
      InvalidateMtrr (VariableSettings, VariableMtrrCount, VariableMtrr);
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
    InvalidateMtrr (VariableSettings, VariableMtrrCount, VariableMtrr);
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
  InvalidateMtrr (VariableSettings, VariableMtrrCount, VariableMtrr);

  //
  // Find first unused MTRR
  //
  for (MsrNum = 0; MsrNum < VariableMtrrCount; MsrNum++) {
    if ((VariableSettings->Mtrr[MsrNum].Mask & CACHE_MTRR_ENABLED) == 0) {
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
      for (; MsrNum < VariableMtrrCount; MsrNum++) {
        if ((VariableSettings->Mtrr[MsrNum].Mask & CACHE_MTRR_ENABLED) == 0) {
          break;
        }
      }

      ProgramVariableMtrr (
        VariableSettings,
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
    for (; MsrNum < VariableMtrrCount; MsrNum++) {
      if ((VariableSettings->Mtrr[MsrNum].Mask & CACHE_MTRR_ENABLED) == 0) {
        break;
      }
    }

    ProgramVariableMtrr (
      VariableSettings,
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
    for (; MsrNum < VariableMtrrCount; MsrNum++) {
      if ((VariableSettings->Mtrr[MsrNum].Mask & CACHE_MTRR_ENABLED) == 0) {
        break;
      }
    }

    Length = Power2MaxMemory (TempQword);
    if (!Positive) {
      BaseAddress -= Length;
    }

    ProgramVariableMtrr (
      VariableSettings,
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

  //
  // Write fixed MTRRs that have been modified
  //
  for (Index = 0; Index < MTRR_NUMBER_OF_FIXED_MTRR; Index++) {
    if (FixedSettingsModified[Index]) {
      if (!MtrrContextValid) {
        PreMtrrChange (&MtrrContext);
        MtrrContextValid = TRUE;
      }
      MtrrRegisterWrite (
        mMtrrLibFixedMtrrTable[Index].Msr,
        WorkingFixedSettings.Mtrr[Index]
        );
    }
  }

  //
  // Write variable MTRRs
  //
  if (ProgramVariableSettings) {
    for (Index = 0; Index < VariableMtrrCount; Index++) {
      if (WorkingVariableSettings.Mtrr[Index].Base != OriginalVariableSettings.Mtrr[Index].Base ||
          WorkingVariableSettings.Mtrr[Index].Mask != OriginalVariableSettings.Mtrr[Index].Mask    ) {
        if (!MtrrContextValid) {
          PreMtrrChange (&MtrrContext);
          MtrrContextValid = TRUE;
        }
        MtrrRegisterWrite (
          QUARK_NC_HOST_BRIDGE_IA32_MTRR_PHYSBASE0 + (Index << 1),
          WorkingVariableSettings.Mtrr[Index].Base
          );
        MtrrRegisterWrite (
          QUARK_NC_HOST_BRIDGE_IA32_MTRR_PHYSBASE0 + (Index << 1) + 1,
          WorkingVariableSettings.Mtrr[Index].Mask
          );
      }
    }
  }
  if (MtrrContextValid) {
    PostMtrrChange (&MtrrContext);
  }

  DEBUG((DEBUG_CACHE, "  Status = %r\n", Status));
  if (!RETURN_ERROR (Status)) {
    if (MtrrSetting != NULL) {
      MtrrSetting->MtrrDefType |= CACHE_MTRR_ENABLED;
    }
    MtrrDebugPrintAllMtrrsWorker (MtrrSetting);
  }

  return Status;
}

/**
  This function attempts to set the attributes for a memory range.

  @param[in]  BaseAddress        The physical address that is the start
                                 address of a memory region.
  @param[in]  Length             The size in bytes of the memory region.
  @param[in]  Attributes         The bit mask of attributes to set for the
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
  DEBUG((DEBUG_CACHE, "MtrrSetMemoryAttribute() %a:%016lx-%016lx\n", mMtrrMemoryCacheTypeShortName[Attribute], BaseAddress, Length));
  return MtrrSetMemoryAttributeWorker (
           NULL,
           BaseAddress,
           Length,
           Attribute
           );
}

/**
  This function attempts to set the attributes into MTRR setting buffer for a memory range.

  @param[in, out]  MtrrSetting  MTRR setting buffer to be set.
  @param[in]       BaseAddress  The physical address that is the start address
                                of a memory region.
  @param[in]       Length       The size in bytes of the memory region.
  @param[in]       Attribute    The bit mask of attributes to set for the
                                memory region.

  @retval RETURN_SUCCESS            The attributes were set for the memory region.
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
  DEBUG((DEBUG_CACHE, "MtrrSetMemoryAttributeMtrrSettings(%p) %a:%016lx-%016lx\n", MtrrSetting, mMtrrMemoryCacheTypeShortName[Attribute], BaseAddress, Length));
  return MtrrSetMemoryAttributeWorker (
           MtrrSetting,
           BaseAddress,
           Length,
           Attribute
           );
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
    MtrrRegisterWrite (
      QUARK_NC_HOST_BRIDGE_IA32_MTRR_PHYSBASE0 + (Index << 1),
      VariableSettings->Mtrr[Index].Base
      );
    MtrrRegisterWrite (
      QUARK_NC_HOST_BRIDGE_IA32_MTRR_PHYSBASE0 + (Index << 1) + 1,
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

  PreMtrrChange (&MtrrContext);
  MtrrSetVariableMtrrWorker (VariableSettings);
  PostMtrrChange (&MtrrContext);
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
     MtrrRegisterWrite (
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

  PreMtrrChange (&MtrrContext);
  MtrrSetFixedMtrrWorker (FixedSettings);
  PostMtrrChange (&MtrrContext);
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
  MtrrSetting->MtrrDefType = MtrrRegisterRead (QUARK_NC_HOST_BRIDGE_IA32_MTRR_DEF_TYPE);

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
  MtrrRegisterWrite (QUARK_NC_HOST_BRIDGE_IA32_MTRR_DEF_TYPE, MtrrSetting->MtrrDefType);

  PostMtrrChangeEnableCache (&MtrrContext);

  MtrrDebugPrintAllMtrrs ();

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
  UINT32  RegEax;

  //
  // Check CPUID(1).EAX[0..11] for Quark SoC
  //
  AsmCpuid (1, &RegEax, NULL, NULL, NULL);
  if ((RegEax & 0xfff) == QUARK_SOC_CPUID_FAMILY_MODEL_STEPPING) {
    return TRUE;
  }

  return FALSE;
}
