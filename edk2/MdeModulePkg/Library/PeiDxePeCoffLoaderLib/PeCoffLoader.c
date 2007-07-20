/*++

Copyright (c) 2006 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EdkPeCoffLoader.c

Abstract:

  Wrap the Base PE/COFF loader with the PE COFF Protocol


--*/


#include <PiPei.h>

#include <Guid/PeiPeCoffLoader.h>

#include <Library/PeCoffLib.h>

#include <IndustryStandard/PeImage.h>


STATIC
EFI_STATUS
EFIAPI
PeCoffLoaderLibGetImageInfo (
  IN EFI_PEI_PE_COFF_LOADER_PROTOCOL          *This,
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT         *ImageContext
  )
{
  EFI_STATUS  Status;

  Status = PeCoffLoaderGetImageInfo (ImageContext);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  switch (ImageContext->ImageType) {

  case EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION:
    ImageContext->ImageCodeMemoryType = EfiLoaderCode;
    ImageContext->ImageDataMemoryType = EfiLoaderData;
    break;

  case EFI_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER:
    ImageContext->ImageCodeMemoryType = EfiBootServicesCode;
    ImageContext->ImageDataMemoryType = EfiBootServicesData;
    break;

  case EFI_IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER:
  case EFI_IMAGE_SUBSYSTEM_SAL_RUNTIME_DRIVER:
    ImageContext->ImageCodeMemoryType = EfiRuntimeServicesCode;
    ImageContext->ImageDataMemoryType = EfiRuntimeServicesData;
    break;

  default:
    ImageContext->ImageError = IMAGE_ERROR_INVALID_SUBSYSTEM;
    return RETURN_UNSUPPORTED;
  }

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
PeCoffLoaderLibLoadImage (
  IN EFI_PEI_PE_COFF_LOADER_PROTOCOL          *This,
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT         *ImageContext
  )
{
  return PeCoffLoaderLoadImage (ImageContext);
}

STATIC
EFI_STATUS
EFIAPI
PeCoffLoaderLibRelocateImage (
  IN EFI_PEI_PE_COFF_LOADER_PROTOCOL          *This,
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT         *ImageContext
  )
{
  return PeCoffLoaderRelocateImage (ImageContext);
}

STATIC
EFI_STATUS
EFIAPI
PeCoffLoaderLibUnloadimage (
  IN EFI_PEI_PE_COFF_LOADER_PROTOCOL      *This,
  IN PE_COFF_LOADER_IMAGE_CONTEXT         *ImageContext
  )
{
  return EFI_SUCCESS;
}


EFI_PEI_PE_COFF_LOADER_PROTOCOL  mPeiEfiPeiPeCoffLoader = {
  PeCoffLoaderLibGetImageInfo,
  PeCoffLoaderLibLoadImage,
  PeCoffLoaderLibRelocateImage,
  PeCoffLoaderLibUnloadimage
};

EFI_PEI_PE_COFF_LOADER_PROTOCOL *
EFIAPI
GetPeCoffLoaderProtocol (
  )
{
  return &mPeiEfiPeiPeCoffLoader;
}


