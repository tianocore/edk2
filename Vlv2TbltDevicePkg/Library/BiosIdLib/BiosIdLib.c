/*++

Copyright (c) 2011  - 2014, Intel Corporation. All rights reserved
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

Module Name:

  BiosIdLib.c

Abstract:

  Boot service DXE BIOS ID library implementation.

  These functions in this file can be called during DXE and cannot be called during runtime
  or in SMM which should use a RT or SMM library.

--*/

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/HobLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

#include <Library/BiosIdLib.h>
#include <Guid/BiosId.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/LoadedImage.h>


EFI_STATUS
GetImageFromFv (
  IN  EFI_FIRMWARE_VOLUME2_PROTOCOL *Fv,
  IN  EFI_GUID           *NameGuid,
  IN  EFI_SECTION_TYPE   SectionType,
  OUT VOID               **Buffer,
  OUT UINTN              *Size
  )
{
  EFI_STATUS                Status;
  EFI_FV_FILETYPE           FileType;
  EFI_FV_FILE_ATTRIBUTES    Attributes;
  UINT32                    AuthenticationStatus;

  //
  // Read desired section content in NameGuid file
  //
  *Buffer     = NULL;
  *Size       = 0;
  Status      = Fv->ReadSection (
                      Fv,
                      NameGuid,
                      SectionType,
                      0,
                      Buffer,
                      Size,
                      &AuthenticationStatus
                      );

  if (EFI_ERROR (Status) && (SectionType == EFI_SECTION_TE)) {
    //
    // Try reading PE32 section, since the TE section does not exist
    //
    *Buffer = NULL;
    *Size   = 0;
    Status  = Fv->ReadSection (
                    Fv,
                    NameGuid,
                    EFI_SECTION_PE32,
                    0,
                    Buffer,
                    Size,
                    &AuthenticationStatus
                    );
  }

  if (EFI_ERROR (Status) &&
      ((SectionType == EFI_SECTION_TE) || (SectionType == EFI_SECTION_PE32))) {
    //
    // Try reading raw file, since the desired section does not exist
    //
    *Buffer = NULL;
    *Size   = 0;
    Status  = Fv->ReadFile (
                    Fv,
                    NameGuid,
                    Buffer,
                    Size,
                    &FileType,
                    &Attributes,
                    &AuthenticationStatus
                    );
  }

  return Status;
}


EFI_STATUS
GetImageEx (
  IN  EFI_HANDLE         ImageHandle,
  IN  EFI_GUID           *NameGuid,
  IN  EFI_SECTION_TYPE   SectionType,
  OUT VOID               **Buffer,
  OUT UINTN              *Size,
  BOOLEAN                WithinImageFv
  )
{
  EFI_STATUS                    Status;
  EFI_HANDLE                    *HandleBuffer;
  UINTN                         HandleCount;
  UINTN                         Index;
  EFI_LOADED_IMAGE_PROTOCOL     *LoadedImage;

  EFI_FIRMWARE_VOLUME2_PROTOCOL *ImageFv;
  EFI_FIRMWARE_VOLUME2_PROTOCOL *Fv;


  if (ImageHandle == NULL && WithinImageFv) {
    return EFI_INVALID_PARAMETER;
  }

  Status  = EFI_NOT_FOUND;
  ImageFv = NULL;
  if (ImageHandle != NULL) {
    Status = gBS->HandleProtocol (
                    ImageHandle,
                    &gEfiLoadedImageProtocolGuid,
                    (VOID **) &LoadedImage
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }
    Status = gBS->HandleProtocol (
                    LoadedImage->DeviceHandle,
                    &gEfiFirmwareVolume2ProtocolGuid,
                    (VOID **) &ImageFv
                    );
    if (!EFI_ERROR (Status)) {
      Status = GetImageFromFv (ImageFv, NameGuid, SectionType, Buffer, Size);
    }
  }

  if (Status == EFI_SUCCESS || WithinImageFv) {
    return Status;
  }

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiFirmwareVolume2ProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Find desired image in all Fvs
  //
  for (Index = 0; Index < HandleCount; ++Index) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiFirmwareVolume2ProtocolGuid,
                    (VOID**)&Fv
                    );

    if (EFI_ERROR (Status)) {
      gBS->FreePool(HandleBuffer);
      return Status;
    }

    if (ImageFv != NULL && Fv == ImageFv) {
      continue;
    }

    Status = GetImageFromFv (Fv, NameGuid, SectionType, Buffer, Size);

    if (!EFI_ERROR (Status)) {
      break;
    }
  }
  gBS->FreePool(HandleBuffer);

  //
  // Not found image
  //
  if (Index == HandleCount) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**
  This function returns BIOS ID by searching HOB or FV.

  @param BiosIdImage             The BIOS ID got from HOB or FV.

  @retval EFI_SUCCESS            All parameters were valid and BIOS ID has been got.
  @retval EFI_NOT_FOUND          BiosId image is not found, and no parameter will be modified.
  @retval EFI_INVALID_PARAMETER  The parameter is NULL.

**/
EFI_STATUS
GetBiosId (
  OUT BIOS_ID_IMAGE     *BiosIdImage
  )

{
  EFI_STATUS    Status;
  VOID          *Address = NULL;
  UINTN         Size = 0;

    DEBUG ((EFI_D_INFO, "Get BIOS ID from FV\n"));

    Status = GetImageEx (
               NULL,
               &gEfiBiosIdGuid,
               EFI_SECTION_RAW,
               &Address,
               &Size,
               FALSE
               );

    if (Status == EFI_SUCCESS) {
      //
      // BiosId image is present in FV
      //
      if (Address != NULL) {
        Size = sizeof (BIOS_ID_IMAGE);
        gBS->CopyMem (
              (void *) BiosIdImage,
              Address,
              Size
              );
        //
        // GetImage () allocated buffer for Address, now clear it.
        //
        gBS->FreePool (Address);

        DEBUG ((EFI_D_INFO, "Get BIOS ID from FV successfully\n"));
        DEBUG ((EFI_D_INFO, "BIOS ID: %s\n", (CHAR16 *) (&(BiosIdImage->BiosIdString))));

        return EFI_SUCCESS;
      }
    }
  return EFI_NOT_FOUND;
}

/**
  This function returns the Version & Release Date and Time by getting and converting
  BIOS ID.

  @param BiosVersion  The Bios Version out of the conversion.
  @param BiosReleaseDate  The Bios Release Date out of the conversion.
  @param BiosReleaseTime - The Bios Release Time out of the conversion.

  @retval EFI_SUCCESS - BIOS Version & Release Date and Time have been got successfully.
  @retval EFI_NOT_FOUND - BiosId image is not found, and no parameter will be modified.
  @retval EFI_INVALID_PARAMETER - All the parameters are NULL.

**/
EFI_STATUS
GetBiosVersionDateTime (
  OUT CHAR16    *BiosVersion, OPTIONAL
  OUT CHAR16    *BiosReleaseDate, OPTIONAL
  OUT CHAR16    *BiosReleaseTime OPTIONAL
  )
{
  EFI_STATUS        Status;
  BIOS_ID_IMAGE     BiosIdImage;

  if ((BiosVersion == NULL) && (BiosReleaseDate == NULL) && (BiosReleaseTime == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetBiosId (&BiosIdImage);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  if (BiosVersion != NULL) {
    //
    // Fill the BiosVersion data from the BIOS ID.
    //
    StrCpy (BiosVersion, (CHAR16 *) (&(BiosIdImage.BiosIdString)));
  }

  if (BiosReleaseDate != NULL) {
    //
    // Fill the build timestamp date from the BIOS ID in the "MM/DD/YY" format.
    //
    BiosReleaseDate[0] = BiosIdImage.BiosIdString.TimeStamp[2];
    BiosReleaseDate[1] = BiosIdImage.BiosIdString.TimeStamp[3];
    BiosReleaseDate[2] = (CHAR16) ((UINT8) ('/'));

    BiosReleaseDate[3] = BiosIdImage.BiosIdString.TimeStamp[4];
    BiosReleaseDate[4] = BiosIdImage.BiosIdString.TimeStamp[5];
    BiosReleaseDate[5] = (CHAR16) ((UINT8) ('/'));

    //
    // Add 20 for SMBIOS table
    // Current Linux kernel will misjudge 09 as year 0, so using 2009 for SMBIOS table
    //
    BiosReleaseDate[6] = '2';
    BiosReleaseDate[7] = '0';
    BiosReleaseDate[8] = BiosIdImage.BiosIdString.TimeStamp[0];
    BiosReleaseDate[9] = BiosIdImage.BiosIdString.TimeStamp[1];

    BiosReleaseDate[10] = (CHAR16) ((UINT8) ('\0'));
  }

  if (BiosReleaseTime != NULL) {

    //
    // Fill the build timestamp time from the BIOS ID in the "HH:MM" format.
    //

    BiosReleaseTime[0] = BiosIdImage.BiosIdString.TimeStamp[6];
    BiosReleaseTime[1] = BiosIdImage.BiosIdString.TimeStamp[7];
    BiosReleaseTime[2] = (CHAR16) ((UINT8) (':'));

    BiosReleaseTime[3] = BiosIdImage.BiosIdString.TimeStamp[8];
    BiosReleaseTime[4] = BiosIdImage.BiosIdString.TimeStamp[9];

    BiosReleaseTime[5] = (CHAR16) ((UINT8) ('\0'));
  }

  return  EFI_SUCCESS;
}

