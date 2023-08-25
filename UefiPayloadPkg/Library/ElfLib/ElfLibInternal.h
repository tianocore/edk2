/** @file
  ELF library
  Copyright (c) 2019 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef EFI_LIB_INTERNAL_H_
#define EFI_LIB_INTERNAL_H_

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/ElfLib.h>
#include <ElfCommon.h>
#include <Elf32.h>
#include <Elf64.h>

#define ELF_NEXT_ENTRY(EntryType, Current, EntrySize) \
              ((EntryType *) ((UINT8 *)Current + EntrySize))

/**
  Load ELF image which has 32-bit architecture
  @param[in]  ElfCt               ELF image context pointer.
  @retval EFI_SUCCESS         ELF binary is loaded successfully.
  @retval Others              Loading ELF binary fails.
**/
EFI_STATUS
LoadElf32Image (
  IN    ELF_IMAGE_CONTEXT  *ElfCt
  );

/**
  Load ELF image which has 64-bit architecture
  @param[in]  ImageBase       Memory address of an image.
  @param[out] EntryPoint      The entry point of loaded ELF image.
  @retval EFI_SUCCESS         ELF binary is loaded successfully.
  @retval Others              Loading ELF binary fails.
**/
EFI_STATUS
LoadElf64Image (
  IN    ELF_IMAGE_CONTEXT  *ElfCt
  );

#endif