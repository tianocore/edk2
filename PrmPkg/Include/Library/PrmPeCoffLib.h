/** @file

  The PRM PE/COFF library provides functionality to support additional PE/COFF functionality needed to use
  Platform Runtime Mechanism (PRM) modules.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PRM_PECOFF_LIB_H_
#define PRM_PECOFF_LIB_H_

#include <Base.h>
#include <PrmExportDescriptor.h>
#include <IndustryStandard/PeImage.h>
#include <Library/PeCoffLib.h>

/**
  Gets a pointer to the export directory in a given PE/COFF image.

  @param[in]  ImageExportDirectory        A pointer to an export directory table in a PE/COFF image.
  @param[in]  PeCoffLoaderImageContext    A pointer to a PE_COFF_LOADER_IMAGE_CONTEXT structure that contains the
                                          PE/COFF image context for the Image containing the PRM Module Export
                                          Descriptor table.
  @param[out] ExportDescriptor            A pointer to a pointer to the PRM Module Export Descriptor table found
                                          in the ImageExportDirectory given.

  @retval EFI_SUCCESS                     The PRM Module Export Descriptor table was found successfully.
  @retval EFI_INVALID_PARAMETER           A required parameter is NULL.
  @retval EFI_NOT_FOUND                   The PRM Module Export Descriptor table was not found in the given
                                          ImageExportDirectory.

**/
EFI_STATUS
GetPrmModuleExportDescriptorTable (
  IN  EFI_IMAGE_EXPORT_DIRECTORY           *ImageExportDirectory,
  IN  PE_COFF_LOADER_IMAGE_CONTEXT         *PeCoffLoaderImageContext,
  OUT PRM_MODULE_EXPORT_DESCRIPTOR_STRUCT  **ExportDescriptor
  );

/**
  Gets a pointer to the export directory in a given PE/COFF image.

  @param[in]  Image                       A pointer to a PE32/COFF image base address that is loaded into memory
                                          and already relocated to the memory base address. RVAs in the image given
                                          should be valid.
  @param[in]  PeCoffLoaderImageContext    A pointer to a PE_COFF_LOADER_IMAGE_CONTEXT structure that contains the
                                          PE/COFF image context for the Image given.
  @param[out] ImageExportDirectory        A pointer to a pointer to the export directory found in the Image given.

  @retval EFI_SUCCESS                     The export directory was found successfully.
  @retval EFI_INVALID_PARAMETER           A required parameter is NULL.
  @retval EFI_UNSUPPORTED                 The PE/COFF image given is not supported as a PRM Module.
  @retval EFI_NOT_FOUND                   The image export directory could not be found for this image.

**/
EFI_STATUS
GetExportDirectoryInPeCoffImage (
  IN  VOID                          *Image,
  IN  PE_COFF_LOADER_IMAGE_CONTEXT  *PeCoffLoaderImageContext,
  OUT EFI_IMAGE_EXPORT_DIRECTORY    **ImageExportDirectory
  );

/**
  Returns the image major and image minor version in a given PE/COFF image.

  @param[in]  Image                       A pointer to a PE32/COFF image base address that is loaded into memory
                                          and already relocated to the memory base address. RVAs in the image given
                                          should be valid.
  @param[in]  PeCoffLoaderImageContext    A pointer to a PE_COFF_LOADER_IMAGE_CONTEXT structure that contains the
                                          PE/COFF image context for the Image given.
  @param[out] ImageMajorVersion           A pointer to a UINT16 buffer to hold the image major version.
  @param[out] ImageMinorVersion           A pointer to a UINT16 buffer to hold the image minor version.

  @retval EFI_SUCCESS                     The image version was read successfully.
  @retval EFI_INVALID_PARAMETER           A required parameter is NULL.
  @retval EFI_UNSUPPORTED                 The PE/COFF image given is not supported.

**/
EFI_STATUS
GetImageVersionInPeCoffImage (
  IN  VOID                          *Image,
  IN  PE_COFF_LOADER_IMAGE_CONTEXT  *PeCoffLoaderImageContext,
  OUT UINT16                        *ImageMajorVersion,
  OUT UINT16                        *ImageMinorVersion
  );

/**
  Gets the address of an entry in an image export table by ASCII name.

  @param[in]  ExportName                  A pointer to an ASCII name string of the entry name.
  @param[in]  ImageBaseAddress            The base address of the PE/COFF image.
  @param[in]  ImageExportDirectory        A pointer to the export directory in the image.
  @param[out] ExportPhysicalAddress       A pointer that will be updated with the address of the address of the
                                          export entry if found.

  @retval EFI_SUCCESS                     The export entry was found successfully.
  @retval EFI_INVALID_PARAMETER           A required pointer argument is NULL.
  @retval EFI_NOT_FOUND                   An entry with the given ExportName was not found.

**/
EFI_STATUS
GetExportEntryAddress (
  IN  CONST CHAR8                 *ExportName,
  IN  EFI_PHYSICAL_ADDRESS        ImageBaseAddress,
  IN  EFI_IMAGE_EXPORT_DIRECTORY  *ImageExportDirectory,
  OUT EFI_PHYSICAL_ADDRESS        *ExportPhysicalAddress
  );

#endif
