/** @file
  ELF library

  Copyright (c) 2018 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef ELF_LIB_H_
#define ELF_LIB_H_

#include <PiPei.h>

#define  ELF_CLASS32   1
#define  ELF_CLASS64   2

#define  ELF_PT_LOAD   1

typedef struct {
  RETURN_STATUS ParseStatus;             ///< Return the status after ParseElfImage().
  UINT8         *FileBase;               ///< The source location in memory.
  UINTN         FileSize;                ///< The size including sections that don't require loading.
  UINT8         *PreferredImageAddress;  ///< The preferred image to be loaded. No relocation is needed if loaded to this address.
  BOOLEAN       ReloadRequired;          ///< The image needs a new memory location for running.
  UINT8         *ImageAddress;           ///< The destination memory address set by caller.
  UINTN         ImageSize;               ///< The memory size for loading and execution.
  UINT32        EiClass;
  UINT32        ShNum;
  UINT32        PhNum;
  UINTN         ShStrOff;
  UINTN         ShStrLen;
  UINTN         EntryPoint;              ///< Return the actual entry point after LoadElfImage().
} ELF_IMAGE_CONTEXT;


typedef struct {
  UINT32      PtType;
  UINTN       Offset;
  UINTN       Length;
  UINTN       MemLen;
  UINTN       MemAddr;
  UINTN       Alignment;
} SEGMENT_INFO;

/**
  Parse the ELF image info.

  @param[in]  ImageBase      Memory address of an image.
  @param[out] ElfCt          The EFL image context pointer.

  @retval EFI_INVALID_PARAMETER   Input parameters are not valid.
  @retval EFI_UNSUPPORTED         Unsupported binary type.
  @retval EFI_LOAD_ERROR          ELF binary loading error.
  @retval EFI_SUCCESS             ELF binary is loaded successfully.
**/
EFI_STATUS
EFIAPI
ParseElfImage (
  IN  VOID                 *ImageBase,
  OUT ELF_IMAGE_CONTEXT    *ElfCt
  );

/**
  Load the ELF segments to specified address in ELF header.

  This function loads ELF image segments into memory address specified
  in ELF program header.

  @param[in]  ElfCt               ELF image context pointer.

  @retval EFI_INVALID_PARAMETER   Input parameters are not valid.
  @retval EFI_UNSUPPORTED         Unsupported binary type.
  @retval EFI_LOAD_ERROR          ELF binary loading error.
  @retval EFI_SUCCESS             ELF binary is loaded successfully.
**/
EFI_STATUS
EFIAPI
LoadElfImage (
  IN  ELF_IMAGE_CONTEXT       *ElfCt
  );

/**
  Get a ELF section name from its index.

  @param[in]  ElfCt               ELF image context pointer.
  @param[in]  SectionIndex        ELF section index.
  @param[out] SectionName         The pointer to the section name.

  @retval EFI_INVALID_PARAMETER   ElfCt or SecName is NULL.
  @retval EFI_NOT_FOUND           Could not find the section.
  @retval EFI_SUCCESS             Section name was filled successfully.
**/
EFI_STATUS
EFIAPI
GetElfSectionName (
  IN  ELF_IMAGE_CONTEXT     *ElfCt,
  IN  UINT32                SectionIndex,
  OUT CHAR8                 **SectionName
  );

/**
  Get the offset and size of x-th ELF section.

  @param[in]  ElfCt               ELF image context pointer.
  @param[in]  Index               ELF section index.
  @param[out] Offset              Return the offset of the specific section.
  @param[out] Size                Return the size of the specific section.

  @retval EFI_INVALID_PARAMETER   ImageBase, Offset or Size is NULL.
  @retval EFI_INVALID_PARAMETER   EiClass doesn't equal to ELFCLASS32 or ELFCLASS64.
  @retval EFI_NOT_FOUND           Could not find the section.
  @retval EFI_SUCCESS             Offset and Size are returned.
**/
EFI_STATUS
EFIAPI
GetElfSectionPos (
  IN  ELF_IMAGE_CONTEXT     *ElfCt,
  IN  UINT32                Index,
  OUT UINTN                 *Offset,
  OUT UINTN                 *Size
  );
#endif /* ELF_LIB_H_ */
