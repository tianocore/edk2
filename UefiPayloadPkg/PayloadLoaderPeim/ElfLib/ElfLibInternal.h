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
#include "ElfLib.h"
#include "ElfCommon.h"
#include "Elf32.h"
#include "Elf64.h"

#define ELF_NEXT_ENTRY(EntryType, Current, EntrySize) \
              ((EntryType *) ((UINT8 *)Current + EntrySize))


/**
  Return the section header specified by Index.

  @param ImageBase      The image base.
  @param Index          The section index.

  @return Pointer to the section header.
**/
Elf32_Shdr *
GetElf32SectionByIndex (
  IN  UINT8                 *ImageBase,
  IN  UINT32                Index
  );

/**
  Return the section header specified by Index.

  @param ImageBase      The image base.
  @param Index          The section index.

  @return Pointer to the section header.
**/
Elf64_Shdr *
GetElf64SectionByIndex (
  IN  UINT8                 *ImageBase,
  IN  UINT32                Index
  );

/**
  Return the segment header specified by Index.

  @param ImageBase      The image base.
  @param Index          The segment index.

  @return Pointer to the segment header.
**/
Elf32_Phdr *
GetElf32SegmentByIndex (
  IN  UINT8                 *ImageBase,
  IN  UINT32                Index
  );

/**
  Return the segment header specified by Index.

  @param ImageBase      The image base.
  @param Index          The segment index.

  @return Pointer to the segment header.
**/
Elf64_Phdr *
GetElf64SegmentByIndex (
  IN  UINT8                 *ImageBase,
  IN  UINT32                Index
  );

/**
  Load ELF image which has 32-bit architecture

  @param[in]  ElfCt               ELF image context pointer.

  @retval EFI_SUCCESS         ELF binary is loaded successfully.
  @retval Others              Loading ELF binary fails.

**/
EFI_STATUS
LoadElf32Image (
  IN    ELF_IMAGE_CONTEXT    *ElfCt
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
  IN    ELF_IMAGE_CONTEXT    *ElfCt
  );

#endif
