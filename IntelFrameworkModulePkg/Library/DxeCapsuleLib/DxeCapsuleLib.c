/** @file
  Capsule Library instance to update capsule image to flash.

  Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include <PiDxe.h>
#include <Guid/Capsule.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/CapsuleLib.h>

/**
  Those capsules supported by the firmwares.

  @param  CapsuleHeader    Points to a capsule header.

  @retval EFI_SUCESS       Input capsule is supported by firmware.
  @retval EFI_UNSUPPORTED  Input capsule is not supported by the firmware.
**/
EFI_STATUS
EFIAPI
SupportCapsuleImage (
  IN EFI_CAPSULE_HEADER *CapsuleHeader
  )
{
  if (CompareGuid (&gEfiCapsuleGuid, &CapsuleHeader->CapsuleGuid)) {
    return EFI_SUCCESS;
  }

  return EFI_UNSUPPORTED;
}

/**
  The firmware implements to process the capsule image.

  @param  CapsuleHeader         Points to a capsule header.

  @retval EFI_SUCESS            Process Capsule Image successfully.
  @retval EFI_UNSUPPORTED       Capsule image is not supported by the firmware.
  @retval EFI_VOLUME_CORRUPTED  FV volume in the capsule is corrupted.
  @retval EFI_OUT_OF_RESOURCES  Not enough memory.
**/
EFI_STATUS
EFIAPI
ProcessCapsuleImage (
  IN EFI_CAPSULE_HEADER *CapsuleHeader
  )
{
  UINT32                       Length;
  EFI_FIRMWARE_VOLUME_HEADER   *FvImage;
  EFI_FIRMWARE_VOLUME_HEADER   *ProcessedFvImage;
  EFI_STATUS                   Status;
  EFI_HANDLE                   FvProtocolHandle;
  UINT32                       FvAlignment;

  FvImage = NULL;
  ProcessedFvImage = NULL;
  Status  = EFI_SUCCESS;

  if (SupportCapsuleImage (CapsuleHeader) != EFI_SUCCESS) {
    return EFI_UNSUPPORTED;
  }

  //
  // Skip the capsule header, move to the Firware Volume
  //
  FvImage = (EFI_FIRMWARE_VOLUME_HEADER *) ((UINT8 *) CapsuleHeader + CapsuleHeader->HeaderSize);
  Length  = CapsuleHeader->CapsuleImageSize - CapsuleHeader->HeaderSize;

  while (Length != 0) {
    //
    // Point to the next firmware volume header, and then
    // call the DXE service to process it.
    //
    if (FvImage->FvLength > (UINTN) Length) {
      //
      // Notes: need to stuff this status somewhere so that the
      // error can be detected at OS runtime
      //
      Status = EFI_VOLUME_CORRUPTED;
      break;
    }

    FvAlignment = 1 << ((FvImage->Attributes & EFI_FVB2_ALIGNMENT) >> 16);
    //
    // FvAlignment must be more than 8 bytes required by FvHeader structure.
    //
    if (FvAlignment < 8) {
      FvAlignment = 8;
    }
    //
    // Check FvImage Align is required.
    //
    if (((UINTN) FvImage % FvAlignment) == 0) {
      ProcessedFvImage = FvImage;
    } else {
      //
      // Allocate new aligned buffer to store FvImage.
      //
      ProcessedFvImage = (EFI_FIRMWARE_VOLUME_HEADER *) AllocateAlignedPages ((UINTN) EFI_SIZE_TO_PAGES ((UINTN) FvImage->FvLength), (UINTN) FvAlignment);
      if (ProcessedFvImage == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        break;
      }
      CopyMem (ProcessedFvImage, FvImage, (UINTN) FvImage->FvLength);
    }

    Status = gDS->ProcessFirmwareVolume (
                  (VOID *) ProcessedFvImage,
                  (UINTN) ProcessedFvImage->FvLength,
                  &FvProtocolHandle
                  );
    if (EFI_ERROR (Status)) {
      break;
    }
    //
    // Call the dispatcher to dispatch any drivers from the produced firmware volume
    //
    gDS->Dispatch ();
    //
    // On to the next FV in the capsule
    //
    Length -= (UINT32) FvImage->FvLength;
    FvImage = (EFI_FIRMWARE_VOLUME_HEADER *) ((UINT8 *) FvImage + FvImage->FvLength);
  }

  return Status;
}


