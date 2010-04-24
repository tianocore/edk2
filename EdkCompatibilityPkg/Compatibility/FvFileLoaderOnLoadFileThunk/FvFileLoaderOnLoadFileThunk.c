/** @file
Module produce Framework's EFI_PEI_FV_FILE_LOADER_PPI top of EFI_PEI_LOAD_FILE_PPI.

UEFI PI Spec supersedes Intel's Framework Specs. 
EFI_PEI_FV_FILE_LOADER_PPI defined in Intel Framework Pkg is replaced by EFI_PEI_LOAD_FILE_PPI
in MdePkg.
This module produces EFI_PEI_FV_FILE_LOADER_PPI on top of EFI_PEI_LOAD_FILE_PPI . 
This module is used on platform when both of these two conditions are true:
1) Framework module consumes EFI_PEI_FV_FILE_LOADER_PPI is present.
2) The platform has PI modules that produce EFI_PEI_LOAD_FILE_PPI.

Copyright (c) 2008 - 2010, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
Module Name:

**/

#include <PiPei.h>
#include <Ppi/LoadFile.h>
#include <Ppi/FvLoadFile.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>

/**

  Wrap the call to PI's EFI_PEI_LOAD_FILE_PPI.

  @param This           A pointer to EFI_PEI_FV_FILE_LOADER_PPI.
  @param FfsHeader      Pointer to the FFS header of the file to load.
  @param ImageAddress   The loaded address of the Image.
  @param ImageSize      Pointer to the size of the loaded image.
  @param EntryPoint     Pointer to the entry point of the image.

  @retval  EFI_SUCCESS           The image was loaded successfully.
  @retval  EFI_OUT_OF_RESOURCE   There was not enought memory.
  @retval  EFI_INVALID_PARAMETER The contents of the FFS file did not contain a valid PE/COFF image that could be loaded.
**/  
EFI_STATUS
EFIAPI
FrameworkLoadFile (
  IN EFI_PEI_FV_FILE_LOADER_PPI                 *This,
  IN  EFI_FFS_FILE_HEADER                       *FfsHeader,
  OUT EFI_PHYSICAL_ADDRESS                      *ImageAddress,
  OUT UINT64                                    *ImageSize,
  OUT EFI_PHYSICAL_ADDRESS                      *EntryPoint
  );

EFI_PEI_FV_FILE_LOADER_PPI mLoadFilePpi = {
  FrameworkLoadFile
};

EFI_PEI_PPI_DESCRIPTOR     mPpiFrameworkLoadFile = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiFvFileLoaderPpiGuid,
  &mLoadFilePpi
};

/**
  Standard entry point of a PEIM.

  @param FfsHeader    The FFS file header
  @param PeiServices  General purpose services available to every PEIM.

  @retval EFI_SUCCESS If the gEfiPeiReadOnlyVariablePpiGuid interface could be successfully installed.

**/
EFI_STATUS
EFIAPI
InitPeim (
  IN EFI_PEI_FILE_HANDLE     FfsHeader,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  //
  // This thunk module can only be used together with a PI PEI core, as we 
  // assume PeiServices Pointer Table can be located in a standard way defined
  // in PI spec.
  //
  ASSERT ((*PeiServices)->Hdr.Revision >= 0x00010000);
  return (*PeiServices)->InstallPpi (PeiServices, &mPpiFrameworkLoadFile);
}


/**

  Wrap the call to PI's EFI_PEI_LOAD_FILE_PPI.

  @param This           A pointer to EFI_PEI_FV_FILE_LOADER_PPI.
  @param FfsHeader      The pointer to the file header to be loaded by the Pe/Coff loader.
  @param ImageAddress   The loaded address of the Image.
  @param ImageSize      Pointer to the size of the loaded image.
  @param EntryPoint     Pointer to the entry point of the image.

  @retval  EFI_SUCCESS           The image was loaded successfully.
  @retval  EFI_OUT_OF_RESOURCE   There was not enought memory.
  @retval  EFI_INVALID_PARAMETER The contents of the FFS file did not contain a valid PE/COFF image that could be loaded.
**/  
EFI_STATUS
EFIAPI
FrameworkLoadFile (
  IN EFI_PEI_FV_FILE_LOADER_PPI                 *This,
  IN  EFI_FFS_FILE_HEADER                       *FfsHeader,
  OUT EFI_PHYSICAL_ADDRESS                      *ImageAddress,
  OUT UINT64                                    *ImageSize,
  OUT EFI_PHYSICAL_ADDRESS                      *EntryPoint
  )
{
  EFI_STATUS              Status;
  EFI_PEI_LOAD_FILE_PPI   *PiLoadFile;
  UINT32                  AuthenticationState;

  Status = PeiServicesLocatePpi (
             &gEfiPeiLoadFilePpiGuid,
             0,
             NULL,
             (VOID **) &PiLoadFile
             );
  ASSERT_EFI_ERROR (Status);

  return PiLoadFile->LoadFile (
           PiLoadFile,
           (EFI_PEI_FILE_HANDLE) FfsHeader,
           ImageAddress,
           ImageSize,
           EntryPoint,
           &AuthenticationState
         );
    
}

