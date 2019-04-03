/** @file
  Capsule Library instance to process capsule images.

  Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiDxe.h>

#include <Guid/Capsule.h>
#include <Guid/FmpCapsule.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/CapsuleLib.h>
#include <Library/GenericBdsLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/DevicePathLib.h>

#include <Protocol/FirmwareManagement.h>
#include <Protocol/DevicePath.h>


/**
  Function indicate the current completion progress of the firmware
  update. Platform may override with own specific progress function.

  @param  Completion    A value between 1 and 100 indicating the current completion progress of the firmware update

  @retval EFI_SUCESS    Input capsule is a correct FMP capsule.
**/
EFI_STATUS
EFIAPI
Update_Image_Progress (
   IN UINTN Completion
)
{
  return EFI_SUCCESS;
}


/**
  Validate Fmp capsules layout.

  @param  CapsuleHeader    Points to a capsule header.

  @retval EFI_SUCESS                     Input capsule is a correct FMP capsule.
  @retval EFI_INVALID_PARAMETER  Input capsule is not a correct FMP capsule.
**/
EFI_STATUS
ValidateFmpCapsule (
  IN EFI_CAPSULE_HEADER *CapsuleHeader
  )
{
  EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER       *FmpCapsuleHeader;
  UINT8                                        *EndOfCapsule;
  EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER *ImageHeader;
  UINT8                                        *EndOfPayload;
  UINT64                                       *ItemOffsetList;
  UINT32                                       ItemNum;
  UINTN                                        Index;

  FmpCapsuleHeader = (EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER *) ((UINT8 *) CapsuleHeader + CapsuleHeader->HeaderSize);
  EndOfCapsule     = (UINT8 *) CapsuleHeader + CapsuleHeader->CapsuleImageSize;

  if (FmpCapsuleHeader->Version > EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER_INIT_VERSION) {
    return EFI_INVALID_PARAMETER;
  }
  ItemOffsetList = (UINT64 *)(FmpCapsuleHeader + 1);

  ItemNum = FmpCapsuleHeader->EmbeddedDriverCount + FmpCapsuleHeader->PayloadItemCount;

  if (ItemNum == FmpCapsuleHeader->EmbeddedDriverCount) {
    //
    // No payload element
    //
    if (((UINT8 *)FmpCapsuleHeader + ItemOffsetList[ItemNum - 1]) < EndOfCapsule) {
      return EFI_SUCCESS;
    } else {
      return EFI_INVALID_PARAMETER;
    }
  }

  if (FmpCapsuleHeader->PayloadItemCount != 0) {
    //
    // Check if the last payload is within capsule image range
    //
    ImageHeader  = (EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER *)((UINT8 *)FmpCapsuleHeader + ItemOffsetList[ItemNum - 1]);
    EndOfPayload = (UINT8 *)(ImageHeader + 1) + ImageHeader->UpdateImageSize + ImageHeader->UpdateVendorCodeSize;
  } else {
    //
    // No driver & payload element in FMP
    //
    EndOfPayload = (UINT8 *)(FmpCapsuleHeader + 1);
  }

  if (EndOfPayload != EndOfCapsule) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // All the address in ItemOffsetList must be stored in ascending order
  //
  if (ItemNum >= 2) {
    for (Index = 0; Index < ItemNum - 1; Index++) {
      if (ItemOffsetList[Index] >= ItemOffsetList[Index + 1]) {
        return EFI_INVALID_PARAMETER;
      }
    }
  }

  return EFI_SUCCESS;
}

/**
  Process Firmware management protocol data capsule.

  @param  CapsuleHeader         Points to a capsule header.

  @retval EFI_SUCESS            Process Capsule Image successfully.
  @retval EFI_UNSUPPORTED       Capsule image is not supported by the firmware.
  @retval EFI_VOLUME_CORRUPTED  FV volume in the capsule is corrupted.
  @retval EFI_OUT_OF_RESOURCES  Not enough memory.
**/
EFI_STATUS
ProcessFmpCapsuleImage (
  IN EFI_CAPSULE_HEADER *CapsuleHeader
  )
{
  EFI_STATUS                                    Status;
  EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER        *FmpCapsuleHeader;
  EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER  *ImageHeader;
  EFI_HANDLE                                    ImageHandle;
  UINT64                                        *ItemOffsetList;
  UINT32                                        ItemNum;
  UINTN                                         Index;
  UINTN                                         ExitDataSize;
  EFI_HANDLE                                    *HandleBuffer;
  EFI_FIRMWARE_MANAGEMENT_PROTOCOL              *Fmp;
  UINTN                                         NumberOfHandles;
  UINTN                                         DescriptorSize;
  UINT8                                         FmpImageInfoCount;
  UINT32                                        FmpImageInfoDescriptorVer;
  UINTN                                         ImageInfoSize;
  UINT32                                        PackageVersion;
  CHAR16                                        *PackageVersionName;
  CHAR16                                        *AbortReason;
  EFI_FIRMWARE_IMAGE_DESCRIPTOR                 *FmpImageInfoBuf;
  EFI_FIRMWARE_IMAGE_DESCRIPTOR                 *TempFmpImageInfo;
  UINTN                                         DriverLen;
  UINTN                                         Index1;
  UINTN                                         Index2;
  MEMMAP_DEVICE_PATH                            MemMapNode;
  EFI_DEVICE_PATH_PROTOCOL                      *DriverDevicePath;

  Status           = EFI_SUCCESS;
  HandleBuffer     = NULL;
  ExitDataSize     = 0;
  DriverDevicePath = NULL;

  FmpCapsuleHeader = (EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER *) ((UINT8 *) CapsuleHeader + CapsuleHeader->HeaderSize);

  if (FmpCapsuleHeader->Version > EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER_INIT_VERSION) {
    return EFI_INVALID_PARAMETER;
  }
  ItemOffsetList = (UINT64 *)(FmpCapsuleHeader + 1);

  ItemNum = FmpCapsuleHeader->EmbeddedDriverCount + FmpCapsuleHeader->PayloadItemCount;

  //
  // capsule in which driver count and payload count are both zero is not processed.
  //
  if (ItemNum == 0) {
    return EFI_SUCCESS;
  }

  //
  // 1. ConnectAll to ensure
  //    All the communication protocol required by driver in capsule installed
  //    All FMP protocols are installed
  //
  BdsLibConnectAll();


  //
  // 2. Try to load & start all the drivers within capsule
  //
  SetDevicePathNodeLength (&MemMapNode.Header, sizeof (MemMapNode));
  MemMapNode.Header.Type     = HARDWARE_DEVICE_PATH;
  MemMapNode.Header.SubType  = HW_MEMMAP_DP;
  MemMapNode.MemoryType      = EfiBootServicesCode;
  MemMapNode.StartingAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)CapsuleHeader;
  MemMapNode.EndingAddress   = (EFI_PHYSICAL_ADDRESS)(UINTN)((UINT8 *)CapsuleHeader + CapsuleHeader->CapsuleImageSize - 1);

  DriverDevicePath = AppendDevicePathNode (NULL, &MemMapNode.Header);
  if (DriverDevicePath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  for (Index = 0; Index < FmpCapsuleHeader->EmbeddedDriverCount; Index++) {
    if (FmpCapsuleHeader->PayloadItemCount == 0 && Index == (UINTN)FmpCapsuleHeader->EmbeddedDriverCount - 1) {
      //
      // When driver is last element in the ItemOffsetList array, the driver size is calculated by reference CapsuleImageSize in EFI_CAPSULE_HEADER
      //
      DriverLen = CapsuleHeader->CapsuleImageSize - CapsuleHeader->HeaderSize - (UINTN)ItemOffsetList[Index];
    } else {
      DriverLen = (UINTN)ItemOffsetList[Index + 1] - (UINTN)ItemOffsetList[Index];
    }

    Status = gBS->LoadImage(
                    FALSE,
                    gImageHandle,
                    DriverDevicePath,
                    (UINT8 *)FmpCapsuleHeader + ItemOffsetList[Index],
                    DriverLen,
                    &ImageHandle
                    );
    if (EFI_ERROR(Status)) {
      goto EXIT;
    }

    Status = gBS->StartImage(
                    ImageHandle,
                    &ExitDataSize,
                    NULL
                    );
    if (EFI_ERROR(Status)) {
      DEBUG ((DEBUG_ERROR, "Driver Return Status = %r\n", Status));
      goto EXIT;
    }
  }

  //
  // Connnect all again to connect drivers within capsule
  //
  if (FmpCapsuleHeader->EmbeddedDriverCount > 0) {
    BdsLibConnectAll();
  }

  //
  // 3. Route payload to right FMP instance
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiFirmwareManagementProtocolGuid,
                  NULL,
                  &NumberOfHandles,
                  &HandleBuffer
                  );

  if (!EFI_ERROR(Status)) {
    for(Index1 = 0; Index1 < NumberOfHandles; Index1++) {
      Status = gBS->HandleProtocol(
                      HandleBuffer[Index1],
                      &gEfiFirmwareManagementProtocolGuid,
                      (VOID **)&Fmp
                      );
      if (EFI_ERROR(Status)) {
        continue;
      }

      ImageInfoSize = 0;
      Status = Fmp->GetImageInfo (
                      Fmp,
                      &ImageInfoSize,
                      NULL,
                      NULL,
                      NULL,
                      NULL,
                      NULL,
                      NULL
                      );
      if (Status != EFI_BUFFER_TOO_SMALL) {
        continue;
      }

      FmpImageInfoBuf = NULL;
      FmpImageInfoBuf = AllocateZeroPool (ImageInfoSize);
      if (FmpImageInfoBuf == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto EXIT;
      }

      PackageVersionName = NULL;
      Status = Fmp->GetImageInfo (
                      Fmp,
                      &ImageInfoSize,               // ImageInfoSize
                      FmpImageInfoBuf,              // ImageInfo
                      &FmpImageInfoDescriptorVer,   // DescriptorVersion
                      &FmpImageInfoCount,           // DescriptorCount
                      &DescriptorSize,              // DescriptorSize
                      &PackageVersion,              // PackageVersion
                      &PackageVersionName           // PackageVersionName
                      );

      //
      // If FMP GetInformation interface failed, skip this resource
      //
      if (EFI_ERROR(Status)) {
        FreePool(FmpImageInfoBuf);
        continue;
      }

      if (PackageVersionName != NULL) {
        FreePool(PackageVersionName);
      }

      TempFmpImageInfo = FmpImageInfoBuf;
      for (Index2 = 0; Index2 < FmpImageInfoCount; Index2++) {
        //
        // Check all the payload entry in capsule payload list
        //
        for (Index = FmpCapsuleHeader->EmbeddedDriverCount; Index < ItemNum; Index++) {
          ImageHeader  = (EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER *)((UINT8 *)FmpCapsuleHeader + ItemOffsetList[Index]);
          if (CompareGuid(&ImageHeader->UpdateImageTypeId, &TempFmpImageInfo->ImageTypeId) &&
              ImageHeader->UpdateImageIndex == TempFmpImageInfo->ImageIndex) {
            AbortReason = NULL;
            if (ImageHeader->UpdateVendorCodeSize == 0) {
              Status = Fmp->SetImage(
                              Fmp,
                              TempFmpImageInfo->ImageIndex,           // ImageIndex
                              (UINT8 *)(ImageHeader + 1),             // Image
                              ImageHeader->UpdateImageSize,           // ImageSize
                              NULL,                                   // VendorCode
                              Update_Image_Progress,                  // Progress
                              &AbortReason                            // AbortReason
                              );
            } else {
              Status = Fmp->SetImage(
                              Fmp,
                              TempFmpImageInfo->ImageIndex,                                          // ImageIndex
                              (UINT8 *)(ImageHeader + 1),                                            // Image
                              ImageHeader->UpdateImageSize,                                          // ImageSize
                              (UINT8 *)((UINT8 *) (ImageHeader + 1) + ImageHeader->UpdateImageSize), // VendorCode
                              Update_Image_Progress,                                                 // Progress
                              &AbortReason                                                           // AbortReason
                              );
            }
            if (AbortReason != NULL) {
              DEBUG ((EFI_D_ERROR, "%s\n", AbortReason));
              FreePool(AbortReason);
            }
          }
        }
        //
        // Use DescriptorSize to move ImageInfo Pointer to stay compatible with different ImageInfo version
        //
        TempFmpImageInfo = (EFI_FIRMWARE_IMAGE_DESCRIPTOR *)((UINT8 *)TempFmpImageInfo + DescriptorSize);
      }
      FreePool(FmpImageInfoBuf);
    }
  }

EXIT:

  if (HandleBuffer != NULL) {
    FreePool(HandleBuffer);
  }

  if (DriverDevicePath != NULL) {
    FreePool(DriverDevicePath);
  }

  return Status;
}

/**
  Those capsules supported by the firmwares.

  @param  CapsuleHeader    Points to a capsule header.

  @retval EFI_SUCESS       Input capsule is supported by firmware.
  @retval EFI_UNSUPPORTED  Input capsule is not supported by the firmware.
  @retval EFI_INVALID_PARAMETER Input capsule layout is not correct
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

  if (CompareGuid (&gEfiFmpCapsuleGuid, &CapsuleHeader->CapsuleGuid)) {
    //
    // Check layout of FMP capsule
    //
    return ValidateFmpCapsule(CapsuleHeader);
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
  // Check FMP capsule layout
  //
  if (CompareGuid (&gEfiFmpCapsuleGuid, &CapsuleHeader->CapsuleGuid)){
    Status = ValidateFmpCapsule(CapsuleHeader);
    if (EFI_ERROR(Status)) {
      return Status;
    }

    //
    // Press EFI FMP Capsule
    //
    return ProcessFmpCapsuleImage(CapsuleHeader);
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

/**

  This routine is called to process capsules.

  Caution: This function may receive untrusted input.

  The capsules reported in EFI_HOB_UEFI_CAPSULE are processed.
  If there is no EFI_HOB_UEFI_CAPSULE, this routine does nothing.

  This routine should be called twice in BDS.
  1) The first call must be before EndOfDxe. The system capsules is processed.
     If device capsule FMP protocols are exposted at this time and device FMP
     capsule has zero EmbeddedDriverCount, the device capsules are processed.
     Each individual capsule result is recorded in capsule record variable.
     System may reset in this function, if reset is required by capsule and
     all capsules are processed.
     If not all capsules are processed, reset will be defered to second call.

  2) The second call must be after EndOfDxe and after ConnectAll, so that all
     device capsule FMP protocols are exposed.
     The system capsules are skipped. If the device capsules are NOT processed
     in first call, they are processed here.
     Each individual capsule result is recorded in capsule record variable.
     System may reset in this function, if reset is required by capsule
     processed in first call and second call.

  @retval EFI_SUCCESS             There is no error when processing capsules.
  @retval EFI_OUT_OF_RESOURCES    No enough resource to process capsules.

**/
EFI_STATUS
EFIAPI
ProcessCapsules (
  VOID
  )
{
  return EFI_UNSUPPORTED;
}

