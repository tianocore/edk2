/** @file
SMRAM Save State Map header file.

Copyright (c) 2010 - 2019, Intel Corporation. All rights reserved.<BR>
Copyright (C) 2022 Advanced Micro Devices, Inc. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SMRAM_SAVESTATE_H_
#define SMRAM_SAVESTATE_H_

#include <Library/SmmCpuFeaturesLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Register/Amd/SmramSaveStateMap.h>
#include <Guid/AcpiS3Context.h>

// EFER register LMA bit
#define LMA  BIT10

// Machine Specific Registers (MSRs)
#define SMMADDR_ADDRESS  0xC0010112ul
#define SMMMASK_ADDRESS  0xC0010113ul
#define EFER_ADDRESS     0XC0000080ul

// Macro used to simplify the lookup table entries of type CPU_SMM_SAVE_STATE_LOOKUP_ENTRY
#define SMM_CPU_OFFSET(Field)  OFFSET_OF (AMD_SMRAM_SAVE_STATE_MAP, Field)

// Macro used to simplify the lookup table entries of type CPU_SMM_SAVE_STATE_REGISTER_RANGE
#define SMM_REGISTER_RANGE(Start, End)  { Start, End, End - Start + 1 }

// Structure used to describe a range of registers
typedef struct {
  EFI_SMM_SAVE_STATE_REGISTER    Start;
  EFI_SMM_SAVE_STATE_REGISTER    End;
  UINTN                          Length;
} CPU_SMM_SAVE_STATE_REGISTER_RANGE;

// Structure used to build a lookup table to retrieve the widths and offsets
// associated with each supported EFI_SMM_SAVE_STATE_REGISTER value

#define SMM_SAVE_STATE_REGISTER_SMMREVID_INDEX  1
#define SMM_SAVE_STATE_REGISTER_MAX_INDEX       2

typedef struct {
  UINT8      Width32;
  UINT8      Width64;
  UINT16     Offset32;
  UINT16     Offset64Lo;
  UINT16     Offset64Hi;
  BOOLEAN    Writeable;
} CPU_SMM_SAVE_STATE_LOOKUP_ENTRY;

/**
  Read an SMM Save State register on the target processor.  If this function
  returns EFI_UNSUPPORTED, then the caller is responsible for reading the
  SMM Save Sate register.

  @param[in]  CpuIndex  The index of the CPU to read the SMM Save State.  The
                        value must be between 0 and the NumberOfCpus field in
                        the System Management System Table (SMST).
  @param[in]  Register  The SMM Save State register to read.
  @param[in]  Width     The number of bytes to read from the CPU save state.
  @param[out] Buffer    Upon return, this holds the CPU register value read
                        from the save state.

  @retval EFI_SUCCESS           The register was read from Save State.
  @retval EFI_INVALID_PARAMTER  Buffer is NULL.
  @retval EFI_UNSUPPORTED       This function does not support reading Register.

**/
EFI_STATUS
EFIAPI
InternalSmmCpuFeaturesReadSaveStateRegister (
  IN  UINTN                        CpuIndex,
  IN  EFI_SMM_SAVE_STATE_REGISTER  Register,
  IN  UINTN                        Width,
  OUT VOID                         *Buffer
  );

/**
  Writes an SMM Save State register on the target processor.  If this function
  returns EFI_UNSUPPORTED, then the caller is responsible for writing the
  SMM Save Sate register.

  @param[in] CpuIndex  The index of the CPU to write the SMM Save State.  The
                       value must be between 0 and the NumberOfCpus field in
                       the System Management System Table (SMST).
  @param[in] Register  The SMM Save State register to write.
  @param[in] Width     The number of bytes to write to the CPU save state.
  @param[in] Buffer    Upon entry, this holds the new CPU register value.

  @retval EFI_SUCCESS           The register was written to Save State.
  @retval EFI_INVALID_PARAMTER  Buffer is NULL.
  @retval EFI_UNSUPPORTED       This function does not support writing Register.
**/
EFI_STATUS
EFIAPI
InternalSmmCpuFeaturesWriteSaveStateRegister (
  IN UINTN                        CpuIndex,
  IN EFI_SMM_SAVE_STATE_REGISTER  Register,
  IN UINTN                        Width,
  IN CONST VOID                   *Buffer
  );

/**
  Initialize MP synchronization data.
**/
VOID
EFIAPI
InitializeMpSyncData (
  VOID
  );

/**
  Perform SMM MP sync Semaphores re-initialization in the S3 boot path.
**/
VOID
EFIAPI
SmmS3MpSemaphoreInit (
  VOID
  );

#endif
