/** @file

Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "LegacyBiosInterface.h"

/**
  Assign drive number to legacy HDD drives prior to booting an EFI
  aware OS so the OS can access drives without an EFI driver.
  Note: BBS compliant drives ARE NOT available until this call by
  either shell or EFI.

  @param  This                    Protocol instance pointer.
  @param  BbsCount                Number of BBS_TABLE structures
  @param  BbsTable                List BBS entries

  @retval EFI_SUCCESS             Drive numbers assigned

**/
EFI_STATUS
EFIAPI
LegacyBiosPrepareToBootEfi (
  IN EFI_LEGACY_BIOS_PROTOCOL         *This,
  OUT UINT16                          *BbsCount,
  OUT BBS_TABLE                       **BbsTable
  )
{
  //
  // Shadow All Opion ROM
  //
  LegacyBiosShadowAllLegacyOproms (This);
  return EFI_SUCCESS;
}


/**
  To boot from an unconventional device like parties and/or execute
  HDD diagnostics.

  @param  This                    Protocol instance pointer.
  @param  Attributes              How to interpret the other input parameters
  @param  BbsEntry                The 0-based index into the BbsTable for the
                                  parent  device.
  @param  BeerData                Pointer to the 128 bytes of ram BEER data.
  @param  ServiceAreaData         Pointer to the 64 bytes of raw Service Area data.
                                  The caller must provide a pointer to the specific
                                  Service Area and not the start all Service Areas.
 EFI_INVALID_PARAMETER if error. Does NOT return if no error.

**/
EFI_STATUS
EFIAPI
LegacyBiosBootUnconventionalDevice (
  IN EFI_LEGACY_BIOS_PROTOCOL         *This,
  IN UDC_ATTRIBUTES                   Attributes,
  IN UINTN                            BbsEntry,
  IN VOID                             *BeerData,
  IN VOID                             *ServiceAreaData
  )
{
  return EFI_INVALID_PARAMETER;
}


/**
  Attempt to legacy boot the BootOption. If the EFI contexted has been
  compromised this function will not return.

  @param  This                    Protocol instance pointer.
  @param  BbsDevicePath           EFI Device Path from BootXXXX variable.
  @param  LoadOptionsSize         Size of LoadOption in size.
  @param  LoadOptions             LoadOption from BootXXXX variable

  @retval EFI_SUCCESS             Removable media not present

**/
EFI_STATUS
EFIAPI
LegacyBiosLegacyBoot (
  IN EFI_LEGACY_BIOS_PROTOCOL           *This,
  IN  BBS_BBS_DEVICE_PATH               *BbsDevicePath,
  IN  UINT32                            LoadOptionsSize,
  IN  VOID                              *LoadOptions
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Build the E820 table.

  @param  Private  Legacy BIOS Instance data
  @param  Size     Size of E820 Table

  @retval EFI_SUCCESS It should always work.

**/
EFI_STATUS
LegacyBiosBuildE820 (
  IN  LEGACY_BIOS_INSTANCE    *Private,
  OUT UINTN                   *Size
  )
{
  *Size = 0;
  return EFI_SUCCESS;
}

/**
  Get all BBS info

  @param  This                    Protocol instance pointer.
  @param  HddCount                Number of HDD_INFO structures
  @param  HddInfo                 Onboard IDE controller information
  @param  BbsCount                Number of BBS_TABLE structures
  @param  BbsTable                List BBS entries

  @retval EFI_SUCCESS             Tables returned
  @retval EFI_NOT_FOUND           resource not found
  @retval EFI_DEVICE_ERROR        can not get BBS table

**/
EFI_STATUS
EFIAPI
LegacyBiosGetBbsInfo (
  IN EFI_LEGACY_BIOS_PROTOCOL         *This,
  OUT UINT16                          *HddCount,
  OUT HDD_INFO                        **HddInfo,
  OUT UINT16                          *BbsCount,
  OUT BBS_TABLE                       **BbsTable
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Fill in the standard BDA for Keyboard LEDs

  @param  This         Protocol instance pointer.
  @param  Leds         Current LED status

  @retval EFI_SUCCESS  It should always work.

**/
EFI_STATUS
EFIAPI
LegacyBiosUpdateKeyboardLedStatus (
  IN EFI_LEGACY_BIOS_PROTOCOL           *This,
  IN  UINT8                             Leds
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Relocate this image under 4G memory for IPF.

  @param  ImageHandle  Handle of driver image.
  @param  SystemTable  Pointer to system table.

  @retval EFI_SUCCESS  Image successfully relocated.
  @retval EFI_ABORTED  Failed to relocate image.

**/
EFI_STATUS
RelocateImageUnder4GIfNeeded (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS                         Status;
  EFI_LOADED_IMAGE_PROTOCOL          *LoadedImage;
  UINTN                              NumberOfPages;
  EFI_PHYSICAL_ADDRESS               LoadedImageBase;
  PE_COFF_LOADER_IMAGE_CONTEXT       ImageContext;
  EFI_PHYSICAL_ADDRESS               MemoryAddress;
  EFI_HANDLE                         NewImageHandle;

  Status = gBS->HandleProtocol (
                    ImageHandle,
                    &gEfiLoadedImageProtocolGuid,
                    (VOID *) &LoadedImage
                    );

  if (!EFI_ERROR (Status)) {
    LoadedImageBase = (EFI_PHYSICAL_ADDRESS) (UINTN) LoadedImage->ImageBase;
    if (LoadedImageBase > 0xffffffff) {
      NumberOfPages = (UINTN) (DivU64x32(LoadedImage->ImageSize, EFI_PAGE_SIZE) + 1);

      //
      // Allocate buffer below 4GB here
      //
      Status = AllocateLegacyMemory (
                AllocateMaxAddress,
                0x7FFFFFFF,
                NumberOfPages,  // do we have to convert this to pages??
                &MemoryAddress
                );
      if (EFI_ERROR (Status)) {
        return Status;
      }

      ZeroMem (&ImageContext, sizeof (PE_COFF_LOADER_IMAGE_CONTEXT));
      ImageContext.Handle    = (VOID *)(UINTN)LoadedImageBase;
      ImageContext.ImageRead = PeCoffLoaderImageReadFromMemory;

      //
      // Get information about the image being loaded
      //
      Status = PeCoffLoaderGetImageInfo (&ImageContext);
      if (EFI_ERROR (Status)) {
        return Status;
      }
      ImageContext.ImageAddress = (PHYSICAL_ADDRESS)MemoryAddress;
      //
      // Align buffer on section boundary
      //
      ImageContext.ImageAddress += ImageContext.SectionAlignment - 1;
     ImageContext.ImageAddress &= ~((PHYSICAL_ADDRESS)ImageContext.SectionAlignment - 1);

      //
      // Load the image to our new buffer
      //
      Status = PeCoffLoaderLoadImage (&ImageContext);
      if (EFI_ERROR (Status)) {
        gBS->FreePages (MemoryAddress, NumberOfPages);
        return Status;
      }

      //
      // Relocate the image in our new buffer
      //
      Status = PeCoffLoaderRelocateImage (&ImageContext);
      if (EFI_ERROR (Status)) {
        gBS->FreePages (MemoryAddress, NumberOfPages);
        return Status;
      }

      //
      // Create a new handle with gEfiCallerIdGuid to be used as the ImageHandle fore the reloaded image
      // 
      NewImageHandle = NULL;
      Status = gBS->InstallProtocolInterface (
                      &NewImageHandle,
                      &gEfiCallerIdGuid,
                      EFI_NATIVE_INTERFACE,
                      NULL
                      );

      //
      // Flush the instruction cache so the image data is written before we execute it
      //
      InvalidateInstructionCacheRange ((VOID *)(UINTN)ImageContext.ImageAddress, (UINTN)ImageContext.ImageSize);

      Status = ((EFI_IMAGE_ENTRY_POINT)(UINTN)(ImageContext.EntryPoint)) (NewImageHandle, SystemTable);
      if (EFI_ERROR (Status)) {
        gBS->FreePages (MemoryAddress, NumberOfPages);
        return Status;
      }
      //
      // return error directly the BS will unload this image
      //
      return EFI_ABORTED;
    }
  }
  return EFI_SUCCESS;
}
