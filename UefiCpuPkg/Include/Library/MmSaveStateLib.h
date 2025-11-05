/** @file
Library that provides service to read/write CPU specific smram save state registers.

Copyright (c) 2010 - 2019, Intel Corporation. All rights reserved.<BR>
Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.<BR>

@note
  SaveState(Read/Write) of EFI_SMM_SAVE_STATE_REGISTER_PROCESSOR_ID/EFI_MM_SAVE_STATE_REGISTER_PROCESSOR_ID
  is handled by PiSmmCpuDxeSmm driver.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MM_SAVE_STATE_LIB_H_
#define MM_SAVE_STATE_LIB_H_

#include <Protocol/MmCpu.h>
#include <Uefi/UefiBaseType.h>

/**
  Read a save state register on the target processor.  If this function
  returns EFI_UNSUPPORTED, then the caller is responsible for reading the
  MM Save State register.

  @param[in]  CpuIndex  The index of the CPU to read the Save State register.
                        The value must be between 0 and the NumberOfCpus field in
                        the System Management System Table (SMST).
  @param[in]  Register  The MM Save State register to read.
  @param[in]  Width     The number of bytes to read from the CPU save state.
  @param[out] Buffer    Upon return, this holds the CPU register value read
                        from the save state.

  @retval EFI_SUCCESS           The register was read from Save State.
  @retval EFI_INVALID_PARAMTER  Buffer is NULL.
  @retval EFI_UNSUPPORTED       This function does not support reading Register.
  @retval EFI_NOT_FOUND         If desired Register not found.
**/
EFI_STATUS
EFIAPI
MmSaveStateReadRegister (
  IN  UINTN                       CpuIndex,
  IN  EFI_MM_SAVE_STATE_REGISTER  Register,
  IN  UINTN                       Width,
  OUT VOID                        *Buffer
  );

/**
  Writes a save state register on the target processor.  If this function
  returns EFI_UNSUPPORTED, then the caller is responsible for writing the
  MM save state register.

  @param[in] CpuIndex  The index of the CPU to write the MM Save State.  The
                       value must be between 0 and the NumberOfCpus field in
                       the System Management System Table (SMST).
  @param[in] Register  The MM Save State register to write.
  @param[in] Width     The number of bytes to write to the CPU save state.
  @param[in] Buffer    Upon entry, this holds the new CPU register value.

  @retval EFI_SUCCESS           The register was written to Save State.
  @retval EFI_INVALID_PARAMTER  Buffer is NULL.
  @retval EFI_UNSUPPORTED       This function does not support writing Register.
  @retval EFI_NOT_FOUND         If desired Register not found.
**/
EFI_STATUS
EFIAPI
MmSaveStateWriteRegister (
  IN UINTN                       CpuIndex,
  IN EFI_MM_SAVE_STATE_REGISTER  Register,
  IN UINTN                       Width,
  IN CONST VOID                  *Buffer
  );

#endif
