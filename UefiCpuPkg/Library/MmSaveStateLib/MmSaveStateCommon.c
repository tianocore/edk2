/** @file
  Provides common supporting function to access SMRAM Save State Map

  Copyright (c) 2010 - 2019, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "MmSaveState.h"

// Table used by MmSaveStateGetRegisterIndex() to convert an EFI_MM_SAVE_STATE_REGISTER
// value to an index into a table of type CPU_MM_SAVE_STATE_LOOKUP_ENTRY
CONST CPU_MM_SAVE_STATE_REGISTER_RANGE  mCpuRegisterRanges[] = {
  MM_REGISTER_RANGE (EFI_MM_SAVE_STATE_REGISTER_GDTBASE, EFI_MM_SAVE_STATE_REGISTER_LDTINFO),
  MM_REGISTER_RANGE (EFI_MM_SAVE_STATE_REGISTER_ES,      EFI_MM_SAVE_STATE_REGISTER_RIP),
  MM_REGISTER_RANGE (EFI_MM_SAVE_STATE_REGISTER_RFLAGS,  EFI_MM_SAVE_STATE_REGISTER_CR4),
  { (EFI_MM_SAVE_STATE_REGISTER)0,                       (EFI_MM_SAVE_STATE_REGISTER)0,      0}
};

extern CONST CPU_MM_SAVE_STATE_LOOKUP_ENTRY  mCpuWidthOffset[];

/**
  Read information from the CPU save state.

  @param  Register  Specifies the CPU register to read form the save state.
  @param  RegOffset Offset for the next register index.

  @retval 0   Register is not valid
  @retval >0  Index into mCpuWidthOffset[] associated with Register

**/
UINTN
MmSaveStateGetRegisterIndex (
  IN EFI_MM_SAVE_STATE_REGISTER  Register,
  IN UINTN                       RegOffset
  )
{
  UINTN  Index;
  UINTN  Offset;

  for (Index = 0, Offset = RegOffset; mCpuRegisterRanges[Index].Length != 0; Index++) {
    if ((Register >= mCpuRegisterRanges[Index].Start) && (Register <= mCpuRegisterRanges[Index].End)) {
      return Register - mCpuRegisterRanges[Index].Start + Offset;
    }

    Offset += mCpuRegisterRanges[Index].Length;
  }

  return 0;
}

/**
  Read a CPU Save State register on the target processor.

  This function abstracts the differences that whether the CPU Save State register is in the
  IA32 CPU Save State Map or X64 CPU Save State Map.

  This function supports reading a CPU Save State register in SMBase relocation handler.

  @param[in]  CpuIndex       Specifies the zero-based index of the CPU save state.
  @param[in]  RegisterIndex  Index into mCpuWidthOffset[] look up table.
  @param[in]  Width          The number of bytes to read from the CPU save state.
  @param[out] Buffer         Upon return, this holds the CPU register value read from the save state.

  @retval EFI_SUCCESS           The register was read from Save State.
  @retval EFI_NOT_FOUND         The register is not defined for the Save State of Processor.
  @retval EFI_INVALID_PARAMTER  This or Buffer is NULL.

**/
EFI_STATUS
MmSaveStateReadRegisterByIndex (
  IN UINTN  CpuIndex,
  IN UINTN  RegisterIndex,
  IN UINTN  Width,
  OUT VOID  *Buffer
  )
{
  if (RegisterIndex == 0) {
    return EFI_NOT_FOUND;
  }

  if (MmSaveStateGetRegisterLma () == EFI_MM_SAVE_STATE_REGISTER_LMA_32BIT) {
    //
    // If 32-bit mode width is zero, then the specified register can not be accessed
    //
    if (mCpuWidthOffset[RegisterIndex].Width32 == 0) {
      return EFI_NOT_FOUND;
    }

    //
    // If Width is bigger than the 32-bit mode width, then the specified register can not be accessed
    //
    if (Width > mCpuWidthOffset[RegisterIndex].Width32) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Write return buffer
    //
    ASSERT (gMmst->CpuSaveState[CpuIndex] != NULL);
    CopyMem (Buffer, (UINT8 *)gMmst->CpuSaveState[CpuIndex] + mCpuWidthOffset[RegisterIndex].Offset32, Width);
  } else {
    //
    // If 64-bit mode width is zero, then the specified register can not be accessed
    //
    if (mCpuWidthOffset[RegisterIndex].Width64 == 0) {
      return EFI_NOT_FOUND;
    }

    //
    // If Width is bigger than the 64-bit mode width, then the specified register can not be accessed
    //
    if (Width > mCpuWidthOffset[RegisterIndex].Width64) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Write lower 32-bits of return buffer
    //
    CopyMem (Buffer, (UINT8 *)gMmst->CpuSaveState[CpuIndex] + mCpuWidthOffset[RegisterIndex].Offset64Lo, MIN (4, Width));
    if (Width > 4) {
      //
      // Write upper 32-bits of return buffer
      //
      CopyMem ((UINT8 *)Buffer + 4, (UINT8 *)gMmst->CpuSaveState[CpuIndex] + mCpuWidthOffset[RegisterIndex].Offset64Hi, Width - 4);
    }
  }

  return EFI_SUCCESS;
}
