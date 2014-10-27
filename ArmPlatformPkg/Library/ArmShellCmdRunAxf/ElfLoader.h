/** @file
*
*  Copyright (c) 2014, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#ifndef ELF_LOADER_H
#define ELF_LOADER_H

/**
  Check that the ELF File Header is valid and Machine type supported.

  Not all information is checked in the ELF header, only the stuff that
  matters to us in our simplified ELF loader.

  @param[in] ElfImage  Address of the ELF file to check.

  @retval EFI_SUCCESS on success.
  @retval EFI_INVALID_PARAMETER if the header is invalid.
  @retval EFI_UNSUPPORTED if the file type/platform is not supported.
**/
EFI_STATUS
ElfCheckFile (
  IN  CONST VOID *ElfImage
  );


/**
  Load a ELF file.

  @param[in]  ElfImage      Address of the ELF file in memory.

  @param[out] EntryPoint    Will be filled with the ELF entry point address.

  @param[out] ImageSize     Will be filled with the ELF size in memory. This will
                            effectively be equal to the sum of the segments sizes.

  This function assumes the header is valid and supported as checked with
  ElfCheckFile().

  NOTE:
   - We don't currently take the segment permissions into account (indicated by
     the program headers). It can be used to allocate pages with the right
     read/write/exec permissions.

  @retval EFI_SUCCESS on success.
  @retval EFI_INVALID_PARAMETER if the ELF file is invalid.
**/
EFI_STATUS
ElfLoadFile (
  IN  CONST VOID   *ElfImage,
  OUT VOID        **EntryPoint,
  OUT LIST_ENTRY   *LoadList
  );

#endif // ELF_LOADER_H
