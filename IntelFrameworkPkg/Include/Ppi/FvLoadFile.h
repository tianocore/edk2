/** @file
  Load image file from fv to memory.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
  This PPI is defined in PEI CIS spec Version 0.91.

**/

#ifndef _FV_FILE_LOADER_PPI_H_
#define _FV_FILE_LOADER_PPI_H_

#define EFI_PEI_FV_FILE_LOADER_GUID \
  { \
    0x7e1f0d85, 0x4ff, 0x4bb2, {0x86, 0x6a, 0x31, 0xa2, 0x99, 0x6a, 0x48, 0xa8 } \
  }

typedef struct _EFI_PEI_FV_FILE_LOADER_PPI  EFI_PEI_FV_FILE_LOADER_PPI;

/**
  Loads a PEIM into memory for subsequent execution.

  @param  This           Interface pointer that implements the Load File PPI instance.
  @param  FfsHeader      The pointer to the FFS header of the file to load.
  @param  ImageAddress   The pointer to the address of the loaded Image
  @param  ImageSize      The pointer to the size of the loaded image.
  @param  EntryPoint     The pointer to the entry point of the image.

  @retval EFI_SUCCESS           The image was loaded successfully.
  @retval EFI_OUT_OF_RESOURCES  There was not enough memory.
  @retval EFI_INVALID_PARAMETER The contents of the FFS file did not
                                contain a valid PE/COFF image that could be loaded.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_FV_LOAD_FILE)(
  IN  EFI_PEI_FV_FILE_LOADER_PPI                *This,
  IN  EFI_FFS_FILE_HEADER                       *FfsHeader,
  OUT EFI_PHYSICAL_ADDRESS                      *ImageAddress,
  OUT UINT64                                    *ImageSize,
  OUT EFI_PHYSICAL_ADDRESS                      *EntryPoint
  );

/**
  This PPI is a pointer to the Load File service. This service will be
  published by a PEIM. The PEI Foundation will use this service to
  launch the known non-XIP PE/COFF PEIM images. This service may
  depend upon the presence of the EFI_PEI_PERMANENT_MEMORY_INSTALLED_PPI.
**/
struct _EFI_PEI_FV_FILE_LOADER_PPI {
  ///
  /// Loads a PEIM into memory for subsequent execution.
  ///
  EFI_PEI_FV_LOAD_FILE  FvLoadFile;
};

extern EFI_GUID gEfiPeiFvFileLoaderPpiGuid;

#endif
