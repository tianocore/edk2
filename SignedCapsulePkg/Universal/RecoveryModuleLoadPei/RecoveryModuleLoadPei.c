/** @file
  Recovery module.

  Caution: This module requires additional review when modified.
  This module will have external input - capsule image.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.

  ProcessRecoveryCapsule(), ProcessFmpCapsuleImage(), ProcessRecoveryImage(),
  ValidateFmpCapsule() will receive untrusted input and do basic validation.

Copyright (c) 2016 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

//
// The package level header files this module uses
//
#include <Uefi.h>
#include <PiPei.h>
//
// The protocols, PPI and GUID definitions for this module
//
#include <Ppi/MasterBootMode.h>
#include <Ppi/BootInRecoveryMode.h>
#include <Ppi/RecoveryModule.h>
#include <Ppi/DeviceRecoveryModule.h>
#include <Ppi/FirmwareVolumeInfo.h>
#include <Guid/FirmwareFileSystem2.h>
#include <Guid/FmpCapsule.h>
#include <Guid/EdkiiSystemFmpCapsule.h>

//
// The Library classes this module consumes
//
#include <Library/DebugLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>

#include "RecoveryModuleLoadPei.h"

/**
  Loads a DXE capsule from some media into memory and updates the HOB table
  with the DXE firmware volume information.

  @param[in]  PeiServices   General-purpose services that are available to every PEIM.
  @param[in]  This          Indicates the EFI_PEI_RECOVERY_MODULE_PPI instance.

  @retval EFI_SUCCESS        The capsule was loaded correctly.
  @retval EFI_DEVICE_ERROR   A device error occurred.
  @retval EFI_NOT_FOUND      A recovery DXE capsule cannot be found.

**/
EFI_STATUS
EFIAPI
LoadRecoveryCapsule (
  IN EFI_PEI_SERVICES             **PeiServices,
  IN EFI_PEI_RECOVERY_MODULE_PPI  *This
  );

EFI_PEI_RECOVERY_MODULE_PPI  mRecoveryPpi = {
  LoadRecoveryCapsule
};

EFI_PEI_PPI_DESCRIPTOR  mRecoveryPpiList = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiRecoveryModulePpiGuid,
  &mRecoveryPpi
};

/**
  Parse Config data file to get the updated data array.

  @param[in]      DataBuffer      Config raw file buffer.
  @param[in]      BufferSize      Size of raw buffer.
  @param[in, out] ConfigHeader    Pointer to the config header.
  @param[in, out] RecoveryArray   Pointer to the config of recovery data.

  @retval EFI_NOT_FOUND         No config data is found.
  @retval EFI_OUT_OF_RESOURCES  No enough memory is allocated.
  @retval EFI_SUCCESS           Parse the config file successfully.

**/
EFI_STATUS
ParseRecoveryDataFile (
  IN      UINT8                 *DataBuffer,
  IN      UINTN                 BufferSize,
  IN OUT  CONFIG_HEADER         *ConfigHeader,
  IN OUT  RECOVERY_CONFIG_DATA  **RecoveryArray
  );

/**
  Return if this FMP is a system FMP or a device FMP, based upon FmpImageInfo.

  @param[in] FmpImageHeader A pointer to EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER

  @return TRUE  It is a system FMP.
  @return FALSE It is a device FMP.
**/
BOOLEAN
IsSystemFmpImage (
  IN EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER  *FmpImageHeader
  )
{
  GUID   *Guid;
  UINTN  Count;
  UINTN  Index;

  Guid  = PcdGetPtr (PcdSystemFmpCapsuleImageTypeIdGuid);
  Count = PcdGetSize (PcdSystemFmpCapsuleImageTypeIdGuid) / sizeof (GUID);

  for (Index = 0; Index < Count; Index++, Guid++) {
    if (CompareGuid (&FmpImageHeader->UpdateImageTypeId, Guid)) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Return if this CapsuleGuid is a FMP capsule GUID or not.

  @param[in] CapsuleGuid A pointer to EFI_GUID

  @return TRUE  It is a FMP capsule GUID.
  @return FALSE It is not a FMP capsule GUID.
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
  This function assumes the input Capsule image already passes basic check in
  ValidateFmpCapsule().

  Criteria of system FMP capsule is:
  1) FmpCapsuleHeader->EmbeddedDriverCount is 0.
  2) FmpCapsuleHeader->PayloadItemCount is not 0.
  3) All ImageHeader->UpdateImageTypeId matches PcdSystemFmpCapsuleImageTypeIdGuid.

  @param[in]  CapsuleHeader    Points to a capsule header.

  @retval TRUE   Input capsule is a correct system FMP capsule.
  @retval FALSE  Input capsule is not a correct system FMP capsule.
**/
BOOLEAN
IsSystemFmpCapsuleImage (
  IN EFI_CAPSULE_HEADER  *CapsuleHeader
  )
{
  EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER        *FmpCapsuleHeader;
  EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER  *ImageHeader;
  UINT64                                        *ItemOffsetList;
  UINT32                                        ItemNum;
  UINTN                                         Index;

  FmpCapsuleHeader = (EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER *)((UINT8 *)CapsuleHeader + CapsuleHeader->HeaderSize);

  if (FmpCapsuleHeader->EmbeddedDriverCount != 0) {
    return FALSE;
  }

  if (FmpCapsuleHeader->PayloadItemCount == 0) {
    return FALSE;
  }

  ItemNum = FmpCapsuleHeader->EmbeddedDriverCount + FmpCapsuleHeader->PayloadItemCount;

  ItemOffsetList = (UINT64 *)(FmpCapsuleHeader + 1);

  for (Index = 0; Index < ItemNum; Index++) {
    ImageHeader = (EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER *)((UINT8 *)FmpCapsuleHeader + ItemOffsetList[Index]);
    if (!IsSystemFmpImage (ImageHeader)) {
      return FALSE;
    }
  }

  return TRUE;
}

/**
  Validate if it is valid capsule header

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

  @param[in]   CapsuleHeader        Points to a capsule header.
  @param[out]  IsSystemFmp          If it is a system FMP.
  @param[out]  EmbeddedDriverCount  The EmbeddedDriverCount in the FMP capsule.

  @retval EFI_SUCCESS            Input capsule is a correct FMP capsule.
  @retval EFI_INVALID_PARAMETER  Input capsule is not a correct FMP capsule.
**/
EFI_STATUS
ValidateFmpCapsule (
  IN EFI_CAPSULE_HEADER  *CapsuleHeader,
  OUT BOOLEAN            *IsSystemFmp  OPTIONAL,
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
        DEBUG ((DEBUG_ERROR, "ItemOffsetList[%d](0x%lx) < ItemOffsetList[%d](0x%x)\n", Index, ItemOffsetList[Index], Index, ItemOffsetList[Index - 1]));
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

    if (FmpImageSize < OFFSET_OF (EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER, UpdateHardwareInstance)) {
      DEBUG ((DEBUG_ERROR, "FmpImageSize(0x%lx) < EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER\n", FmpImageSize));
      return EFI_INVALID_PARAMETER;
    }

    FmpImageHeaderSize = sizeof (EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER);
    if ((ImageHeader->Version > EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER_INIT_VERSION) ||
        (ImageHeader->Version < 1))
    {
      DEBUG ((DEBUG_ERROR, "ImageHeader->Version(0x%x) Unknown\n", ImageHeader->Version));
      return EFI_INVALID_PARAMETER;
    }

    ///
    /// Current Init ImageHeader version is 3. UpdateHardwareInstance field was added in version 2
    /// and ImageCapsuleSupport field was added in version 3
    ///
    if (ImageHeader->Version == 1) {
      FmpImageHeaderSize = OFFSET_OF (EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER, UpdateHardwareInstance);
    } else if (ImageHeader->Version == 2) {
      FmpImageHeaderSize = OFFSET_OF (EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER, ImageCapsuleSupport);
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

  //
  // Check in system FMP capsule
  //
  if (IsSystemFmp != NULL) {
    *IsSystemFmp = IsSystemFmpCapsuleImage (CapsuleHeader);
  }

  if (EmbeddedDriverCount != NULL) {
    *EmbeddedDriverCount = FmpCapsuleHeader->EmbeddedDriverCount;
  }

  return EFI_SUCCESS;
}

/**
  Recovery module entrypoint

  @param[in] FileHandle   Handle of the file being invoked.
  @param[in] PeiServices  Describes the list of possible PEI Services.

  @return EFI_SUCCESS Recovery module is initialized.
**/
EFI_STATUS
EFIAPI
InitializeRecoveryModule (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS  Status;
  UINTN       BootMode;

  BootMode = GetBootModeHob ();
  ASSERT (BootMode == BOOT_IN_RECOVERY_MODE);

  Status = (**PeiServices).InstallPpi (PeiServices, &mRecoveryPpiList);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Create hob and install FvInfo PPI for recovery capsule.

  @param[in]  FvImage         Points to the DXE FV image.
  @param[in]  FvImageSize     The length of the DXE FV image in bytes.

  @retval EFI_SUCCESS           Create hob and install FvInfo PPI successfully.
  @retval EFI_VOLUME_CORRUPTED  The input data is not an FV.
  @retval EFI_OUT_OF_RESOURCES  No enough resource to process the input data.
**/
EFI_STATUS
EFIAPI
CreateHobForRecoveryCapsule (
  IN VOID   *FvImage,
  IN UINTN  FvImageSize
  )
{
  EFI_FIRMWARE_VOLUME_HEADER  *FvHeader;
  UINT32                      FvAlignment;
  UINT64                      FvLength;
  VOID                        *NewFvBuffer;

  //
  // FvImage should be at its required alignment.
  //
  FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *)FvImage;
  //
  // Validate FV Header, if not as expected, return
  //
  if (ReadUnaligned32 (&FvHeader->Signature) != EFI_FVH_SIGNATURE) {
    DEBUG ((DEBUG_ERROR, "CreateHobForRecoveryCapsule (Fv Signature Error)\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  //
  // If EFI_FVB2_WEAK_ALIGNMENT is set in the volume header then the first byte of the volume
  // can be aligned on any power-of-two boundary. A weakly aligned volume can not be moved from
  // its initial linked location and maintain its alignment.
  //
  if ((ReadUnaligned32 (&FvHeader->Attributes) & EFI_FVB2_WEAK_ALIGNMENT) != EFI_FVB2_WEAK_ALIGNMENT) {
    //
    // Get FvHeader alignment
    //
    FvAlignment = 1 << ((ReadUnaligned32 (&FvHeader->Attributes) & EFI_FVB2_ALIGNMENT) >> 16);
    //
    // FvAlignment must be greater than or equal to 8 bytes of the minimum FFS alignment value.
    //
    if (FvAlignment < 8) {
      FvAlignment = 8;
    }

    //
    // Allocate the aligned buffer for the FvImage.
    //
    if ((UINTN)FvHeader % FvAlignment != 0) {
      DEBUG ((DEBUG_INFO, "CreateHobForRecoveryCapsule (FvHeader 0x%lx is not aligned)\n", (UINT64)(UINTN)FvHeader));
      FvLength    = ReadUnaligned64 (&FvHeader->FvLength);
      NewFvBuffer = AllocateAlignedPages (EFI_SIZE_TO_PAGES ((UINTN)FvLength), FvAlignment);
      if (NewFvBuffer == NULL) {
        DEBUG ((DEBUG_ERROR, "CreateHobForRecoveryCapsule (Not enough resource to allocate 0x%lx bytes)\n", FvLength));
        return EFI_OUT_OF_RESOURCES;
      }

      CopyMem (NewFvBuffer, FvHeader, (UINTN)FvLength);
      FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *)NewFvBuffer;
    }
  }

  BuildFvHob ((UINT64)(UINTN)FvHeader, FvHeader->FvLength);
  DEBUG ((DEBUG_INFO, "BuildFvHob (FV in recovery) - 0x%lx - 0x%lx\n", (UINT64)(UINTN)FvHeader, FvHeader->FvLength));

  PeiServicesInstallFvInfoPpi (
    &FvHeader->FileSystemGuid,
    (VOID *)FvHeader,
    (UINT32)FvHeader->FvLength,
    NULL,
    NULL
    );

  return EFI_SUCCESS;
}

/**
  Create recovery context based upon System Firmware image and config file.

  @param[in]  SystemFirmwareImage     Points to the System Firmware image.
  @param[in]  SystemFirmwareImageSize The length of the System Firmware image in bytes.
  @param[in]  ConfigImage             Points to the config file image.
  @param[in]  ConfigImageSize         The length of the config file image in bytes.

  @retval EFI_SUCCESS            Process Recovery Image successfully.
**/
EFI_STATUS
RecoverImage (
  IN VOID   *SystemFirmwareImage,
  IN UINTN  SystemFirmwareImageSize,
  IN VOID   *ConfigImage,
  IN UINTN  ConfigImageSize
  )
{
  EFI_STATUS            Status;
  RECOVERY_CONFIG_DATA  *ConfigData;
  RECOVERY_CONFIG_DATA  *RecoveryConfigData;
  CONFIG_HEADER         ConfigHeader;
  UINTN                 Index;

  if (ConfigImage == NULL) {
    DEBUG ((DEBUG_INFO, "RecoverImage (NoConfig)\n"));
    Status = CreateHobForRecoveryCapsule (
               SystemFirmwareImage,
               SystemFirmwareImageSize
               );
    return Status;
  }

  ConfigData = NULL;
  ZeroMem (&ConfigHeader, sizeof (ConfigHeader));
  Status = ParseRecoveryDataFile (
             ConfigImage,
             ConfigImageSize,
             &ConfigHeader,
             &ConfigData
             );
  DEBUG ((DEBUG_INFO, "ParseRecoveryDataFile - %r\n", Status));
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((DEBUG_INFO, "ConfigHeader.NumOfRecovery - 0x%x\n", ConfigHeader.NumOfRecovery));
  DEBUG ((DEBUG_INFO, "PcdEdkiiSystemFirmwareFileGuid - %g\n", PcdGetPtr (PcdEdkiiSystemFirmwareFileGuid)));

  Index              = 0;
  RecoveryConfigData = ConfigData;
  while (Index < ConfigHeader.NumOfRecovery) {
    if (CompareGuid (&RecoveryConfigData->FileGuid, PcdGetPtr (PcdEdkiiSystemFirmwareFileGuid))) {
      DEBUG ((DEBUG_INFO, "FileGuid - %g (processing)\n", &RecoveryConfigData->FileGuid));
      Status = CreateHobForRecoveryCapsule (
                 (UINT8 *)SystemFirmwareImage + RecoveryConfigData->ImageOffset,
                 RecoveryConfigData->Length
                 );
      //
      // Shall updates be serialized so that if a recovery FV is not successfully completed,
      // the remaining updates won't be performed.
      //
      if (EFI_ERROR (Status)) {
        break;
      }
    } else {
      DEBUG ((DEBUG_INFO, "FileGuid - %g (ignored)\n", &RecoveryConfigData->FileGuid));
    }

    Index++;
    RecoveryConfigData++;
  }

  return Status;
}

/**
  Process recovery image.

  Caution: This function may receive untrusted input.

  @param[in]  Image         Points to the recovery image.
  @param[in]  Length        The length of the recovery image in bytes.

  @retval EFI_SUCCESS            Process Recovery Image successfully.
  @retval EFI_SECURITY_VIOLATION Recovery image is not processed due to security violation.
**/
EFI_STATUS
ProcessRecoveryImage (
  IN VOID   *Image,
  IN UINTN  Length
  )
{
  UINT32      LastAttemptVersion;
  UINT32      LastAttemptStatus;
  EFI_STATUS  Status;
  VOID        *SystemFirmwareImage;
  UINTN       SystemFirmwareImageSize;
  VOID        *ConfigImage;
  UINTN       ConfigImageSize;
  VOID        *AuthenticatedImage;
  UINTN       AuthenticatedImageSize;

  AuthenticatedImage     = NULL;
  AuthenticatedImageSize = 0;

  Status = CapsuleAuthenticateSystemFirmware (Image, Length, TRUE, &LastAttemptVersion, &LastAttemptStatus, &AuthenticatedImage, &AuthenticatedImageSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "CapsuleAuthenticateSystemFirmware - %r\n", Status));
    return Status;
  }

  ExtractSystemFirmwareImage (AuthenticatedImage, AuthenticatedImageSize, &SystemFirmwareImage, &SystemFirmwareImageSize);
  ExtractConfigImage (AuthenticatedImage, AuthenticatedImageSize, &ConfigImage, &ConfigImageSize);

  Status = RecoverImage (SystemFirmwareImage, SystemFirmwareImageSize, ConfigImage, ConfigImageSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "RecoverImage - %r\n", Status));
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Process Firmware management protocol data capsule.

  Caution: This function may receive untrusted input.

  This function assumes the caller validated the capsule by using
  ValidateFmpCapsule(), so that all fields in EFI_CAPSULE_HEADER,
  EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER and
  EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER are correct.

  @param[in]  CapsuleHeader         Points to a capsule header.
  @param[in]  IsSystemFmp           If this capsule is a system FMP capsule.

  @retval EFI_SUCCESS           Process Capsule Image successfully.
  @retval EFI_UNSUPPORTED       Capsule image is not supported by the firmware.
  @retval EFI_VOLUME_CORRUPTED  FV volume in the capsule is corrupted.
  @retval EFI_OUT_OF_RESOURCES  Not enough memory.
**/
EFI_STATUS
ProcessFmpCapsuleImage (
  IN EFI_CAPSULE_HEADER  *CapsuleHeader,
  IN BOOLEAN             IsSystemFmp
  )
{
  EFI_STATUS                                    Status;
  EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER        *FmpCapsuleHeader;
  EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER  *ImageHeader;
  UINT8                                         *Image;
  UINT64                                        *ItemOffsetList;
  UINTN                                         ItemIndex;

  if (!IsSystemFmp) {
    return EFI_UNSUPPORTED;
  }

  FmpCapsuleHeader = (EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER *)((UINT8 *)CapsuleHeader + CapsuleHeader->HeaderSize);
  ItemOffsetList   = (UINT64 *)(FmpCapsuleHeader + 1);

  for (ItemIndex = 0; ItemIndex < FmpCapsuleHeader->PayloadItemCount; ItemIndex++) {
    ImageHeader = (EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER *)((UINT8 *)FmpCapsuleHeader + ItemOffsetList[ItemIndex]);
    if (ImageHeader->Version >= EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER_INIT_VERSION) {
      Image = (UINT8 *)(ImageHeader + 1);
    } else {
      //
      // If the EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER is version 1, only match ImageTypeId.
      // Header should exclude UpdateHardwareInstance field.
      // If version is 2 Header should exclude ImageCapsuleSupport field.
      //
      if (ImageHeader->Version == 1) {
        Image = (UINT8 *)ImageHeader + OFFSET_OF (EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER, UpdateHardwareInstance);
      } else {
        Image = (UINT8 *)ImageHeader + OFFSET_OF (EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER, ImageCapsuleSupport);
      }
    }

    Status = ProcessRecoveryImage (Image, ImageHeader->UpdateImageSize);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  Process recovery capsule image.

  Caution: This function may receive untrusted input.

  @param[in] CapsuleBuffer   The capsule image buffer.
  @param[in] CapsuleSize     The size of the capsule image in bytes.

  @retval EFI_SUCCESS               The recovery capsule is processed.
  @retval EFI_SECURITY_VIOLATION    The recovery capsule is not process because of security violation.
  @retval EFI_NOT_FOUND             The recovery capsule is not process because of unrecognization.
**/
EFI_STATUS
EFIAPI
ProcessRecoveryCapsule (
  IN VOID   *CapsuleBuffer,
  IN UINTN  CapsuleSize
  )
{
  EFI_STATUS          Status;
  BOOLEAN             IsSystemFmp;
  EFI_CAPSULE_HEADER  *CapsuleHeader;

  CapsuleHeader = CapsuleBuffer;
  if (!IsValidCapsuleHeader (CapsuleHeader, CapsuleSize)) {
    DEBUG ((DEBUG_ERROR, "CapsuleImageSize incorrect\n"));
    return EFI_SECURITY_VIOLATION;
  }

  //
  // Check FMP capsule layout
  //
  if (IsFmpCapsuleGuid (&CapsuleHeader->CapsuleGuid)) {
    DEBUG ((DEBUG_INFO, "CreateHobForRecoveryCapsule\n"));

    DEBUG ((DEBUG_INFO, "ProcessCapsuleImage for FmpCapsule ...\n"));
    DEBUG ((DEBUG_INFO, "ValidateFmpCapsule ...\n"));
    Status = ValidateFmpCapsule (CapsuleHeader, &IsSystemFmp, NULL);
    DEBUG ((DEBUG_INFO, "ValidateFmpCapsule - %r\n", Status));
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Process EFI FMP Capsule
    //
    DEBUG ((DEBUG_INFO, "ProcessFmpCapsuleImage ...\n"));
    Status = ProcessFmpCapsuleImage (CapsuleHeader, IsSystemFmp);
    DEBUG ((DEBUG_INFO, "ProcessFmpCapsuleImage - %r\n", Status));

    DEBUG ((DEBUG_INFO, "CreateHobForRecoveryCapsule Done\n"));
    return Status;
  }

  return EFI_UNSUPPORTED;
}

/**
  Loads a DXE capsule from some media into memory and updates the HOB table
  with the DXE firmware volume information.

  @param[in]  PeiServices   General-purpose services that are available to every PEIM.
  @param[in]  This          Indicates the EFI_PEI_RECOVERY_MODULE_PPI instance.

  @retval EFI_SUCCESS        The capsule was loaded correctly.
  @retval EFI_DEVICE_ERROR   A device error occurred.
  @retval EFI_NOT_FOUND      A recovery DXE capsule cannot be found.

**/
EFI_STATUS
EFIAPI
LoadRecoveryCapsule (
  IN EFI_PEI_SERVICES             **PeiServices,
  IN EFI_PEI_RECOVERY_MODULE_PPI  *This
  )
{
  EFI_STATUS                          Status;
  EFI_PEI_DEVICE_RECOVERY_MODULE_PPI  *DeviceRecoveryPpi;
  UINTN                               NumberRecoveryCapsules;
  UINTN                               Instance;
  UINTN                               CapsuleInstance;
  UINTN                               CapsuleSize;
  EFI_GUID                            CapsuleType;
  VOID                                *CapsuleBuffer;

  DEBUG ((DEBUG_INFO | DEBUG_LOAD, "Recovery Entry\n"));

  for (Instance = 0; ; Instance++) {
    Status = PeiServicesLocatePpi (
               &gEfiPeiDeviceRecoveryModulePpiGuid,
               Instance,
               NULL,
               (VOID **)&DeviceRecoveryPpi
               );
    DEBUG ((DEBUG_ERROR, "LoadRecoveryCapsule - LocateRecoveryPpi (%d) - %r\n", Instance, Status));
    if (EFI_ERROR (Status)) {
      break;
    }

    NumberRecoveryCapsules = 0;
    Status                 = DeviceRecoveryPpi->GetNumberRecoveryCapsules (
                                                  (EFI_PEI_SERVICES **)PeiServices,
                                                  DeviceRecoveryPpi,
                                                  &NumberRecoveryCapsules
                                                  );
    DEBUG ((DEBUG_ERROR, "LoadRecoveryCapsule - GetNumberRecoveryCapsules (%d) - %r\n", NumberRecoveryCapsules, Status));
    if (EFI_ERROR (Status)) {
      continue;
    }

    for (CapsuleInstance = 1; CapsuleInstance <= NumberRecoveryCapsules; CapsuleInstance++) {
      CapsuleSize = 0;
      Status      = DeviceRecoveryPpi->GetRecoveryCapsuleInfo (
                                         (EFI_PEI_SERVICES **)PeiServices,
                                         DeviceRecoveryPpi,
                                         CapsuleInstance,
                                         &CapsuleSize,
                                         &CapsuleType
                                         );
      DEBUG ((DEBUG_ERROR, "LoadRecoveryCapsule - GetRecoveryCapsuleInfo (%d - %x) - %r\n", CapsuleInstance, CapsuleSize, Status));
      if (EFI_ERROR (Status)) {
        break;
      }

      CapsuleBuffer = AllocatePages (EFI_SIZE_TO_PAGES (CapsuleSize));
      if (CapsuleBuffer == NULL) {
        DEBUG ((DEBUG_ERROR, "LoadRecoveryCapsule - AllocatePool fail\n"));
        continue;
      }

      Status = DeviceRecoveryPpi->LoadRecoveryCapsule (
                                    (EFI_PEI_SERVICES **)PeiServices,
                                    DeviceRecoveryPpi,
                                    CapsuleInstance,
                                    CapsuleBuffer
                                    );
      DEBUG ((DEBUG_ERROR, "LoadRecoveryCapsule - LoadRecoveryCapsule (%d) - %r\n", CapsuleInstance, Status));
      if (EFI_ERROR (Status)) {
        FreePages (CapsuleBuffer, EFI_SIZE_TO_PAGES (CapsuleSize));
        break;
      }

      //
      // good, load capsule buffer
      //
      Status = ProcessRecoveryCapsule (CapsuleBuffer, CapsuleSize);
      return Status;
    }
  }

  return EFI_NOT_FOUND;
}
