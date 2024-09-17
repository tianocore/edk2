/** @file
  DXE capsule library.

  Caution: This module requires additional review when modified.
  This module will have external input - capsule image.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.

  SupportCapsuleImage(), ProcessCapsuleImage(), IsValidCapsuleHeader(),
  ValidateFmpCapsule(), and DisplayCapsuleImage() receives untrusted input and
  performs basic validation.

  Copyright (c) 2016 - 2024, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <IndustryStandard/WindowsUxCapsule.h>

#include <Guid/FmpCapsule.h>
#include <Guid/SystemResourceTable.h>
#include <Guid/EventGroup.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/CapsuleLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiLib.h>
#include <Library/BmpSupportLib.h>

#include <Protocol/GraphicsOutput.h>
#include <Protocol/EsrtManagement.h>
#include <Protocol/FirmwareManagement.h>
#include <Protocol/FirmwareManagementProgress.h>
#include <Protocol/DevicePath.h>

EFI_SYSTEM_RESOURCE_TABLE  *mEsrtTable = NULL;

BOOLEAN    mDxeCapsuleLibEndOfDxe      = FALSE;
EFI_EVENT  mDxeCapsuleLibEndOfDxeEvent = NULL;

EDKII_FIRMWARE_MANAGEMENT_PROGRESS_PROTOCOL  *mFmpProgress = NULL;

BOOLEAN  mDxeCapsuleLibIsExitBootService = FALSE;

/**
  Initialize capsule related variables.
**/
VOID
InitCapsuleVariable (
  VOID
  );

/**
  Record capsule status variable.

  @param[in] CapsuleHeader  The capsule image header
  @param[in] CapsuleStatus  The capsule process stauts

  @retval EFI_SUCCESS          The capsule status variable is recorded.
  @retval EFI_OUT_OF_RESOURCES No resource to record the capsule status variable.
**/
EFI_STATUS
RecordCapsuleStatusVariable (
  IN EFI_CAPSULE_HEADER  *CapsuleHeader,
  IN EFI_STATUS          CapsuleStatus
  );

/**
  Record FMP capsule status variable.

  @param[in] CapsuleHeader  The capsule image header
  @param[in] CapsuleStatus  The capsule process stauts
  @param[in] PayloadIndex   FMP payload index
  @param[in] ImageHeader    FMP image header
  @param[in] FmpDevicePath  DevicePath associated with the FMP producer
  @param[in] CapFileName    Capsule file name

  @retval EFI_SUCCESS          The capsule status variable is recorded.
  @retval EFI_OUT_OF_RESOURCES No resource to record the capsule status variable.
**/
EFI_STATUS
RecordFmpCapsuleStatusVariable (
  IN EFI_CAPSULE_HEADER                            *CapsuleHeader,
  IN EFI_STATUS                                    CapsuleStatus,
  IN UINTN                                         PayloadIndex,
  IN EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER  *ImageHeader,
  IN EFI_DEVICE_PATH_PROTOCOL                      *FmpDevicePath  OPTIONAL,
  IN CHAR16                                        *CapFileName    OPTIONAL
  );

/**
  Function indicate the current completion progress of the firmware
  update. Platform may override with own specific progress function.

  @param[in]  Completion  A value between 1 and 100 indicating the current
                          completion progress of the firmware update

  @retval EFI_SUCESS             The capsule update progress was updated.
  @retval EFI_INVALID_PARAMETER  Completion is greater than 100%.
**/
EFI_STATUS
EFIAPI
UpdateImageProgress (
  IN UINTN  Completion
  );

/**
  Return if this capsule is a capsule name capsule, based upon CapsuleHeader.

  @param[in] CapsuleHeader A pointer to EFI_CAPSULE_HEADER

  @retval TRUE  It is a capsule name capsule.
  @retval FALSE It is not a capsule name capsule.
**/
BOOLEAN
IsCapsuleNameCapsule (
  IN EFI_CAPSULE_HEADER  *CapsuleHeader
  )
{
  return CompareGuid (&CapsuleHeader->CapsuleGuid, &gEdkiiCapsuleOnDiskNameGuid);
}

/**
  Return if this CapsuleGuid is a FMP capsule GUID or not.

  @param[in] CapsuleGuid A pointer to EFI_GUID

  @retval TRUE  It is a FMP capsule GUID.
  @retval FALSE It is not a FMP capsule GUID.
**/
BOOLEAN
IsFmpCapsuleGuid (
  IN EFI_GUID  *CapsuleGuid
  )
{
  if (CompareGuid (&gEfiFmpCapsuleGuid, CapsuleGuid)) {
    return TRUE;
  }

  return FALSE;
}

/**
  Validate if it is valid capsule header

  Caution: This function may receive untrusted input.

  This function assumes the caller provided correct CapsuleHeader pointer
  and CapsuleSize.

  This function validates the fields in EFI_CAPSULE_HEADER.

  @param[in]  CapsuleHeader    Points to a capsule header.
  @param[in]  CapsuleSize      Size of the whole capsule image.

**/
BOOLEAN
IsValidCapsuleHeader (
  IN EFI_CAPSULE_HEADER  *CapsuleHeader,
  IN UINT64              CapsuleSize
  )
{
  if (CapsuleHeader->CapsuleImageSize != CapsuleSize) {
    return FALSE;
  }

  if (CapsuleHeader->HeaderSize >= CapsuleHeader->CapsuleImageSize) {
    return FALSE;
  }

  return TRUE;
}

/**
  Validate Fmp capsules layout.

  Caution: This function may receive untrusted input.

  This function assumes the caller validated the capsule by using
  IsValidCapsuleHeader(), so that all fields in EFI_CAPSULE_HEADER are correct.
  The capsule buffer size is CapsuleHeader->CapsuleImageSize.

  This function validates the fields in EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER
  and EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER.

  This function need support nested FMP capsule.

  @param[in]   CapsuleHeader        Points to a capsule header.
  @param[out]  EmbeddedDriverCount  The EmbeddedDriverCount in the FMP capsule.

  @retval EFI_SUCESS             Input capsule is a correct FMP capsule.
  @retval EFI_INVALID_PARAMETER  Input capsule is not a correct FMP capsule.
**/
EFI_STATUS
ValidateFmpCapsule (
  IN EFI_CAPSULE_HEADER  *CapsuleHeader,
  OUT UINT16             *EmbeddedDriverCount OPTIONAL
  )
{
  EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER        *FmpCapsuleHeader;
  UINT8                                         *EndOfCapsule;
  EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER  *ImageHeader;
  UINT8                                         *EndOfPayload;
  UINT64                                        *ItemOffsetList;
  UINT32                                        ItemNum;
  UINTN                                         Index;
  UINTN                                         FmpCapsuleSize;
  UINTN                                         FmpCapsuleHeaderSize;
  UINT64                                        FmpImageSize;
  UINTN                                         FmpImageHeaderSize;

  if (!IsFmpCapsuleGuid (&CapsuleHeader->CapsuleGuid)) {
    return ValidateFmpCapsule ((EFI_CAPSULE_HEADER *)((UINTN)CapsuleHeader + CapsuleHeader->HeaderSize), EmbeddedDriverCount);
  }

  if (CapsuleHeader->HeaderSize >= CapsuleHeader->CapsuleImageSize) {
    DEBUG ((DEBUG_ERROR, "HeaderSize(0x%x) >= CapsuleImageSize(0x%x)\n", CapsuleHeader->HeaderSize, CapsuleHeader->CapsuleImageSize));
    return EFI_INVALID_PARAMETER;
  }

  FmpCapsuleHeader = (EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER *)((UINT8 *)CapsuleHeader + CapsuleHeader->HeaderSize);
  EndOfCapsule     = (UINT8 *)CapsuleHeader + CapsuleHeader->CapsuleImageSize;
  FmpCapsuleSize   = (UINTN)EndOfCapsule - (UINTN)FmpCapsuleHeader;

  if (FmpCapsuleSize < sizeof (EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER)) {
    DEBUG ((DEBUG_ERROR, "FmpCapsuleSize(0x%x) < EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER\n", FmpCapsuleSize));
    return EFI_INVALID_PARAMETER;
  }

  // Check EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER
  if (FmpCapsuleHeader->Version != EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER_INIT_VERSION) {
    DEBUG ((DEBUG_ERROR, "FmpCapsuleHeader->Version(0x%x) != EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER_INIT_VERSION\n", FmpCapsuleHeader->Version));
    return EFI_INVALID_PARAMETER;
  }

  ItemOffsetList = (UINT64 *)(FmpCapsuleHeader + 1);

  // No overflow
  ItemNum = FmpCapsuleHeader->EmbeddedDriverCount + FmpCapsuleHeader->PayloadItemCount;

  if ((FmpCapsuleSize - sizeof (EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER))/sizeof (UINT64) < ItemNum) {
    DEBUG ((DEBUG_ERROR, "ItemNum(0x%x) too big\n", ItemNum));
    return EFI_INVALID_PARAMETER;
  }

  FmpCapsuleHeaderSize = sizeof (EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER) + sizeof (UINT64)*ItemNum;

  // Check ItemOffsetList
  for (Index = 0; Index < ItemNum; Index++) {
    if (ItemOffsetList[Index] >= FmpCapsuleSize) {
      DEBUG ((DEBUG_ERROR, "ItemOffsetList[%d](0x%lx) >= FmpCapsuleSize(0x%x)\n", Index, ItemOffsetList[Index], FmpCapsuleSize));
      return EFI_INVALID_PARAMETER;
    }

    if (ItemOffsetList[Index] < FmpCapsuleHeaderSize) {
      DEBUG ((DEBUG_ERROR, "ItemOffsetList[%d](0x%lx) < FmpCapsuleHeaderSize(0x%x)\n", Index, ItemOffsetList[Index], FmpCapsuleHeaderSize));
      return EFI_INVALID_PARAMETER;
    }

    //
    // All the address in ItemOffsetList must be stored in ascending order
    //
    if (Index > 0) {
      if (ItemOffsetList[Index] <= ItemOffsetList[Index - 1]) {
        DEBUG ((DEBUG_ERROR, "ItemOffsetList[%d](0x%lx) < ItemOffsetList[%d](0x%x)\n", Index, ItemOffsetList[Index], Index - 1, ItemOffsetList[Index - 1]));
        return EFI_INVALID_PARAMETER;
      }
    }
  }

  // Check EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER
  for (Index = FmpCapsuleHeader->EmbeddedDriverCount; Index < ItemNum; Index++) {
    ImageHeader = (EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER *)((UINT8 *)FmpCapsuleHeader + ItemOffsetList[Index]);
    if (Index == ItemNum - 1) {
      EndOfPayload = (UINT8 *)((UINTN)EndOfCapsule - (UINTN)FmpCapsuleHeader);
    } else {
      EndOfPayload = (UINT8 *)(UINTN)ItemOffsetList[Index+1];
    }

    FmpImageSize = (UINTN)EndOfPayload - ItemOffsetList[Index];

    FmpImageHeaderSize = sizeof (EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER);
    if ((ImageHeader->Version > EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER_INIT_VERSION) ||
        (ImageHeader->Version < 1))
    {
      DEBUG ((DEBUG_ERROR, "ImageHeader->Version(0x%x) Unknown\n", ImageHeader->Version));
      return EFI_INVALID_PARAMETER;
    }

    if (ImageHeader->Version == 1) {
      FmpImageHeaderSize = OFFSET_OF (EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER, UpdateHardwareInstance);
    } else if (ImageHeader->Version == 2) {
      FmpImageHeaderSize = OFFSET_OF (EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER, ImageCapsuleSupport);
    }

    if (FmpImageSize < FmpImageHeaderSize) {
      DEBUG ((DEBUG_ERROR, "FmpImageSize(0x%lx) < FmpImageHeaderSize(0x%x)\n", FmpImageSize, FmpImageHeaderSize));
      return EFI_INVALID_PARAMETER;
    }

    // No overflow
    if (FmpImageSize != (UINT64)FmpImageHeaderSize + (UINT64)ImageHeader->UpdateImageSize + (UINT64)ImageHeader->UpdateVendorCodeSize) {
      DEBUG ((DEBUG_ERROR, "FmpImageSize(0x%lx) mismatch, UpdateImageSize(0x%x) UpdateVendorCodeSize(0x%x)\n", FmpImageSize, ImageHeader->UpdateImageSize, ImageHeader->UpdateVendorCodeSize));
      return EFI_INVALID_PARAMETER;
    }
  }

  if (ItemNum == 0) {
    //
    // No driver & payload element in FMP
    //
    EndOfPayload = (UINT8 *)(FmpCapsuleHeader + 1);
    if (EndOfPayload != EndOfCapsule) {
      DEBUG ((DEBUG_ERROR, "EndOfPayload(0x%x) mismatch, EndOfCapsule(0x%x)\n", EndOfPayload, EndOfCapsule));
      return EFI_INVALID_PARAMETER;
    }

    return EFI_UNSUPPORTED;
  }

  if (EmbeddedDriverCount != NULL) {
    *EmbeddedDriverCount = FmpCapsuleHeader->EmbeddedDriverCount;
  }

  return EFI_SUCCESS;
}

/**
  Those capsules supported by the firmwares.

  Caution: This function may receive untrusted input.

  @param[in]  CapsuleHeader    Points to a capsule header.

  @retval EFI_SUCESS       Input capsule is supported by firmware.
  @retval EFI_UNSUPPORTED  Input capsule is not supported by the firmware.
**/
EFI_STATUS
DisplayCapsuleImage (
  IN EFI_CAPSULE_HEADER  *CapsuleHeader
  )
{
  DISPLAY_DISPLAY_PAYLOAD        *ImagePayload;
  UINTN                          PayloadSize;
  EFI_STATUS                     Status;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *Blt;
  UINTN                          BltSize;
  UINTN                          Height;
  UINTN                          Width;
  EFI_GRAPHICS_OUTPUT_PROTOCOL   *GraphicsOutput;

  //
  // UX capsule doesn't have extended header entries.
  //
  if (CapsuleHeader->HeaderSize != sizeof (EFI_CAPSULE_HEADER)) {
    return EFI_UNSUPPORTED;
  }

  ImagePayload = (DISPLAY_DISPLAY_PAYLOAD *)((UINTN)CapsuleHeader + CapsuleHeader->HeaderSize);
  //
  // (CapsuleImageSize > HeaderSize) is guaranteed by IsValidCapsuleHeader().
  //
  PayloadSize = CapsuleHeader->CapsuleImageSize - CapsuleHeader->HeaderSize;

  //
  // Make sure the image payload at least contain the DISPLAY_DISPLAY_PAYLOAD header.
  // Further size check is performed by the logic translating BMP to GOP BLT.
  //
  if (PayloadSize <= sizeof (DISPLAY_DISPLAY_PAYLOAD)) {
    return EFI_INVALID_PARAMETER;
  }

  if (ImagePayload->Version != 1) {
    return EFI_UNSUPPORTED;
  }

  if (CalculateCheckSum8 ((UINT8 *)CapsuleHeader, CapsuleHeader->CapsuleImageSize) != 0) {
    return EFI_UNSUPPORTED;
  }

  //
  // Only Support Bitmap by now
  //
  if (ImagePayload->ImageType != 0) {
    return EFI_UNSUPPORTED;
  }

  //
  // Try to open GOP
  //
  Status = gBS->HandleProtocol (gST->ConsoleOutHandle, &gEfiGraphicsOutputProtocolGuid, (VOID **)&GraphicsOutput);
  if (EFI_ERROR (Status)) {
    Status = gBS->LocateProtocol (&gEfiGraphicsOutputProtocolGuid, NULL, (VOID **)&GraphicsOutput);
    if (EFI_ERROR (Status)) {
      return EFI_UNSUPPORTED;
    }
  }

  if (GraphicsOutput->Mode->Mode != ImagePayload->Mode) {
    return EFI_UNSUPPORTED;
  }

  Blt    = NULL;
  Width  = 0;
  Height = 0;
  Status = TranslateBmpToGopBlt (
             ImagePayload + 1,
             PayloadSize - sizeof (DISPLAY_DISPLAY_PAYLOAD),
             &Blt,
             &BltSize,
             &Height,
             &Width
             );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = GraphicsOutput->Blt (
                             GraphicsOutput,
                             Blt,
                             EfiBltBufferToVideo,
                             0,
                             0,
                             (UINTN)ImagePayload->OffsetX,
                             (UINTN)ImagePayload->OffsetY,
                             Width,
                             Height,
                             Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
                             );

  FreePool (Blt);

  return Status;
}

/**
  Dump FMP information.

  @param[in] ImageInfoSize       The size of ImageInfo, in bytes.
  @param[in] ImageInfo           A pointer to EFI_FIRMWARE_IMAGE_DESCRIPTOR.
  @param[in] DescriptorVersion   The version of EFI_FIRMWARE_IMAGE_DESCRIPTOR.
  @param[in] DescriptorCount     The count of EFI_FIRMWARE_IMAGE_DESCRIPTOR.
  @param[in] DescriptorSize      The size of an individual EFI_FIRMWARE_IMAGE_DESCRIPTOR, in bytes.
  @param[in] PackageVersion      The version of package.
  @param[in] PackageVersionName  The version name of package.
**/
VOID
DumpFmpImageInfo (
  IN UINTN                          ImageInfoSize,
  IN EFI_FIRMWARE_IMAGE_DESCRIPTOR  *ImageInfo,
  IN UINT32                         DescriptorVersion,
  IN UINT8                          DescriptorCount,
  IN UINTN                          DescriptorSize,
  IN UINT32                         PackageVersion,
  IN CHAR16                         *PackageVersionName
  )
{
  EFI_FIRMWARE_IMAGE_DESCRIPTOR  *CurrentImageInfo;
  UINTN                          Index;

  DEBUG ((DEBUG_VERBOSE, "  DescriptorVersion  - 0x%x\n", DescriptorVersion));
  DEBUG ((DEBUG_VERBOSE, "  DescriptorCount    - 0x%x\n", DescriptorCount));
  DEBUG ((DEBUG_VERBOSE, "  DescriptorSize     - 0x%x\n", DescriptorSize));
  DEBUG ((DEBUG_VERBOSE, "  PackageVersion     - 0x%x\n", PackageVersion));
  DEBUG ((DEBUG_VERBOSE, "  PackageVersionName - %s\n\n", PackageVersionName));
  CurrentImageInfo = ImageInfo;
  for (Index = 0; Index < DescriptorCount; Index++) {
    DEBUG ((DEBUG_VERBOSE, "  ImageDescriptor (%d)\n", Index));
    DEBUG ((DEBUG_VERBOSE, "    ImageIndex                  - 0x%x\n", CurrentImageInfo->ImageIndex));
    DEBUG ((DEBUG_VERBOSE, "    ImageTypeId                 - %g\n", &CurrentImageInfo->ImageTypeId));
    DEBUG ((DEBUG_VERBOSE, "    ImageId                     - 0x%lx\n", CurrentImageInfo->ImageId));
    DEBUG ((DEBUG_VERBOSE, "    ImageIdName                 - %s\n", CurrentImageInfo->ImageIdName));
    DEBUG ((DEBUG_VERBOSE, "    Version                     - 0x%x\n", CurrentImageInfo->Version));
    DEBUG ((DEBUG_VERBOSE, "    VersionName                 - %s\n", CurrentImageInfo->VersionName));
    DEBUG ((DEBUG_VERBOSE, "    Size                        - 0x%x\n", CurrentImageInfo->Size));
    DEBUG ((DEBUG_VERBOSE, "    AttributesSupported         - 0x%lx\n", CurrentImageInfo->AttributesSupported));
    DEBUG ((DEBUG_VERBOSE, "    AttributesSetting           - 0x%lx\n", CurrentImageInfo->AttributesSetting));
    DEBUG ((DEBUG_VERBOSE, "    Compatibilities             - 0x%lx\n", CurrentImageInfo->Compatibilities));
    if (DescriptorVersion > 1) {
      DEBUG ((DEBUG_VERBOSE, "    LowestSupportedImageVersion - 0x%x\n", CurrentImageInfo->LowestSupportedImageVersion));
      if (DescriptorVersion > 2) {
        DEBUG ((DEBUG_VERBOSE, "    LastAttemptVersion          - 0x%x\n", CurrentImageInfo->LastAttemptVersion));
        DEBUG ((DEBUG_VERBOSE, "    LastAttemptStatus           - 0x%x\n", CurrentImageInfo->LastAttemptStatus));
        DEBUG ((DEBUG_VERBOSE, "    HardwareInstance            - 0x%lx\n", CurrentImageInfo->HardwareInstance));
      }
    }

    //
    // Use DescriptorSize to move ImageInfo Pointer to stay compatible with different ImageInfo version
    //
    CurrentImageInfo = (EFI_FIRMWARE_IMAGE_DESCRIPTOR *)((UINT8 *)CurrentImageInfo + DescriptorSize);
  }
}

/**
  Dump a non-nested FMP capsule.

  @param[in]  CapsuleHeader  A pointer to CapsuleHeader
**/
VOID
DumpFmpCapsule (
  IN EFI_CAPSULE_HEADER  *CapsuleHeader
  )
{
  EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER        *FmpCapsuleHeader;
  EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER  *ImageHeader;
  UINTN                                         Index;
  UINT64                                        *ItemOffsetList;

  FmpCapsuleHeader = (EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER *)((UINT8 *)CapsuleHeader + CapsuleHeader->HeaderSize);

  DEBUG ((DEBUG_VERBOSE, "FmpCapsule:\n"));
  DEBUG ((DEBUG_VERBOSE, "  Version                - 0x%x\n", FmpCapsuleHeader->Version));
  DEBUG ((DEBUG_VERBOSE, "  EmbeddedDriverCount    - 0x%x\n", FmpCapsuleHeader->EmbeddedDriverCount));
  DEBUG ((DEBUG_VERBOSE, "  PayloadItemCount       - 0x%x\n", FmpCapsuleHeader->PayloadItemCount));

  ItemOffsetList = (UINT64 *)(FmpCapsuleHeader + 1);
  for (Index = 0; Index < FmpCapsuleHeader->EmbeddedDriverCount; Index++) {
    DEBUG ((DEBUG_VERBOSE, "  ItemOffsetList[%d]      - 0x%lx\n", Index, ItemOffsetList[Index]));
  }

  for ( ; Index < (UINT32)FmpCapsuleHeader->EmbeddedDriverCount + FmpCapsuleHeader->PayloadItemCount; Index++) {
    DEBUG ((DEBUG_VERBOSE, "  ItemOffsetList[%d]      - 0x%lx\n", Index, ItemOffsetList[Index]));
    ImageHeader = (EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER *)((UINT8 *)FmpCapsuleHeader + ItemOffsetList[Index]);

    DEBUG ((DEBUG_VERBOSE, "  ImageHeader:\n"));
    DEBUG ((DEBUG_VERBOSE, "    Version                - 0x%x\n", ImageHeader->Version));
    DEBUG ((DEBUG_VERBOSE, "    UpdateImageTypeId      - %g\n", &ImageHeader->UpdateImageTypeId));
    DEBUG ((DEBUG_VERBOSE, "    UpdateImageIndex       - 0x%x\n", ImageHeader->UpdateImageIndex));
    DEBUG ((DEBUG_VERBOSE, "    UpdateImageSize        - 0x%x\n", ImageHeader->UpdateImageSize));
    DEBUG ((DEBUG_VERBOSE, "    UpdateVendorCodeSize   - 0x%x\n", ImageHeader->UpdateVendorCodeSize));
    if (ImageHeader->Version >= 2) {
      DEBUG ((DEBUG_VERBOSE, "    UpdateHardwareInstance - 0x%lx\n", ImageHeader->UpdateHardwareInstance));
      if (ImageHeader->Version >= EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER_INIT_VERSION) {
        DEBUG ((DEBUG_VERBOSE, "    ImageCapsuleSupport    - 0x%lx\n", ImageHeader->ImageCapsuleSupport));
      }
    }
  }
}

/**
  Dump all FMP information.
**/
VOID
DumpAllFmpInfo (
  VOID
  )
{
  EFI_STATUS                        Status;
  EFI_HANDLE                        *HandleBuffer;
  UINTN                             NumberOfHandles;
  EFI_FIRMWARE_MANAGEMENT_PROTOCOL  *Fmp;
  UINTN                             Index;
  UINTN                             ImageInfoSize;
  EFI_FIRMWARE_IMAGE_DESCRIPTOR     *FmpImageInfoBuf;
  UINT32                            FmpImageInfoDescriptorVer;
  UINT8                             FmpImageInfoCount;
  UINTN                             DescriptorSize;
  UINT32                            PackageVersion;
  CHAR16                            *PackageVersionName;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiFirmwareManagementProtocolGuid,
                  NULL,
                  &NumberOfHandles,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return;
  }

  for (Index = 0; Index < NumberOfHandles; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiFirmwareManagementProtocolGuid,
                    (VOID **)&Fmp
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    ImageInfoSize = 0;
    Status        = Fmp->GetImageInfo (
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

    FmpImageInfoBuf = AllocateZeroPool (ImageInfoSize);
    if (FmpImageInfoBuf == NULL) {
      continue;
    }

    PackageVersionName = NULL;
    Status             = Fmp->GetImageInfo (
                                Fmp,
                                &ImageInfoSize,             // ImageInfoSize
                                FmpImageInfoBuf,            // ImageInfo
                                &FmpImageInfoDescriptorVer, // DescriptorVersion
                                &FmpImageInfoCount,         // DescriptorCount
                                &DescriptorSize,            // DescriptorSize
                                &PackageVersion,            // PackageVersion
                                &PackageVersionName         // PackageVersionName
                                );
    if (EFI_ERROR (Status)) {
      FreePool (FmpImageInfoBuf);
      continue;
    }

    DEBUG ((DEBUG_INFO, "FMP (%d) ImageInfo:\n", Index));
    DumpFmpImageInfo (
      ImageInfoSize,               // ImageInfoSize
      FmpImageInfoBuf,             // ImageInfo
      FmpImageInfoDescriptorVer,   // DescriptorVersion
      FmpImageInfoCount,           // DescriptorCount
      DescriptorSize,              // DescriptorSize
      PackageVersion,              // PackageVersion
      PackageVersionName           // PackageVersionName
      );

    if (PackageVersionName != NULL) {
      FreePool (PackageVersionName);
    }

    FreePool (FmpImageInfoBuf);
  }

  FreePool (HandleBuffer);

  return;
}

/**
  Get FMP handle by ImageTypeId and HardwareInstance.

  @param[in]     UpdateImageTypeId       Used to identify device firmware targeted by this update.
  @param[in]     UpdateHardwareInstance  The HardwareInstance to target with this update.
  @param[out]    NoHandles               The number of handles returned in HandleBuf.
  @param[out]    HandleBuf               A pointer to the buffer to return the requested array of handles.
  @param[out]    ResetRequiredBuf        A pointer to the buffer to return reset required flag for
                                         the requested array of handles.

  @retval EFI_SUCCESS            The array of handles and their reset required flag were returned in
                                 HandleBuf and ResetRequiredBuf, and the number of handles in HandleBuf
                                 was returned in NoHandles.
  @retval EFI_NOT_FOUND          No handles match the search.
  @retval EFI_OUT_OF_RESOURCES   There is not enough pool memory to store the matching results.
**/
EFI_STATUS
GetFmpHandleBufferByType (
  IN     EFI_GUID    *UpdateImageTypeId,
  IN     UINT64      UpdateHardwareInstance,
  OUT    UINTN       *NoHandles  OPTIONAL,
  OUT    EFI_HANDLE  **HandleBuf  OPTIONAL,
  OUT    BOOLEAN     **ResetRequiredBuf OPTIONAL
  )
{
  EFI_STATUS                        Status;
  EFI_HANDLE                        *HandleBuffer;
  UINTN                             NumberOfHandles;
  EFI_HANDLE                        *MatchedHandleBuffer;
  BOOLEAN                           *MatchedResetRequiredBuffer;
  UINTN                             MatchedNumberOfHandles;
  EFI_FIRMWARE_MANAGEMENT_PROTOCOL  *Fmp;
  UINTN                             Index;
  UINTN                             ImageInfoSize;
  EFI_FIRMWARE_IMAGE_DESCRIPTOR     *FmpImageInfoBuf;
  UINT32                            FmpImageInfoDescriptorVer;
  UINT8                             FmpImageInfoCount;
  UINTN                             DescriptorSize;
  UINT32                            PackageVersion;
  CHAR16                            *PackageVersionName;
  UINTN                             Index2;
  EFI_FIRMWARE_IMAGE_DESCRIPTOR     *TempFmpImageInfo;

  if (NoHandles != NULL) {
    *NoHandles = 0;
  }

  if (HandleBuf != NULL) {
    *HandleBuf = NULL;
  }

  if (ResetRequiredBuf != NULL) {
    *ResetRequiredBuf = NULL;
  }

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiFirmwareManagementProtocolGuid,
                  NULL,
                  &NumberOfHandles,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  MatchedNumberOfHandles = 0;

  MatchedHandleBuffer = NULL;
  if (HandleBuf != NULL) {
    MatchedHandleBuffer = AllocateZeroPool (sizeof (EFI_HANDLE) * NumberOfHandles);
    if (MatchedHandleBuffer == NULL) {
      FreePool (HandleBuffer);
      return EFI_OUT_OF_RESOURCES;
    }
  }

  MatchedResetRequiredBuffer = NULL;
  if (ResetRequiredBuf != NULL) {
    MatchedResetRequiredBuffer = AllocateZeroPool (sizeof (BOOLEAN) * NumberOfHandles);
    if (MatchedResetRequiredBuffer == NULL) {
      if (MatchedHandleBuffer != NULL) {
        FreePool (MatchedHandleBuffer);
      }

      FreePool (HandleBuffer);
      return EFI_OUT_OF_RESOURCES;
    }
  }

  for (Index = 0; Index < NumberOfHandles; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiFirmwareManagementProtocolGuid,
                    (VOID **)&Fmp
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    ImageInfoSize = 0;
    Status        = Fmp->GetImageInfo (
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

    FmpImageInfoBuf = AllocateZeroPool (ImageInfoSize);
    if (FmpImageInfoBuf == NULL) {
      continue;
    }

    PackageVersionName = NULL;
    Status             = Fmp->GetImageInfo (
                                Fmp,
                                &ImageInfoSize,             // ImageInfoSize
                                FmpImageInfoBuf,            // ImageInfo
                                &FmpImageInfoDescriptorVer, // DescriptorVersion
                                &FmpImageInfoCount,         // DescriptorCount
                                &DescriptorSize,            // DescriptorSize
                                &PackageVersion,            // PackageVersion
                                &PackageVersionName         // PackageVersionName
                                );
    if (EFI_ERROR (Status)) {
      FreePool (FmpImageInfoBuf);
      continue;
    }

    if (PackageVersionName != NULL) {
      FreePool (PackageVersionName);
    }

    TempFmpImageInfo = FmpImageInfoBuf;
    for (Index2 = 0; Index2 < FmpImageInfoCount; Index2++) {
      //
      // Check if this FMP instance matches
      //
      if (CompareGuid (UpdateImageTypeId, &TempFmpImageInfo->ImageTypeId)) {
        if ((UpdateHardwareInstance == 0) ||
            ((FmpImageInfoDescriptorVer >= EFI_FIRMWARE_IMAGE_DESCRIPTOR_VERSION) &&
             (UpdateHardwareInstance == TempFmpImageInfo->HardwareInstance)))
        {
          if (MatchedHandleBuffer != NULL) {
            MatchedHandleBuffer[MatchedNumberOfHandles] = HandleBuffer[Index];
          }

          if (MatchedResetRequiredBuffer != NULL) {
            MatchedResetRequiredBuffer[MatchedNumberOfHandles] = (((TempFmpImageInfo->AttributesSupported &
                                                                    IMAGE_ATTRIBUTE_RESET_REQUIRED) != 0) &&
                                                                  ((TempFmpImageInfo->AttributesSetting &
                                                                    IMAGE_ATTRIBUTE_RESET_REQUIRED) != 0));
          }

          MatchedNumberOfHandles++;
          break;
        }
      }

      TempFmpImageInfo = (EFI_FIRMWARE_IMAGE_DESCRIPTOR *)((UINT8 *)TempFmpImageInfo + DescriptorSize);
    }

    FreePool (FmpImageInfoBuf);
  }

  FreePool (HandleBuffer);

  if (MatchedNumberOfHandles == 0) {
    return EFI_NOT_FOUND;
  }

  if (NoHandles != NULL) {
    *NoHandles = MatchedNumberOfHandles;
  }

  if (HandleBuf != NULL) {
    *HandleBuf = MatchedHandleBuffer;
  }

  if (ResetRequiredBuf != NULL) {
    *ResetRequiredBuf = MatchedResetRequiredBuffer;
  }

  return EFI_SUCCESS;
}

/**
  Return FmpImageInfoDescriptorVer by an FMP handle.

  @param[in]  Handle   A FMP handle.

  @return FmpImageInfoDescriptorVer associated with the FMP.
**/
UINT32
GetFmpImageInfoDescriptorVer (
  IN EFI_HANDLE  Handle
  )
{
  EFI_STATUS                        Status;
  EFI_FIRMWARE_MANAGEMENT_PROTOCOL  *Fmp;
  UINTN                             ImageInfoSize;
  EFI_FIRMWARE_IMAGE_DESCRIPTOR     *FmpImageInfoBuf;
  UINT32                            FmpImageInfoDescriptorVer;
  UINT8                             FmpImageInfoCount;
  UINTN                             DescriptorSize;
  UINT32                            PackageVersion;
  CHAR16                            *PackageVersionName;

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiFirmwareManagementProtocolGuid,
                  (VOID **)&Fmp
                  );
  if (EFI_ERROR (Status)) {
    return 0;
  }

  ImageInfoSize = 0;
  Status        = Fmp->GetImageInfo (
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
    return 0;
  }

  FmpImageInfoBuf = AllocateZeroPool (ImageInfoSize);
  if (FmpImageInfoBuf == NULL) {
    return 0;
  }

  PackageVersionName = NULL;
  Status             = Fmp->GetImageInfo (
                              Fmp,
                              &ImageInfoSize,             // ImageInfoSize
                              FmpImageInfoBuf,            // ImageInfo
                              &FmpImageInfoDescriptorVer, // DescriptorVersion
                              &FmpImageInfoCount,         // DescriptorCount
                              &DescriptorSize,            // DescriptorSize
                              &PackageVersion,            // PackageVersion
                              &PackageVersionName         // PackageVersionName
                              );
  if (EFI_ERROR (Status)) {
    FreePool (FmpImageInfoBuf);
    return 0;
  }

  return FmpImageInfoDescriptorVer;
}

/**
  Set FMP image data.

  @param[in]  Handle        A FMP handle.
  @param[in]  ImageHeader   The payload image header.
  @param[in]  PayloadIndex  The index of the payload.

  @return The status of FMP->SetImage.
**/
EFI_STATUS
SetFmpImageData (
  IN EFI_HANDLE                                    Handle,
  IN EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER  *ImageHeader,
  IN UINTN                                         PayloadIndex
  )
{
  EFI_STATUS                                     Status;
  EFI_FIRMWARE_MANAGEMENT_PROTOCOL               *Fmp;
  UINT8                                          *Image;
  VOID                                           *VendorCode;
  CHAR16                                         *AbortReason;
  EFI_FIRMWARE_MANAGEMENT_UPDATE_IMAGE_PROGRESS  ProgressCallback;

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiFirmwareManagementProtocolGuid,
                  (VOID **)&Fmp
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Lookup Firmware Management Progress Protocol before SetImage() is called
  // This is an optional protocol that may not be present on Handle.
  //
  Status = gBS->HandleProtocol (
                  Handle,
                  &gEdkiiFirmwareManagementProgressProtocolGuid,
                  (VOID **)&mFmpProgress
                  );
  if (EFI_ERROR (Status)) {
    mFmpProgress = NULL;
  }

  if (ImageHeader->Version >= EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER_INIT_VERSION) {
    Image = (UINT8 *)(ImageHeader + 1);
  } else {
    //
    // If the EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER is version 1,
    // Header should exclude UpdateHardwareInstance field, and
    // ImageCapsuleSupport field if version is 2.
    //
    if (ImageHeader->Version == 1) {
      Image = (UINT8 *)ImageHeader + OFFSET_OF (EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER, UpdateHardwareInstance);
    } else {
      Image = (UINT8 *)ImageHeader + OFFSET_OF (EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER, ImageCapsuleSupport);
    }
  }

  if (ImageHeader->UpdateVendorCodeSize == 0) {
    VendorCode = NULL;
  } else {
    VendorCode = Image + ImageHeader->UpdateImageSize;
  }

  AbortReason = NULL;
  DEBUG ((DEBUG_INFO, "Fmp->SetImage ...\n"));
  DEBUG ((DEBUG_INFO, "ImageTypeId - %g, ", &ImageHeader->UpdateImageTypeId));
  DEBUG ((DEBUG_INFO, "PayloadIndex - 0x%x, ", PayloadIndex));
  DEBUG ((DEBUG_INFO, "ImageIndex - 0x%x ", ImageHeader->UpdateImageIndex));
  if (ImageHeader->Version >= 2) {
    DEBUG ((DEBUG_INFO, "(UpdateHardwareInstance - 0x%x)", ImageHeader->UpdateHardwareInstance));
    if (ImageHeader->Version >= EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER_INIT_VERSION) {
      DEBUG ((DEBUG_INFO, "(ImageCapsuleSupport - 0x%x)", ImageHeader->ImageCapsuleSupport));
    }
  }

  DEBUG ((DEBUG_INFO, "\n"));

  //
  // Before calling SetImage(), reset the progress bar to 0%
  //
  ProgressCallback = UpdateImageProgress;
  Status           = UpdateImageProgress (0);
  if (EFI_ERROR (Status)) {
    ProgressCallback = NULL;
  }

  Status = Fmp->SetImage (
                  Fmp,
                  ImageHeader->UpdateImageIndex,          // ImageIndex
                  Image,                                  // Image
                  ImageHeader->UpdateImageSize,           // ImageSize
                  VendorCode,                             // VendorCode
                  ProgressCallback,                       // Progress
                  &AbortReason                            // AbortReason
                  );
  //
  // Set the progress bar to 100% after returning from SetImage()
  //
  if (ProgressCallback != NULL) {
    UpdateImageProgress (100);
  }

  DEBUG ((DEBUG_INFO, "Fmp->SetImage - %r\n", Status));
  if (AbortReason != NULL) {
    DEBUG ((DEBUG_ERROR, "%s\n", AbortReason));
    FreePool (AbortReason);
  }

  //
  // Clear mFmpProgress after SetImage() returns
  //
  mFmpProgress = NULL;

  return Status;
}

/**
  Start a UEFI image in the FMP payload.

  @param[in]  ImageBuffer   A pointer to the memory location containing a copy of the image to be loaded..
  @param[in]  ImageSize     The size in bytes of ImageBuffer.

  @return The status of gBS->LoadImage and gBS->StartImage.
**/
EFI_STATUS
StartFmpImage (
  IN VOID   *ImageBuffer,
  IN UINTN  ImageSize
  )
{
  MEMMAP_DEVICE_PATH        MemMapNode;
  EFI_STATUS                Status;
  EFI_HANDLE                ImageHandle;
  EFI_DEVICE_PATH_PROTOCOL  *DriverDevicePath;
  UINTN                     ExitDataSize;

  SetDevicePathNodeLength (&MemMapNode.Header, sizeof (MemMapNode));
  MemMapNode.Header.Type     = HARDWARE_DEVICE_PATH;
  MemMapNode.Header.SubType  = HW_MEMMAP_DP;
  MemMapNode.MemoryType      = EfiBootServicesCode;
  MemMapNode.StartingAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)ImageBuffer;
  MemMapNode.EndingAddress   = (EFI_PHYSICAL_ADDRESS)(UINTN)((UINT8 *)ImageBuffer + ImageSize - 1);

  DriverDevicePath = AppendDevicePathNode (NULL, &MemMapNode.Header);
  if (DriverDevicePath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  DEBUG ((DEBUG_INFO, "FmpCapsule: LoadImage ...\n"));
  Status = gBS->LoadImage (
                  FALSE,
                  gImageHandle,
                  DriverDevicePath,
                  ImageBuffer,
                  ImageSize,
                  &ImageHandle
                  );
  DEBUG ((DEBUG_INFO, "FmpCapsule: LoadImage - %r\n", Status));
  if (EFI_ERROR (Status)) {
    //
    // With EFI_SECURITY_VIOLATION retval, the Image was loaded and an ImageHandle was created
    // with a valid EFI_LOADED_IMAGE_PROTOCOL, but the image can not be started right now.
    // If the caller doesn't have the option to defer the execution of an image, we should
    // unload image for the EFI_SECURITY_VIOLATION to avoid resource leak.
    //
    if (Status == EFI_SECURITY_VIOLATION) {
      gBS->UnloadImage (ImageHandle);
    }

    FreePool (DriverDevicePath);
    return Status;
  }

  DEBUG ((DEBUG_INFO, "FmpCapsule: StartImage ...\n"));
  Status = gBS->StartImage (
                  ImageHandle,
                  &ExitDataSize,
                  NULL
                  );
  DEBUG ((DEBUG_INFO, "FmpCapsule: StartImage - %r\n", Status));
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Driver Return Status = %r\n", Status));
  }

  FreePool (DriverDevicePath);
  return Status;
}

/**
  Record FMP capsule status.

  @param[in] Handle         A FMP handle.
  @param[in] CapsuleHeader  The capsule image header
  @param[in] CapsuleStatus  The capsule process stauts
  @param[in] PayloadIndex   FMP payload index
  @param[in] ImageHeader    FMP image header
  @param[in] CapFileName    Capsule file name
**/
VOID
RecordFmpCapsuleStatus (
  IN EFI_HANDLE                                    Handle   OPTIONAL,
  IN EFI_CAPSULE_HEADER                            *CapsuleHeader,
  IN EFI_STATUS                                    CapsuleStatus,
  IN UINTN                                         PayloadIndex,
  IN EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER  *ImageHeader,
  IN CHAR16                                        *CapFileName   OPTIONAL
  )
{
  EFI_STATUS                 Status;
  EFI_DEVICE_PATH_PROTOCOL   *FmpDevicePath;
  UINT32                     FmpImageInfoDescriptorVer;
  EFI_STATUS                 StatusEsrt;
  ESRT_MANAGEMENT_PROTOCOL   *EsrtProtocol;
  EFI_SYSTEM_RESOURCE_ENTRY  EsrtEntry;

  FmpDevicePath = NULL;
  if (Handle != NULL) {
    gBS->HandleProtocol (
           Handle,
           &gEfiDevicePathProtocolGuid,
           (VOID **)&FmpDevicePath
           );
  }

  RecordFmpCapsuleStatusVariable (
    CapsuleHeader,
    CapsuleStatus,
    PayloadIndex,
    ImageHeader,
    FmpDevicePath,
    CapFileName
    );

  //
  // Update corresponding ESRT entry LastAttemp Status
  //
  Status = gBS->LocateProtocol (&gEsrtManagementProtocolGuid, NULL, (VOID **)&EsrtProtocol);
  if (EFI_ERROR (Status)) {
    return;
  }

  if (Handle == NULL) {
    return;
  }

  //
  // Update EsrtEntry For V1, V2 FMP instance.
  // V3 FMP ESRT cache will be synced up through SyncEsrtFmp interface
  //
  FmpImageInfoDescriptorVer = GetFmpImageInfoDescriptorVer (Handle);
  if (FmpImageInfoDescriptorVer < EFI_FIRMWARE_IMAGE_DESCRIPTOR_VERSION) {
    StatusEsrt = EsrtProtocol->GetEsrtEntry (&ImageHeader->UpdateImageTypeId, &EsrtEntry);
    if (!EFI_ERROR (StatusEsrt)) {
      if (!EFI_ERROR (CapsuleStatus)) {
        EsrtEntry.LastAttemptStatus = LAST_ATTEMPT_STATUS_SUCCESS;
      } else {
        EsrtEntry.LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_UNSUCCESSFUL;
      }

      EsrtEntry.LastAttemptVersion = 0;
      EsrtProtocol->UpdateEsrtEntry (&EsrtEntry);
    }
  }
}

/**
  Process Firmware management protocol data capsule.

  This function assumes the caller validated the capsule by using
  ValidateFmpCapsule(), so that all fields in EFI_CAPSULE_HEADER,
  EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER and
  EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER are correct.

  This function need support nested FMP capsule.

  @param[in]  CapsuleHeader         Points to a capsule header.
  @param[in]  CapFileName           Capsule file name.
  @param[out] ResetRequired         Indicates whether reset is required or not.

  @retval EFI_SUCESS            Process Capsule Image successfully.
  @retval EFI_UNSUPPORTED       Capsule image is not supported by the firmware.
  @retval EFI_VOLUME_CORRUPTED  FV volume in the capsule is corrupted.
  @retval EFI_OUT_OF_RESOURCES  Not enough memory.
  @retval EFI_NOT_READY         No FMP protocol to handle this FMP capsule.
**/
EFI_STATUS
ProcessFmpCapsuleImage (
  IN EFI_CAPSULE_HEADER  *CapsuleHeader,
  IN CHAR16              *CapFileName   OPTIONAL,
  OUT BOOLEAN            *ResetRequired OPTIONAL
  )
{
  EFI_STATUS                                    Status;
  EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER        *FmpCapsuleHeader;
  EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER  *ImageHeader;
  UINT64                                        *ItemOffsetList;
  UINT32                                        ItemNum;
  UINTN                                         Index;
  EFI_HANDLE                                    *HandleBuffer;
  BOOLEAN                                       *ResetRequiredBuffer;
  UINTN                                         NumberOfHandles;
  UINTN                                         DriverLen;
  UINT64                                        UpdateHardwareInstance;
  UINTN                                         Index2;
  BOOLEAN                                       NotReady;
  BOOLEAN                                       Abort;

  if (!IsFmpCapsuleGuid (&CapsuleHeader->CapsuleGuid)) {
    return ProcessFmpCapsuleImage ((EFI_CAPSULE_HEADER *)((UINTN)CapsuleHeader + CapsuleHeader->HeaderSize), CapFileName, ResetRequired);
  }

  NotReady = FALSE;
  Abort    = FALSE;

  DumpFmpCapsule (CapsuleHeader);

  FmpCapsuleHeader = (EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER *)((UINT8 *)CapsuleHeader + CapsuleHeader->HeaderSize);

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
  // 1. Try to load & start all the drivers within capsule
  //
  for (Index = 0; Index < FmpCapsuleHeader->EmbeddedDriverCount; Index++) {
    if ((FmpCapsuleHeader->PayloadItemCount == 0) &&
        (Index == (UINTN)FmpCapsuleHeader->EmbeddedDriverCount - 1))
    {
      //
      // When driver is last element in the ItemOffsetList array, the driver size is calculated by reference CapsuleImageSize in EFI_CAPSULE_HEADER
      //
      DriverLen = CapsuleHeader->CapsuleImageSize - CapsuleHeader->HeaderSize - (UINTN)ItemOffsetList[Index];
    } else {
      DriverLen = (UINTN)ItemOffsetList[Index + 1] - (UINTN)ItemOffsetList[Index];
    }

    Status = StartFmpImage (
               (UINT8 *)FmpCapsuleHeader + ItemOffsetList[Index],
               DriverLen
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Driver Return Status = %r\n", Status));
      return Status;
    }
  }

  //
  // 2. Route payload to right FMP instance
  //
  DEBUG ((DEBUG_INFO, "FmpCapsule: route payload to right FMP instance ...\n"));

  DumpAllFmpInfo ();

  //
  // Check all the payload entry in capsule payload list
  //
  for (Index = FmpCapsuleHeader->EmbeddedDriverCount; Index < ItemNum; Index++) {
    ImageHeader = (EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER *)((UINT8 *)FmpCapsuleHeader + ItemOffsetList[Index]);

    UpdateHardwareInstance = 0;
    ///
    /// UpdateHardwareInstance field was added in Version 2
    ///
    if (ImageHeader->Version >= 2) {
      UpdateHardwareInstance = ImageHeader->UpdateHardwareInstance;
    }

    Status = GetFmpHandleBufferByType (
               &ImageHeader->UpdateImageTypeId,
               UpdateHardwareInstance,
               &NumberOfHandles,
               &HandleBuffer,
               &ResetRequiredBuffer
               );
    if (EFI_ERROR (Status) ||
        (HandleBuffer == NULL) ||
        (ResetRequiredBuffer == NULL))
    {
      NotReady = TRUE;
      RecordFmpCapsuleStatus (
        NULL,
        CapsuleHeader,
        EFI_NOT_READY,
        Index - FmpCapsuleHeader->EmbeddedDriverCount,
        ImageHeader,
        CapFileName
        );
      continue;
    }

    for (Index2 = 0; Index2 < NumberOfHandles; Index2++) {
      if (Abort) {
        RecordFmpCapsuleStatus (
          HandleBuffer[Index2],
          CapsuleHeader,
          EFI_ABORTED,
          Index - FmpCapsuleHeader->EmbeddedDriverCount,
          ImageHeader,
          CapFileName
          );
        continue;
      }

      Status = SetFmpImageData (
                 HandleBuffer[Index2],
                 ImageHeader,
                 Index - FmpCapsuleHeader->EmbeddedDriverCount
                 );
      if (Status != EFI_SUCCESS) {
        Abort = TRUE;
      } else {
        if (ResetRequired != NULL) {
          *ResetRequired |= ResetRequiredBuffer[Index2];
        }
      }

      RecordFmpCapsuleStatus (
        HandleBuffer[Index2],
        CapsuleHeader,
        Status,
        Index - FmpCapsuleHeader->EmbeddedDriverCount,
        ImageHeader,
        CapFileName
        );
    }

    if (HandleBuffer != NULL) {
      FreePool (HandleBuffer);
    }

    if (ResetRequiredBuffer != NULL) {
      FreePool (ResetRequiredBuffer);
    }
  }

  if (NotReady) {
    return EFI_NOT_READY;
  }

  //
  // always return SUCCESS to indicate this capsule is processed.
  // The status of SetImage is recorded in capsule result variable.
  //
  return EFI_SUCCESS;
}

/**
  Return if there is a FMP header below capsule header.

  @param[in] CapsuleHeader A pointer to EFI_CAPSULE_HEADER

  @retval TRUE  There is a FMP header below capsule header.
  @retval FALSE There is not a FMP header below capsule header
**/
BOOLEAN
IsNestedFmpCapsule (
  IN EFI_CAPSULE_HEADER  *CapsuleHeader
  )
{
  EFI_STATUS                 Status;
  EFI_SYSTEM_RESOURCE_ENTRY  *EsrtEntry;
  UINTN                      Index;
  BOOLEAN                    EsrtGuidFound;
  EFI_CAPSULE_HEADER         *NestedCapsuleHeader;
  UINTN                      NestedCapsuleSize;
  ESRT_MANAGEMENT_PROTOCOL   *EsrtProtocol;
  EFI_SYSTEM_RESOURCE_ENTRY  Entry;

  EsrtGuidFound = FALSE;
  if (mDxeCapsuleLibIsExitBootService) {
    if (mEsrtTable != NULL) {
      EsrtEntry = (EFI_SYSTEM_RESOURCE_ENTRY *)(mEsrtTable + 1);
      for (Index = 0; Index < mEsrtTable->FwResourceCount; Index++, EsrtEntry++) {
        if (CompareGuid (&EsrtEntry->FwClass, &CapsuleHeader->CapsuleGuid)) {
          EsrtGuidFound = TRUE;
          break;
        }
      }
    }
  } else {
    //
    // Check ESRT protocol
    //
    Status = gBS->LocateProtocol (&gEsrtManagementProtocolGuid, NULL, (VOID **)&EsrtProtocol);
    if (!EFI_ERROR (Status)) {
      Status = EsrtProtocol->GetEsrtEntry (&CapsuleHeader->CapsuleGuid, &Entry);
      if (!EFI_ERROR (Status)) {
        EsrtGuidFound = TRUE;
      }
    }

    //
    // Check Firmware Management Protocols
    //
    if (!EsrtGuidFound) {
      Status = GetFmpHandleBufferByType (
                 &CapsuleHeader->CapsuleGuid,
                 0,
                 NULL,
                 NULL,
                 NULL
                 );
      if (!EFI_ERROR (Status)) {
        EsrtGuidFound = TRUE;
      }
    }
  }

  if (!EsrtGuidFound) {
    return FALSE;
  }

  //
  // Check nested capsule header
  // FMP GUID after ESRT one
  //
  NestedCapsuleHeader = (EFI_CAPSULE_HEADER *)((UINT8 *)CapsuleHeader + CapsuleHeader->HeaderSize);
  NestedCapsuleSize   = (UINTN)CapsuleHeader + CapsuleHeader->CapsuleImageSize - (UINTN)NestedCapsuleHeader;
  if (NestedCapsuleSize < sizeof (EFI_CAPSULE_HEADER)) {
    return FALSE;
  }

  if (!IsValidCapsuleHeader (NestedCapsuleHeader, NestedCapsuleSize)) {
    return FALSE;
  }

  if (!IsFmpCapsuleGuid (&NestedCapsuleHeader->CapsuleGuid)) {
    return FALSE;
  }

  DEBUG ((DEBUG_INFO, "IsNestedFmpCapsule\n"));
  return TRUE;
}

/**
  Return if this FMP is a system FMP or a device FMP, based upon CapsuleHeader.

  @param[in] CapsuleHeader A pointer to EFI_CAPSULE_HEADER

  @retval TRUE  It is a system FMP.
  @retval FALSE It is a device FMP.
**/
BOOLEAN
IsFmpCapsule (
  IN EFI_CAPSULE_HEADER  *CapsuleHeader
  )
{
  if (IsFmpCapsuleGuid (&CapsuleHeader->CapsuleGuid)) {
    return TRUE;
  }

  if (IsNestedFmpCapsule (CapsuleHeader)) {
    return TRUE;
  }

  return FALSE;
}

/**
  Those capsules supported by the firmwares.

  Caution: This function may receive untrusted input.

  @param[in]  CapsuleHeader    Points to a capsule header.

  @retval EFI_SUCESS       Input capsule is supported by firmware.
  @retval EFI_UNSUPPORTED  Input capsule is not supported by the firmware.
  @retval EFI_INVALID_PARAMETER Input capsule layout is not correct
**/
EFI_STATUS
EFIAPI
SupportCapsuleImage (
  IN EFI_CAPSULE_HEADER  *CapsuleHeader
  )
{
  //
  // check Display Capsule Guid
  //
  if (CompareGuid (&gWindowsUxCapsuleGuid, &CapsuleHeader->CapsuleGuid)) {
    return EFI_SUCCESS;
  }

  //
  // Check capsule file name capsule
  //
  if (IsCapsuleNameCapsule (CapsuleHeader)) {
    return EFI_SUCCESS;
  }

  if (IsFmpCapsule (CapsuleHeader)) {
    //
    // Fake capsule header is valid case in QueryCapsuleCpapbilities().
    //
    if (CapsuleHeader->HeaderSize == CapsuleHeader->CapsuleImageSize) {
      return EFI_SUCCESS;
    }

    //
    // Check layout of FMP capsule
    //
    return ValidateFmpCapsule (CapsuleHeader, NULL);
  }

  DEBUG ((DEBUG_ERROR, "Unknown Capsule Guid - %g\n", &CapsuleHeader->CapsuleGuid));
  return EFI_UNSUPPORTED;
}

/**
  The firmware implements to process the capsule image.

  Caution: This function may receive untrusted input.

  @param[in]  CapsuleHeader         Points to a capsule header.
  @param[in]  CapFileName           Capsule file name.
  @param[out] ResetRequired         Indicates whether reset is required or not.

  @retval EFI_SUCESS            Process Capsule Image successfully.
  @retval EFI_UNSUPPORTED       Capsule image is not supported by the firmware.
  @retval EFI_VOLUME_CORRUPTED  FV volume in the capsule is corrupted.
  @retval EFI_OUT_OF_RESOURCES  Not enough memory.
**/
EFI_STATUS
EFIAPI
ProcessThisCapsuleImage (
  IN EFI_CAPSULE_HEADER  *CapsuleHeader,
  IN CHAR16              *CapFileName   OPTIONAL,
  OUT BOOLEAN            *ResetRequired OPTIONAL
  )
{
  EFI_STATUS  Status;

  if (SupportCapsuleImage (CapsuleHeader) != EFI_SUCCESS) {
    RecordCapsuleStatusVariable (CapsuleHeader, EFI_UNSUPPORTED);
    return EFI_UNSUPPORTED;
  }

  //
  // Display image in firmware update display capsule
  //
  if (CompareGuid (&gWindowsUxCapsuleGuid, &CapsuleHeader->CapsuleGuid)) {
    DEBUG ((DEBUG_INFO, "ProcessCapsuleImage for WindowsUxCapsule ...\n"));
    Status = DisplayCapsuleImage (CapsuleHeader);
    RecordCapsuleStatusVariable (CapsuleHeader, Status);
    return Status;
  }

  //
  // Check FMP capsule layout
  //
  if (IsFmpCapsule (CapsuleHeader)) {
    DEBUG ((DEBUG_INFO, "ProcessCapsuleImage for FmpCapsule ...\n"));
    DEBUG ((DEBUG_INFO, "ValidateFmpCapsule ...\n"));
    Status = ValidateFmpCapsule (CapsuleHeader, NULL);
    DEBUG ((DEBUG_INFO, "ValidateFmpCapsule - %r\n", Status));
    if (EFI_ERROR (Status)) {
      RecordCapsuleStatusVariable (CapsuleHeader, Status);
      return Status;
    }

    //
    // Process EFI FMP Capsule
    //
    DEBUG ((DEBUG_INFO, "ProcessFmpCapsuleImage ...\n"));
    Status = ProcessFmpCapsuleImage (CapsuleHeader, CapFileName, ResetRequired);
    DEBUG ((DEBUG_INFO, "ProcessFmpCapsuleImage - %r\n", Status));

    return Status;
  }

  return EFI_UNSUPPORTED;
}

/**
  The firmware implements to process the capsule image.

  Caution: This function may receive untrusted input.

  @param[in]  CapsuleHeader         Points to a capsule header.

  @retval EFI_SUCESS            Process Capsule Image successfully.
  @retval EFI_UNSUPPORTED       Capsule image is not supported by the firmware.
  @retval EFI_VOLUME_CORRUPTED  FV volume in the capsule is corrupted.
  @retval EFI_OUT_OF_RESOURCES  Not enough memory.
**/
EFI_STATUS
EFIAPI
ProcessCapsuleImage (
  IN EFI_CAPSULE_HEADER  *CapsuleHeader
  )
{
  return ProcessThisCapsuleImage (CapsuleHeader, NULL, NULL);
}

/**
  Callback function executed when the EndOfDxe event group is signaled.

  @param[in] Event      Event whose notification function is being invoked.
  @param[in] Context    The pointer to the notification function's context, which
                        is implementation-dependent.
**/
VOID
EFIAPI
DxeCapsuleLibEndOfDxe (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  mDxeCapsuleLibEndOfDxe = TRUE;
}

/**
  The constructor function.

  @param[in]  ImageHandle   The firmware allocated handle for the EFI image.
  @param[in]  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor successfully .
**/
EFI_STATUS
EFIAPI
DxeCapsuleLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  DxeCapsuleLibEndOfDxe,
                  NULL,
                  &gEfiEndOfDxeEventGroupGuid,
                  &mDxeCapsuleLibEndOfDxeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  InitCapsuleVariable ();

  return EFI_SUCCESS;
}

/**
  The destructor function closes the End of DXE event.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The destructor completed successfully.
**/
EFI_STATUS
EFIAPI
DxeCapsuleLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Close the End of DXE event.
  //
  Status = gBS->CloseEvent (mDxeCapsuleLibEndOfDxeEvent);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
