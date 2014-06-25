/** @file
  Functions in this file will mainly focus on looking through the capsule
  for the image to be programmed, and the flash area that is going to be
  programed.

  Copyright (c) 2002 - 2014, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions
  of the BSD License which accompanies this distribution.  The
  full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UpdateDriver.h"

EFI_HII_HANDLE  gHiiHandle;

/**
  Update the whole FV, or certain files in the FV.
  
  @param ConfigData      Pointer to the config data on updating file.
  @param ImageBuffer     Image buffer to be updated.
  @param ImageSize       Image size.
  @param FileType        FFS file type.
  @param FileAttributes  FFS file attribute.

  @retval EFI_NOT_FOUND  The matched FVB protocol is not found.
  @retval EFI_SUCCESS    The image buffer is updated into FV.

**/
EFI_STATUS
PerformUpdateOnFirmwareVolume (
  IN UPDATE_CONFIG_DATA                 *ConfigData,
  IN UINT8                              *ImageBuffer,
  IN UINTN                              ImageSize,
  IN EFI_FV_FILETYPE                    FileType,
  IN EFI_FV_FILE_ATTRIBUTES             FileAttributes
  )
{
  EFI_STATUS                            Status;
  BOOLEAN                               Found;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL    *FvbProtocol;
  UINTN                                 Index;
  UINTN                                 NumOfHandles;
  EFI_HANDLE                            *HandleBuffer;
  EFI_PHYSICAL_ADDRESS                  BaseAddress;
  EFI_FVB_ATTRIBUTES_2                  Attributes;

  //
  // Locate all Fvb protocol
  //
  HandleBuffer = NULL;
  Status          = gBS->LocateHandleBuffer (
                           ByProtocol,
                           &gEfiFirmwareVolumeBlockProtocolGuid,
                           NULL,
                           &NumOfHandles,
                           &HandleBuffer
                           );
  if ((EFI_ERROR (Status)) || (NumOfHandles == 0) || (HandleBuffer == NULL)) {
    if (HandleBuffer != NULL) {
      FreePool (HandleBuffer);
    }
    return EFI_NOT_FOUND;
  }

  //
  // Check the FVB protocol one by one
  //
  Found               = FALSE;
  FvbProtocol         = NULL;
  for (Index = 0; Index < NumOfHandles; Index++) {
    Status            = gBS->HandleProtocol (
                               HandleBuffer[Index],
                               &gEfiFirmwareVolumeBlockProtocolGuid,
                               (VOID **) &FvbProtocol
                               );
    if (EFI_ERROR (Status)) {
      break;
    }

    //
    // Ensure this FVB protocol supported Write operation.
    //
    Status = FvbProtocol->GetAttributes (FvbProtocol, &Attributes);
    if (EFI_ERROR (Status) || ((Attributes & EFI_FVB2_WRITE_STATUS) == 0)) {
      continue;     
    }

    Status            = FvbProtocol->GetPhysicalAddress (
                                       FvbProtocol,
                                       &BaseAddress
                                      );
    if (EFI_ERROR (Status)) {
      break;
    }
    if (BaseAddress == ConfigData->BaseAddress) {
      Found           = TRUE;
      break;
    }
  }

  if (!Found) {
    if (HandleBuffer != NULL) {
      FreePool (HandleBuffer);
      HandleBuffer = NULL;
    }
    return EFI_NOT_FOUND;
  }

  //
  // Now we have got the corresponding FVB protocol. Use the FVB protocol
  // to update the whole FV, or certain files in the FV.
  //
  if (ConfigData->UpdateType == UpdateWholeFV) {
    if (FileType != EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE) {
      Status    = EFI_INVALID_PARAMETER;
    } else {
      Status    = PerformUpdateOnWholeFv (
                    HandleBuffer[Index],
                    FvbProtocol,
                    ConfigData,
                    ImageBuffer,
                    ImageSize
                    );
    }
  } else if (ConfigData->UpdateType == UpdateFvFile) {
    Status = PerformUpdateOnFvFile (
               HandleBuffer[Index],
               FvbProtocol,
               ConfigData,
               ImageBuffer,
               ImageSize,
               FileType,
               FileAttributes
               );
  }

  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
    HandleBuffer = NULL;
  }

  return Status;
}

/**
  Update the file directly into flash area.

  @param ConfigData      Pointer to the config data on updating file.
  @param ImageBuffer     Image buffer to be updated.
  @param ImageSize       Image size.

  @retval EFI_SUCCESS    The file is updated into flash area.
  @retval EFI_NOT_FOUND  The FVB protocol for the updated flash area is not found.

**/
EFI_STATUS
PerformUpdateOnFlashArea (
  IN UPDATE_CONFIG_DATA                 *ConfigData,
  IN UINT8                              *ImageBuffer,
  IN UINTN                              ImageSize
  )
{
  EFI_STATUS                            Status;
  UINTN                                 SizeLeft;
  EFI_PHYSICAL_ADDRESS                  FlashAddress;
  UINT8                                 *PtrImage;
  BOOLEAN                               Found;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL    *FvbProtocol;
  UINTN                                 Index;
  UINTN                                 NumOfHandles;
  EFI_HANDLE                            *HandleBuffer;
  EFI_PHYSICAL_ADDRESS                  BaseAddress;
  EFI_FIRMWARE_VOLUME_HEADER            *FwVolHeader;
  EFI_HANDLE                            FvbHandle;
  UINTN                                 SizeUpdated;
  CHAR16                                *TmpStr;
  EFI_FVB_ATTRIBUTES_2                  Attributes;

  SizeLeft              = ImageSize;
  PtrImage              = ImageBuffer;
  FlashAddress          = ConfigData->BaseAddress;
  Status                = EFI_SUCCESS;
  HandleBuffer          = NULL;

  //
  // Print on screen
  //
  TmpStr = HiiGetString (gHiiHandle, STRING_TOKEN(UPDATE_FLASH_RANGE), NULL);
  if (TmpStr != NULL) {
    Print (TmpStr, FlashAddress, ((UINT64)SizeLeft + FlashAddress));
    FreePool (TmpStr);
  }
  
  //
  // Locate all Fvb protocol
  //
  Status          = gBS->LocateHandleBuffer (
                           ByProtocol,
                           &gEfiFirmwareVolumeBlockProtocolGuid,
                           NULL,
                           &NumOfHandles,
                           &HandleBuffer
                           );
  if ((EFI_ERROR (Status)) || (NumOfHandles == 0) || (HandleBuffer == NULL)) {
    if (HandleBuffer != NULL) {
      FreePool (HandleBuffer);
    }
    return EFI_NOT_FOUND;
  }

  while (SizeLeft > 0) {
    //
    // First get the FVB protocols. If the flash area is a FV, or sub FV,
    // we can directly locate all the FVB protocol. Otherwise we should use
    // implementation specific method to get the alternate FVB protocol
    //
    Found               = FALSE;
    FvbProtocol         = NULL;

    //
    // Check the FVB protocol one by one
    //
    for (Index = 0; Index < NumOfHandles; Index++) {
      Status        = gBS->HandleProtocol (
                             HandleBuffer[Index],
                             &gEfiFirmwareVolumeBlockProtocolGuid,
                             (VOID **) &FvbProtocol
                             );
      if (EFI_ERROR (Status)) {
        break;
      }

      //
      // Ensure this FVB protocol supported Write operation.
      //
      Status = FvbProtocol->GetAttributes (FvbProtocol, &Attributes);
      if (EFI_ERROR (Status) || ((Attributes & EFI_FVB2_WRITE_STATUS) == 0)) {
        continue;     
      }

      Status        = FvbProtocol->GetPhysicalAddress (
                                     FvbProtocol,
                                     &BaseAddress
                                     );
      if (EFI_ERROR (Status)) {
        break;
      }
      FwVolHeader   = (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)BaseAddress;

      //
      // This sub area entry falls in the range of the FV
      //
      if ((FlashAddress >= BaseAddress) && (FlashAddress < (BaseAddress + FwVolHeader->FvLength))) {
        Found       = TRUE;
        break;
      }
    }

    if (!Found) {
      if (HandleBuffer != NULL) {
        FreePool (HandleBuffer);
        HandleBuffer    = NULL;
      }
      return EFI_NOT_FOUND;
    }

    FvbHandle           = HandleBuffer[Index];
    SizeUpdated         = 0;

    //
    // If the flash area is boot required, the update must be fault tolerant
    //
    if (ConfigData->FaultTolerant) {
      //
      // Finally we are here. We have got the corresponding FVB protocol. Now
      // we need to convert the physical address to LBA and offset and call
      // FTW write. Also check if the flash range is larger than the FV.
      //
      Status            = FaultTolerantUpdateOnPartFv (
                            PtrImage,
                            SizeLeft,
                            &SizeUpdated,
                            ConfigData,
                            FlashAddress,
                            FvbProtocol,
                            FvbHandle
                            );
    } else {
      //
      // Finally we are here. We have got the corresponding FVB protocol. Now
      // we need to convert the physical address to LBA and offset and call
      // FVB write. Also check if the flash range is larger than the FV.
      //
      Status            = NonFaultTolerantUpdateOnPartFv (
                            PtrImage,
                            SizeLeft,
                            &SizeUpdated,
                            FlashAddress,
                            FvbProtocol,
                            FvbHandle
                            );
    }

    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // As part of the FV has been replaced, the FV driver shall re-parse
    // the firmware volume. So re-install FVB protocol here
    //
    Status                =  gBS->ReinstallProtocolInterface (
                                    FvbHandle,
                                    &gEfiFirmwareVolumeBlockProtocolGuid,
                                    FvbProtocol,
                                    FvbProtocol
                                    );

    if (EFI_ERROR (Status)) {
      return Status;
    }
 
    //
    // Check if we are done with the update
    //
    SizeLeft            = SizeLeft - SizeUpdated;
    FlashAddress        = FlashAddress + SizeUpdated;
    PtrImage            = PtrImage + SizeUpdated;
  }

  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
    HandleBuffer = NULL;
  }

  return Status;
}

/**
  Find the updated file, and program it into the flash area based on the config data.

  @param FwVolProtocol   Pointer to FV protocol that contains the updated file.
  @param ConfigData      Pointer to the Config Data on updating file.

  @retval EFI_INVALID_PARAMETER  The update operation is not valid.
  @retval EFI_NOT_FOUND          The updated file is not found.
  @retval EFI_SUCCESS            The file is updated into the flash area.

**/
EFI_STATUS
PerformUpdate (
  IN EFI_FIRMWARE_VOLUME2_PROTOCOL      *FwVolProtocol,
  IN UPDATE_CONFIG_DATA                 *ConfigData
  )
{
  EFI_STATUS                            Status;
  UINT8                                 *FileBuffer;
  UINTN                                 FileBufferSize;
  EFI_FV_FILETYPE                       FileType;
  EFI_FV_FILE_ATTRIBUTES                Attrib;
  EFI_SECTION_TYPE                      SectionType;
  UINT32                                AuthenticationStatus;
  CHAR16                                *TmpStr;
  BOOLEAN                               StartToUpdate;

  Status            = EFI_SUCCESS;
  FileBuffer        = NULL;
  FileBufferSize    = 0;
  Status            = FwVolProtocol->ReadFile (
                                       FwVolProtocol,
                                       &(ConfigData->FileGuid),
                                       (VOID **) &FileBuffer,
                                       &FileBufferSize,
                                       &FileType,
                                       &Attrib,
                                       &AuthenticationStatus
                                       );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  StartToUpdate = FALSE;

  //
  // Check if the update image is the one we require
  // and then perform the update
  //
  switch (ConfigData->UpdateType) {

    case UpdateWholeFV:

      //
      // For UpdateWholeFv, the update file shall be a firmware volume
      // image file.
      //
      if (FileType != EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE) {
        DEBUG ((EFI_D_UPDATE, "UpdateDriver: Data file should be of TYPE EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE\n"));
        Status          = EFI_INVALID_PARAMETER;
      } else {
        if (FileBuffer != NULL) {
          FreePool (FileBuffer);
        }
        SectionType     = EFI_SECTION_FIRMWARE_VOLUME_IMAGE;
        FileBuffer      = NULL;
        FileBufferSize  = 0;
        Status          = FwVolProtocol->ReadSection (
                                           FwVolProtocol,
                                           &(ConfigData->FileGuid),
                                           SectionType,
                                           0,
                                           (VOID **) &FileBuffer,
                                           &FileBufferSize,
                                           &AuthenticationStatus
                                           );
        if (!EFI_ERROR (Status)) {
          //
          // Execute the update. For UpdateWholeFv, the update
          // will always execute on a whole FV
          //
          StartToUpdate = TRUE;
          Status        = PerformUpdateOnFirmwareVolume (
                            ConfigData,
                            FileBuffer,
                            FileBufferSize,
                            FileType,
                            Attrib
                            );

        } else {
          DEBUG ((EFI_D_UPDATE, "UpdateDriver: Data file should be sectioned with TYPE EFI_SECTION_FIRMWARE_VOLUME_IMAGE\n"));
        }
      }
      break;

    case UpdateFvRange:

      //
      // For UpdateFvRange, the update file shall be a raw file
      // which does not contain any sections. The contents of the file
      // will be directly programmed.
      //
      if (FileType != EFI_FV_FILETYPE_RAW) {
        DEBUG ((EFI_D_UPDATE, "UpdateDriver: Data file should of TYPE EFI_FV_FILETYPE_RAW\n"));
        Status          = EFI_INVALID_PARAMETER;
      } else {
        //
        // For UpdateFvRange, the update may be performed on a sub area
        // of a certain FV, or a flash area that is not FV, or part of FV.
        // The update may also go across more than one FVs.
        //
        StartToUpdate   = TRUE;
        Status          = PerformUpdateOnFlashArea (
                            ConfigData,
                            FileBuffer,
                            FileBufferSize
                          );
      }
      break;

    case UpdateFvFile:

      //
      // No check will be done the the file got. The contents of the file
      // will be directly programmed.
      // Though UpdateFvFile will only update a single file, but the update
      // will always execute on a FV
      //
      StartToUpdate = TRUE;
      Status        = PerformUpdateOnFirmwareVolume (
                        ConfigData,
                        FileBuffer,
                        FileBufferSize,
                        FileType,
                        Attrib
                        );
      break;

    default:
      Status        = EFI_INVALID_PARAMETER;
  }

  if (StartToUpdate) {
    if (EFI_ERROR (Status)) {
      TmpStr  = HiiGetString (gHiiHandle, STRING_TOKEN(UPDATE_DRIVER_ABORTED), NULL);
    } else {
      TmpStr  = HiiGetString (gHiiHandle, STRING_TOKEN(UPDATE_DRIVER_DONE), NULL);
    }
    if (TmpStr != NULL) {
      Print (TmpStr);
      FreePool (TmpStr);
    }
  }

  if (FileBuffer != NULL) {
    FreePool(FileBuffer);
    FileBuffer = NULL;
  }

  return Status;
}

/**
  Process the input firmware volume by using DXE service ProcessFirmwareVolume.

  @param DataBuffer      Point to the FV image to be processed.
  @param BufferSize      Size of the FV image buffer.
  @param FwVolProtocol   Point to the installed FV protocol for the input FV image.

  @retval EFI_OUT_OF_RESOURCES   No enough memory is allocated.
  @retval EFI_VOLUME_CORRUPTED   FV image is corrupted.
  @retval EFI_SUCCESS            FV image is processed and FV protocol is installed.

**/
EFI_STATUS
ProcessUpdateImage (
  UINT8                                 *DataBuffer,
  UINTN                                 BufferSize,
  EFI_FIRMWARE_VOLUME2_PROTOCOL          **FwVolProtocol
  )
{
  EFI_FIRMWARE_VOLUME_HEADER            *FwVolHeader;
  EFI_HANDLE                            FwVolHandle;
  EFI_STATUS                            Status;
  UINT8                                 *ProcessedDataBuffer;
  UINT32                                FvAlignment;

  ProcessedDataBuffer = NULL;
  FwVolHeader   = (EFI_FIRMWARE_VOLUME_HEADER *) DataBuffer;
  if (FwVolHeader->FvLength != BufferSize) {
    return EFI_VOLUME_CORRUPTED;
  }

  FvAlignment = 1 << ((FwVolHeader->Attributes & EFI_FVB2_ALIGNMENT) >> 16);
  //
  // FvAlignment must be greater than or equal to 8 bytes of the minimum FFS alignment value.
  // 
  if (FvAlignment < 8) {
    FvAlignment = 8;
  }
  //
  // Check FvImage Align is required.
  //
  if (((UINTN) FwVolHeader % FvAlignment) == 0) {
    ProcessedDataBuffer = DataBuffer;
  } else {
    //
    // Allocate new aligned buffer to store DataBuffer.
    //
    ProcessedDataBuffer = AllocateAlignedPages (EFI_SIZE_TO_PAGES (BufferSize), (UINTN) FvAlignment);
    if (ProcessedDataBuffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    CopyMem (ProcessedDataBuffer, DataBuffer, BufferSize);
  }
  //
  // Process the firmware volume
  //
  gDS->ProcessFirmwareVolume (
         ProcessedDataBuffer,
         BufferSize,
         &FwVolHandle
         );

  //
  // Get the FwVol protocol
  //
  Status = gBS->HandleProtocol (
                  FwVolHandle,
                  &gEfiFirmwareVolume2ProtocolGuid,
                  (VOID **) FwVolProtocol
                  );

  return Status;
}

/**
  Find the image in the same FV and program it in a target Firmware Volume device.
  After update image, it will reset system and no return.
  
  @param ImageHandle   A handle for the image that is initializing this driver
  @param SystemTable   A pointer to the EFI system table

  @retval EFI_ABORTED    System reset failed.
  @retval EFI_NOT_FOUND  The updated image is not found in the same FV.

**/
EFI_STATUS
EFIAPI
InitializeUpdateDriver (
  IN EFI_HANDLE                         ImageHandle,
  IN EFI_SYSTEM_TABLE                   *SystemTable
  )
{
  EFI_STATUS                            Status;
  EFI_LOADED_IMAGE_PROTOCOL             *LoadedImageProtocol;
  EFI_FIRMWARE_VOLUME2_PROTOCOL         *FwVolProtocol;
  EFI_FIRMWARE_VOLUME2_PROTOCOL         *DataFwVolProtocol;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH     *FwVolFilePathNode; 
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH     *AlignedDevPathNode;
  EFI_DEVICE_PATH_PROTOCOL              *FilePathNode;
  EFI_SECTION_TYPE                      SectionType;
  UINT8                                 *FileBuffer;
  UINTN                                 FileBufferSize;
  EFI_FV_FILETYPE                       FileType;
  EFI_FV_FILE_ATTRIBUTES                Attrib;
  UINT32                                AuthenticationStatus;
  UPDATE_CONFIG_DATA                    *ConfigData;
  UPDATE_CONFIG_DATA                    *UpdateConfigData;
  UINTN                                 NumOfUpdates;
  UINTN                                 Index;
  CHAR16                                *TmpStr;

  //
  // Clear screen
  //
  if (gST->ConOut != NULL) {
    gST->ConOut->ClearScreen (gST->ConOut);
    gST->ConOut->SetAttribute (gST->ConOut, EFI_YELLOW | EFI_BRIGHT);
    gST->ConOut->EnableCursor (gST->ConOut, FALSE);
  }

  gHiiHandle = HiiAddPackages (
                 &gEfiCallerIdGuid,
                 NULL,
                 UpdateDriverDxeStrings,
                 NULL
                 );
  ASSERT (gHiiHandle != NULL);

  //
  // In order to look for the update data file and programmed image file
  // from the same volume which this driver is dispatched from, we need
  // to get the device path of this driver image. It is done by first
  // locate the LoadedImageProtocol and then get its device path
  //
  Status            = gBS->OpenProtocol (
                             ImageHandle,
                             &gEfiLoadedImageProtocolGuid,
                             (VOID **)&LoadedImageProtocol,
                             ImageHandle,
                             NULL,
                             EFI_OPEN_PROTOCOL_GET_PROTOCOL
                             );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Get the firmware volume protocol where this file resides
  //
  Status            = gBS->HandleProtocol (
                             LoadedImageProtocol->DeviceHandle,
                             &gEfiFirmwareVolume2ProtocolGuid,
                             (VOID **)  &FwVolProtocol
                             );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  //
  // Shall do some extra check to see if it is really contained in the FV?
  // Should be able to find the section of this driver in the the FV.
  //
  FilePathNode      = LoadedImageProtocol->FilePath;
  FwVolFilePathNode = NULL;
  while (!IsDevicePathEnd (FilePathNode)) {
    if (EfiGetNameGuidFromFwVolDevicePathNode ((MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *)FilePathNode)!= NULL) {
      FwVolFilePathNode = (MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *) FilePathNode;
      break;
    }
    FilePathNode    = NextDevicePathNode (FilePathNode);
  }

  if (FwVolFilePathNode != NULL) {
    AlignedDevPathNode = AllocateCopyPool (DevicePathNodeLength (FwVolFilePathNode), FwVolFilePathNode);

    SectionType     = EFI_SECTION_PE32;
    FileBuffer      = NULL;
    FileBufferSize  = 0;
    Status          = FwVolProtocol->ReadSection (
                                       FwVolProtocol,
                                       &(AlignedDevPathNode->FvFileName),
                                       SectionType,
                                       0,
                                       (VOID **) &FileBuffer,
                                       &FileBufferSize,
                                       &AuthenticationStatus
                                       );
    if (EFI_ERROR (Status)) {
      FreePool (AlignedDevPathNode);
      return Status;
    }

    if (FileBuffer != NULL) {
      FreePool(FileBuffer);
      FileBuffer = NULL;
    }

    //
    // Check the NameGuid of the udpate driver so that it can be
    // used as the CallerId in fault tolerant write protocol
    //
    if (!CompareGuid (&gEfiCallerIdGuid, &(AlignedDevPathNode->FvFileName))) {
      FreePool (AlignedDevPathNode);
      return EFI_NOT_FOUND;
    }
    FreePool (AlignedDevPathNode);
  } else {
    return EFI_NOT_FOUND;
  }

  //
  // Now try to find the script file. The script file is usually
  // a raw data file which does not contain any sections.
  //
  FileBuffer        = NULL;
  FileBufferSize    = 0;
  Status            = FwVolProtocol->ReadFile (
                                       FwVolProtocol,
                                       &gEfiConfigFileNameGuid,
                                       (VOID **) &FileBuffer,
                                       &FileBufferSize,
                                       &FileType,
                                       &Attrib,
                                       &AuthenticationStatus
                                       );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  if (FileType != EFI_FV_FILETYPE_RAW) {
    return EFI_NOT_FOUND;
  }

  //
  // Parse the configuration file.
  //
  ConfigData        = NULL;
  NumOfUpdates      = 0;
  Status            = ParseUpdateDataFile (
                        FileBuffer,
                        FileBufferSize,
                        &NumOfUpdates,
                        &ConfigData
                        );
  if (FileBuffer != NULL) {
    FreePool (FileBuffer);
    FileBuffer = NULL;
  }
  if (EFI_ERROR (Status)) {
    return Status;
  }
  ASSERT (ConfigData != NULL);

  //
  // Now find the update image. The update image should be put in a FV, and then
  // encapsulated as a raw FFS file. This is to prevent the update image from
  // being dispatched. So the raw data we get here should be an FV. We need to
  // process this FV and read the files that is going to be updated.
  //
  FileBuffer        = NULL;
  FileBufferSize    = 0;
  Status            = FwVolProtocol->ReadFile (
                                       FwVolProtocol,
                                       &gEfiUpdateDataFileGuid,
                                       (VOID **) &FileBuffer,
                                       &FileBufferSize,
                                       &FileType,
                                       &Attrib,
                                       &AuthenticationStatus
                                       );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  if (FileType != EFI_FV_FILETYPE_RAW) {
    return EFI_NOT_FOUND;
  }

  //
  // FileBuffer should be an FV. Process the FV
  //
  DataFwVolProtocol = NULL;
  Status            = ProcessUpdateImage (
                        FileBuffer,
                        FileBufferSize,
                        &DataFwVolProtocol
                        );
  if (EFI_ERROR (Status)) {
    FreePool (FileBuffer);
    return Status;
  }

  //
  // Print on screen
  //
  TmpStr  = HiiGetString (gHiiHandle, STRING_TOKEN(UPDATE_PROCESS_DATA), NULL);
  if (TmpStr != NULL) {
    Print (TmpStr);
    FreePool(TmpStr);
  }

  //
  // Execute the update
  //
  Index = 0;
  UpdateConfigData = ConfigData;
  while (Index < NumOfUpdates) {
    Status = PerformUpdate (
               DataFwVolProtocol,
               UpdateConfigData
               );
    //
    // Shall updates be serialized so that if an update is not successfully completed, 
    // the remaining updates won't be performed.
    //
    if (EFI_ERROR (Status)) {
      break;
    }

    Index++;
    UpdateConfigData++;
  }

  if (EFI_ERROR (Status)) {
    if (ConfigData != NULL) {
      FreePool(ConfigData);
      ConfigData = NULL;
    }
    return Status;
  }

  //
  // Call system reset
  //
  gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);

  //
  // Hopefully it won't be reached
  //
  return EFI_ABORTED;
}
