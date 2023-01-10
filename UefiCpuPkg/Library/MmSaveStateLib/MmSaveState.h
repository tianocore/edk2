/** @file
  SMRAM Save State Map header file.

  Copyright (c) 2010 - 2019, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MM_SAVESTATE_H_
#define MM_SAVESTATE_H_

#include <Uefi/UefiBaseType.h>
#include <Protocol/SmmCpu.h>
#include <Library/DebugLib.h>
#include <Library/MmSaveStateLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/BaseMemoryLib.h>

// EFER register LMA bit
#define LMA  BIT10

// Macro used to simplify the lookup table entries of type CPU_MM_SAVE_STATE_REGISTER_RANGE
#define MM_REGISTER_RANGE(Start, End)  { Start, End, End - Start + 1 }

#define MM_SAVE_STATE_REGISTER_MAX_INDEX  2

// Structure used to describe a range of registers
typedef struct {
  EFI_MM_SAVE_STATE_REGISTER    Start;
  EFI_MM_SAVE_STATE_REGISTER    End;
  UINTN                         Length;
} CPU_MM_SAVE_STATE_REGISTER_RANGE;

// Structure used to build a lookup table to retrieve the widths and offsets
// associated with each supported EFI_MM_SAVE_STATE_REGISTER value

typedef struct {
  UINT8      Width32;
  UINT8      Width64;
  UINT16     Offset32;
  UINT16     Offset64Lo;
  UINT16     Offset64Hi;
  BOOLEAN    Writeable;
} CPU_MM_SAVE_STATE_LOOKUP_ENTRY;

/**
  Returns LMA value of the Processor.

  @param[in]  VOID

  @retval     UINT8 returns LMA bit value.
**/
UINT8
EFIAPI
MmSaveStateGetRegisterLma (
  VOID
  );

/**
  Read information from the CPU save state.

  @param  Register  Specifies the CPU register to read form the save state.

  @retval 0   Register is not valid
  @retval >0  Index into mSmmSmramCpuWidthOffset[] associated with Register

**/
UINTN
EFIAPI
MmSaveStateGetRegisterIndex (
  IN EFI_MM_SAVE_STATE_REGISTER  Register
  );

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
EFIAPI
MmSaveStateReadRegisterByIndex (
  IN UINTN  CpuIndex,
  IN UINTN  RegisterIndex,
  IN UINTN  Width,
  OUT VOID  *Buffer
  );

#endif
