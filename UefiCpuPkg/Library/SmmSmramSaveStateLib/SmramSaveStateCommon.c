/** @file
  Provides common supporting function to access SMRAM Save State Map

  Copyright (c) 2010 - 2019, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmramSaveState.h"
#include <Library/BaseLib.h>
#include <Library/CpuLib.h>

#define CPUID_VERSION_INFO       0x01
#define CPUID_EXTENDED_FUNCTION  0x80000000
#define CPUID_EXTENDED_CPU_SIG   0x80000001
#define EFER_ADDRESS             0XC0000080ul

// Table used by SmramSaveStateGetRegisterIndex() to convert an EFI_SMM_SAVE_STATE_REGISTER
// value to an index into a table of type CPU_SMM_SAVE_STATE_LOOKUP_ENTRY
CONST CPU_SMM_SAVE_STATE_REGISTER_RANGE  mSmmSmramCpuRegisterRanges[] = {
  SMM_REGISTER_RANGE (EFI_SMM_SAVE_STATE_REGISTER_GDTBASE, EFI_SMM_SAVE_STATE_REGISTER_LDTINFO),
  SMM_REGISTER_RANGE (EFI_SMM_SAVE_STATE_REGISTER_ES,      EFI_SMM_SAVE_STATE_REGISTER_RIP),
  SMM_REGISTER_RANGE (EFI_SMM_SAVE_STATE_REGISTER_RFLAGS,  EFI_SMM_SAVE_STATE_REGISTER_CR4),
  { (EFI_SMM_SAVE_STATE_REGISTER)0,                        (EFI_SMM_SAVE_STATE_REGISTER)0,      0}
};

extern CONST CPU_SMM_SAVE_STATE_LOOKUP_ENTRY  mSmmSmramCpuWidthOffset[];

/**
  Read information from the CPU save state.

  @param  Register  Specifies the CPU register to read form the save state.

  @retval 0   Register is not valid
  @retval >0  Index into mSmmSmramCpuWidthOffset[] associated with Register

**/
UINTN
EFIAPI
SmramSaveStateGetRegisterIndex (
  IN EFI_SMM_SAVE_STATE_REGISTER  Register
  )
{
  UINTN  Index;
  UINTN  Offset;

  for (Index = 0, Offset = SMM_SAVE_STATE_REGISTER_MAX_INDEX; mSmmSmramCpuRegisterRanges[Index].Length != 0; Index++) {
    if ((Register >= mSmmSmramCpuRegisterRanges[Index].Start) && (Register <= mSmmSmramCpuRegisterRanges[Index].End)) {
      return Register - mSmmSmramCpuRegisterRanges[Index].Start + Offset;
    }

    Offset += mSmmSmramCpuRegisterRanges[Index].Length;
  }

  return 0;
}

/**
  Read a CPU Save State register on the target processor.

  This function abstracts the differences that whether the CPU Save State register is in the
  IA32 CPU Save State Map or X64 CPU Save State Map.

  This function supports reading a CPU Save State register in SMBase relocation handler.

  @param[in]  CpuIndex       Specifies the zero-based index of the CPU save state.
  @param[in]  RegisterIndex  Index into mSmmSmramCpuWidthOffset[] look up table.
  @param[in]  Width          The number of bytes to read from the CPU save state.
  @param[out] Buffer         Upon return, this holds the CPU register value read from the save state.

  @retval EFI_SUCCESS           The register was read from Save State.
  @retval EFI_NOT_FOUND         The register is not defined for the Save State of Processor.
  @retval EFI_INVALID_PARAMTER  This or Buffer is NULL.

**/
EFI_STATUS
SmramSaveStateReadRegisterByIndex (
  IN UINTN  CpuIndex,
  IN UINTN  RegisterIndex,
  IN UINTN  Width,
  OUT VOID  *Buffer
  )
{
  if (RegisterIndex == 0) {
    return EFI_NOT_FOUND;
  }

  if (SmramSaveStateGetRegisterLma () == EFI_SMM_SAVE_STATE_REGISTER_LMA_32BIT) {
    //
    // If 32-bit mode width is zero, then the specified register can not be accessed
    //
    if (mSmmSmramCpuWidthOffset[RegisterIndex].Width32 == 0) {
      return EFI_NOT_FOUND;
    }

    //
    // If Width is bigger than the 32-bit mode width, then the specified register can not be accessed
    //
    if (Width > mSmmSmramCpuWidthOffset[RegisterIndex].Width32) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Write return buffer
    //
    ASSERT (gSmst->CpuSaveState[CpuIndex] != NULL);
    CopyMem (Buffer, (UINT8 *)gSmst->CpuSaveState[CpuIndex] + mSmmSmramCpuWidthOffset[RegisterIndex].Offset32, Width);
  } else {
    //
    // If 64-bit mode width is zero, then the specified register can not be accessed
    //
    if (mSmmSmramCpuWidthOffset[RegisterIndex].Width64 == 0) {
      return EFI_NOT_FOUND;
    }

    //
    // If Width is bigger than the 64-bit mode width, then the specified register can not be accessed
    //
    if (Width > mSmmSmramCpuWidthOffset[RegisterIndex].Width64) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Write lower 32-bits of return buffer
    //
    CopyMem (Buffer, (UINT8 *)gSmst->CpuSaveState[CpuIndex] + mSmmSmramCpuWidthOffset[RegisterIndex].Offset64Lo, MIN (4, Width));
    if (Width > 4) {
      //
      // Write upper 32-bits of return buffer
      //
      CopyMem ((UINT8 *)Buffer + 4, (UINT8 *)gSmst->CpuSaveState[CpuIndex] + mSmmSmramCpuWidthOffset[RegisterIndex].Offset64Hi, Width - 4);
    }
  }

  return EFI_SUCCESS;
}

/**
  Returns LMA value of the Processor.

  @param[in]  VOID

  @retval     UINT8 returns LMA bit value.
**/
UINT8
IntelSmramSaveStateGetRegisterLma (
  VOID
  )
{
  UINT32  RegEax;
  UINT32  RegEdx;
  UINTN   FamilyId;
  UINTN   ModelId;
  UINT8   SmmSaveStateRegisterLma;

  //
  // Retrieve CPU Family
  //
  AsmCpuid (CPUID_VERSION_INFO, &RegEax, NULL, NULL, NULL);
  FamilyId = (RegEax >> 8) & 0xf;
  ModelId  = (RegEax >> 4) & 0xf;
  if ((FamilyId == 0x06) || (FamilyId == 0x0f)) {
    ModelId = ModelId | ((RegEax >> 12) & 0xf0);
  }

  RegEdx = 0;
  AsmCpuid (CPUID_EXTENDED_FUNCTION, &RegEax, NULL, NULL, NULL);
  if (RegEax >= CPUID_EXTENDED_CPU_SIG) {
    AsmCpuid (CPUID_EXTENDED_CPU_SIG, NULL, NULL, NULL, &RegEdx);
  }

  //
  // Determine the mode of the CPU at the time an SMI occurs
  //   Intel(R) 64 and IA-32 Architectures Software Developer's Manual
  //   Volume 3C, Section 34.4.1.1
  //
  SmmSaveStateRegisterLma = EFI_SMM_SAVE_STATE_REGISTER_LMA_32BIT;
  if ((RegEdx & BIT29) != 0) {
    SmmSaveStateRegisterLma = EFI_SMM_SAVE_STATE_REGISTER_LMA_64BIT;
  }

  if (FamilyId == 0x06) {
    if ((ModelId == 0x17) || (ModelId == 0x0f) || (ModelId == 0x1c)) {
      SmmSaveStateRegisterLma = EFI_SMM_SAVE_STATE_REGISTER_LMA_64BIT;
    }
  }

  return SmmSaveStateRegisterLma;
}

/**
  Returns LMA value of the Processor.

  @param[in]  VOID

  @retval     UINT8 returns LMA bit value.
**/
UINT8
AmdSmramSaveStateGetRegisterLma (
  VOID
  )
{
  UINT32  LMAValue;

  LMAValue = (UINT32)AsmReadMsr64 (EFER_ADDRESS) & LMA;
  if (LMAValue) {
    return EFI_SMM_SAVE_STATE_REGISTER_LMA_64BIT;
  }

  return EFI_SMM_SAVE_STATE_REGISTER_LMA_32BIT;
}

/**
  Returns LMA value of the Processor.

  @param[in]  VOID

  @retval     UINT8 returns LMA bit value.
**/
UINT8
SmramSaveStateGetRegisterLma (
  VOID
  )
{
  if (StandardSignatureIsAuthenticAMD ()) {
    return AmdSmramSaveStateGetRegisterLma ();
  }

  return IntelSmramSaveStateGetRegisterLma ();
}
