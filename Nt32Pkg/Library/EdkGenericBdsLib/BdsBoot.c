/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  BdsBoot.c

Abstract:

  BDS Lib functions which relate with create or process the boot
  option.

--*/

#include <EdkGenericBdsLibInternal.h>

BOOLEAN mEnumBootDevice = FALSE;

EFI_STATUS
BdsLibDoLegacyBoot (
  IN  BDS_COMMON_OPTION           *Option
  )
/*++

Routine Description:

  Boot the legacy system with the boot option

Arguments:

  Option           - The legacy boot option which have BBS device path

Returns:

  EFI_UNSUPPORTED  - There is no legacybios protocol, do not support
                     legacy boot.

  EFI_STATUS       - Return the status of LegacyBios->LegacyBoot ().

--*/
{
  EFI_STATUS                Status;
  EFI_LEGACY_BIOS_PROTOCOL  *LegacyBios;

  Status = gBS->LocateProtocol (&gEfiLegacyBiosProtocolGuid, NULL, &LegacyBios);
  if (EFI_ERROR (Status)) {
    //
    // If no LegacyBios protocol we do not support legacy boot
    //
    return EFI_UNSUPPORTED;
  }
  //
  // Notes: if we seperate the int 19, then we don't need to refresh BBS
  //
  BdsRefreshBbsTableForBoot (Option);

  //
  // Write boot to OS performance data to a file
  //
  PERF_CODE (
    WriteBootToOsPerformanceData ();
  );


  DEBUG ((EFI_D_INFO | EFI_D_LOAD, "Legacy Boot: %S\n", Option->Description));
  return LegacyBios->LegacyBoot (
                      LegacyBios,
                      (BBS_BBS_DEVICE_PATH *) Option->DevicePath,
                      Option->LoadOptionsSize,
                      Option->LoadOptions
                      );
}

EFI_STATUS
BdsLibBootViaBootOption (
  IN  BDS_COMMON_OPTION             * Option,
  IN  EFI_DEVICE_PATH_PROTOCOL      * DevicePath,
  OUT UINTN                         *ExitDataSize,
  OUT CHAR16                        **ExitData OPTIONAL
  )
/*++

Routine Description:

  Process the boot option follow the EFI 1.1 specification and
  special treat the legacy boot option with BBS_DEVICE_PATH.

Arguments:

  Option       - The boot option need to be processed

  DevicePath   - The device path which describe where to load
                 the boot image or the legcy BBS device path
                 to boot the legacy OS

  ExitDataSize - Returned directly from gBS->StartImage ()

  ExitData     - Returned directly from gBS->StartImage ()

Returns:

  EFI_SUCCESS   - Status from gBS->StartImage (),
                  or BdsBootByDiskSignatureAndPartition ()

  EFI_NOT_FOUND - If the Device Path is not found in the system

--*/
{
  EFI_STATUS                Status;
  EFI_HANDLE                Handle;
  EFI_HANDLE                ImageHandle;
  EFI_DEVICE_PATH_PROTOCOL  *TempDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *FilePath;
  EFI_LOADED_IMAGE_PROTOCOL *ImageInfo;
  EFI_ACPI_S3_SAVE_PROTOCOL *AcpiS3Save;
  EFI_BLOCK_IO_PROTOCOL     *BlkIo;
  VOID                      *Buffer;

  //
  // Record the performance data for End of BDS
  //
  PERF_END (0, BDS_TOK, NULL, 0);

  *ExitDataSize = 0;
  *ExitData     = NULL;

  //
  // Notes: put EFI64 ROM Shadow Solution
  //
  EFI64_SHADOW_ALL_LEGACY_ROM ();

  //
  // Notes: this code can be remove after the s3 script table
  // hook on the event EFI_EVENT_SIGNAL_READY_TO_BOOT or
  // EFI_EVENT_SIGNAL_LEGACY_BOOT
  //
  Status = gBS->LocateProtocol (&gEfiAcpiS3SaveProtocolGuid, NULL, &AcpiS3Save);
  if (!EFI_ERROR (Status)) {
    AcpiS3Save->S3Save (AcpiS3Save, NULL);
  }
  //
  // If it's Device Path that starts with a hard drive path,
  // this routine will do the booting.
  //
  Status = BdsBootByDiskSignatureAndPartition (
            Option,
            (HARDDRIVE_DEVICE_PATH *) DevicePath,
            Option->LoadOptionsSize,
            Option->LoadOptions,
            ExitDataSize,
            ExitData
            );
  if (!EFI_ERROR (Status)) {
    //
    // If we found a disk signature and partition device path return success
    //
    return EFI_SUCCESS;
  }
  //
  // Signal the EFI_EVENT_SIGNAL_READY_TO_BOOT event
  //
  EfiSignalEventReadyToBoot ();

  //
  // Set Boot Current
  //
  gRT->SetVariable (
        L"BootCurrent",
        &gEfiGlobalVariableGuid,
        EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
        sizeof (UINT16),
        &Option->BootCurrent
        );

  if ((DevicePathType (Option->DevicePath) == BBS_DEVICE_PATH) &&
      (DevicePathSubType (Option->DevicePath) == BBS_BBS_DP)
    ) {
    //
    // Check to see if we should legacy BOOT. If yes then do the legacy boot
    //
    return BdsLibDoLegacyBoot (Option);
  }

  DEBUG ((EFI_D_INFO | EFI_D_LOAD, "Booting EFI 1.1 way %S\n", Option->Description));

  Status = gBS->LoadImage (
                  TRUE,
                  mBdsImageHandle,
                  DevicePath,
                  NULL,
                  0,
                  &ImageHandle
                  );

  //
  // If we didn't find an image, we may need to load the default
  // boot behavior for the device.
  //
  if (EFI_ERROR (Status)) {
    //
    // Find a Simple File System protocol on the device path. If the remaining
    // device path is set to end then no Files are being specified, so try
    // the removable media file name.
    //
    TempDevicePath = DevicePath;
    Status = gBS->LocateDevicePath (
                    &gEfiSimpleFileSystemProtocolGuid,
                    &TempDevicePath,
                    &Handle
                    );
    if (!EFI_ERROR (Status) && IsDevicePathEnd (TempDevicePath)) {
      FilePath = FileDevicePath (Handle, EFI_REMOVABLE_MEDIA_FILE_NAME);
      if (FilePath) {
        //
        // Issue a dummy read to the device to check for media change.
        // When the removable media is changed, any Block IO read/write will
        // cause the BlockIo protocol be reinstalled and EFI_MEDIA_CHANGED is
        // returned. After the Block IO protocol is reinstalled, subsequent
        // Block IO read/write will success.
        //
        Status = gBS->HandleProtocol (
                        Handle,
                        &gEfiBlockIoProtocolGuid,
                        (VOID **) &BlkIo
                        );
        if (!EFI_ERROR (Status)) {
          Buffer = AllocatePool (BlkIo->Media->BlockSize);
          if (Buffer != NULL) {
            BlkIo->ReadBlocks (
                     BlkIo,
                     BlkIo->Media->MediaId,
                     0,
                     BlkIo->Media->BlockSize,
                     Buffer
                     );
            FreePool (Buffer);
          }
        }

        Status = gBS->LoadImage (
                        TRUE,
                        mBdsImageHandle,
                        FilePath,
                        NULL,
                        0,
                        &ImageHandle
                        );
        if (EFI_ERROR (Status)) {
          //
          // The DevicePath failed, and it's not a valid
          // removable media device.
          //
          goto Done;
        }
      }
    } else {
      Status = EFI_NOT_FOUND;
    }
  }

  if (EFI_ERROR (Status)) {
    //
    // It there is any error from the Boot attempt exit now.
    //
    goto Done;
  }
  //
  // Provide the image with it's load options
  //
  Status = gBS->HandleProtocol (ImageHandle, &gEfiLoadedImageProtocolGuid, &ImageInfo);
  ASSERT_EFI_ERROR (Status);

  if (Option->LoadOptionsSize != 0) {
    ImageInfo->LoadOptionsSize  = Option->LoadOptionsSize;
    ImageInfo->LoadOptions      = Option->LoadOptions;
  }
  //
  // Before calling the image, enable the Watchdog Timer for
  // the 5 Minute period
  //
  gBS->SetWatchdogTimer (5 * 60, 0x0000, 0x00, NULL);

  Status = gBS->StartImage (ImageHandle, ExitDataSize, ExitData);
  DEBUG ((EFI_D_INFO | EFI_D_LOAD, "Image Return Status = %r\n", Status));

  //
  // Clear the Watchdog Timer after the image returns
  //
  gBS->SetWatchdogTimer (0x0000, 0x0000, 0x0000, NULL);

Done:
  //
  // Clear Boot Current
  //
  gRT->SetVariable (
        L"BootCurrent",
        &gEfiGlobalVariableGuid,
        EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
        0,
        &Option->BootCurrent
        );

  return Status;
}

EFI_STATUS
BdsBootByDiskSignatureAndPartition (
  IN  BDS_COMMON_OPTION          * Option,
  IN  HARDDRIVE_DEVICE_PATH      * HardDriveDevicePath,
  IN  UINT32                     LoadOptionsSize,
  IN  VOID                       *LoadOptions,
  OUT UINTN                      *ExitDataSize,
  OUT CHAR16                     **ExitData OPTIONAL
  )
/*++

Routine Description:

  Check to see if a hard ware device path was passed in. If it was then search
  all the block IO devices for the passed in hard drive device path.

Arguments:

  Option - The current processing boot option.

  HardDriveDevicePath - EFI Device Path to boot, if it starts with a hard
                        drive device path.

  LoadOptionsSize - Passed into gBS->StartImage ()
                    via the loaded image protocol.

  LoadOptions     - Passed into gBS->StartImage ()
                    via the loaded image protocol.

  ExitDataSize - returned directly from gBS->StartImage ()

  ExitData     - returned directly from gBS->StartImage ()

Returns:

  EFI_SUCCESS   - Status from gBS->StartImage (),
                  or BootByDiskSignatureAndPartition ()

  EFI_NOT_FOUND - If the Device Path is not found in the system

--*/
{
  EFI_STATUS                Status;
  UINTN                     BlockIoHandleCount;
  EFI_HANDLE                *BlockIoBuffer;
  EFI_DEVICE_PATH_PROTOCOL  *BlockIoDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *BlockIoHdDevicePath;
  HARDDRIVE_DEVICE_PATH     *TmpHdPath;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *NewDevicePath;
  UINTN                     Index;
  BOOLEAN                   DevicePathMatch;
  HARDDRIVE_DEVICE_PATH     *TempPath;

  *ExitDataSize = 0;
  *ExitData     = NULL;

  if ( !((DevicePathType (&HardDriveDevicePath->Header) == MEDIA_DEVICE_PATH) &&
          (DevicePathSubType (&HardDriveDevicePath->Header) == MEDIA_HARDDRIVE_DP))
        ) {
    //
    // If the HardDriveDevicePath does not start with a Hard Drive Device Path
    // exit.
    //
    return EFI_NOT_FOUND;
  }
  //
  // The boot device have already been connected
  //
  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiBlockIoProtocolGuid, NULL, &BlockIoHandleCount, &BlockIoBuffer);
  if (EFI_ERROR (Status) || BlockIoHandleCount == 0) {
    //
    // If there was an error or there are no device handles that support
    // the BLOCK_IO Protocol, then return.
    //
    return EFI_NOT_FOUND;
  }
  //
  // Loop through all the device handles that support the BLOCK_IO Protocol
  //
  for (Index = 0; Index < BlockIoHandleCount; Index++) {

    Status = gBS->HandleProtocol (BlockIoBuffer[Index], &gEfiDevicePathProtocolGuid, (VOID *) &BlockIoDevicePath);
    if (EFI_ERROR (Status) || BlockIoDevicePath == NULL) {
      continue;
    }
    //
    // Make PreviousDevicePath == the device path node before the end node
    //
    DevicePath          = BlockIoDevicePath;
    BlockIoHdDevicePath = NULL;

    //
    // find HardDriver device path node
    //
    while (!IsDevicePathEnd (DevicePath)) {
      if ((DevicePathType (DevicePath) == MEDIA_DEVICE_PATH) &&
          (DevicePathSubType (DevicePath) == MEDIA_HARDDRIVE_DP)
          ) {
        BlockIoHdDevicePath = DevicePath;
        break;
      }

      DevicePath = NextDevicePathNode (DevicePath);
    }

    if (BlockIoHdDevicePath == NULL) {
      continue;
    }
    //
    // See if the harddrive device path in blockio matches the orig Hard Drive Node
    //
    DevicePathMatch = FALSE;

    TmpHdPath       = (HARDDRIVE_DEVICE_PATH *) BlockIoHdDevicePath;
    TempPath        = (HARDDRIVE_DEVICE_PATH *) BdsLibUnpackDevicePath ((EFI_DEVICE_PATH_PROTOCOL *) HardDriveDevicePath);

    //
    // Only several fields will be checked. NOT whole NODE
    //
    if ( TmpHdPath->PartitionNumber == TempPath->PartitionNumber &&
        TmpHdPath->MBRType == TempPath->MBRType &&
        TmpHdPath->SignatureType == TempPath->SignatureType &&
        CompareGuid ((EFI_GUID *) TmpHdPath->Signature, (EFI_GUID *) TempPath->Signature)) {
      //
      // Get the matched device path
      //
      DevicePathMatch = TRUE;
    }
    //
    // Only do the boot, when devicepath match
    //
    if (DevicePathMatch) {
      //
      // Combine the Block IO and Hard Drive Device path together and try
      // to boot from it.
      //
      DevicePath    = NextDevicePathNode ((EFI_DEVICE_PATH_PROTOCOL *) HardDriveDevicePath);
      NewDevicePath = AppendDevicePath (BlockIoDevicePath, DevicePath);

      //
      // Recursive boot with new device path
      //
      Status = BdsLibBootViaBootOption (Option, NewDevicePath, ExitDataSize, ExitData);
      if (!EFI_ERROR (Status)) {
        break;
      }
    }
  }

  FreePool (BlockIoBuffer);
  return Status;
}

EFI_STATUS
BdsLibDeleteOptionFromHandle (
  IN  EFI_HANDLE                 Handle
  )
/*++

Routine Description:

  Delete the boot option associated with the handle passed in

Arguments:

  Handle - The handle which present the device path to create boot option

Returns:

  EFI_SUCCESS           - Delete the boot option success

  EFI_NOT_FOUND         - If the Device Path is not found in the system

  EFI_OUT_OF_RESOURCES  - Lack of memory resource

  Other                 - Error return value from SetVariable()

--*/
{
  UINT16                    *BootOrder;
  UINT8                     *BootOptionVar;
  UINTN                     BootOrderSize;
  UINTN                     BootOptionSize;
  EFI_STATUS                Status;
  UINTN                     Index;
  UINT16                    BootOption[BOOT_OPTION_MAX_CHAR];
  UINTN                     DevicePathSize;
  UINTN                     OptionDevicePathSize;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *OptionDevicePath;
  UINT8                     *TempPtr;
  CHAR16                    *Description;

  Status        = EFI_SUCCESS;
  BootOrder     = NULL;
  BootOrderSize = 0;

  BootOrder = BdsLibGetVariableAndSize (
                L"BootOrder",
                &gEfiGlobalVariableGuid,
                &BootOrderSize
                );
  if (NULL == BootOrder) {
    return EFI_NOT_FOUND;
  }

  DevicePath = DevicePathFromHandle (Handle);
  if (DevicePath == NULL) {
    return EFI_NOT_FOUND;
  }
  DevicePathSize = GetDevicePathSize (DevicePath);

  Index = 0;
  while (Index < BootOrderSize / sizeof (UINT16)) {
    UnicodeSPrint (BootOption, sizeof (BootOption), L"Boot%04x", BootOrder[Index]);
    BootOptionVar = BdsLibGetVariableAndSize (
                      BootOption,
                      &gEfiGlobalVariableGuid,
                      &BootOptionSize
                      );
    if (NULL == BootOptionVar) {
      FreePool (BootOrder);
      return EFI_OUT_OF_RESOURCES;
    }

    TempPtr = BootOptionVar;
    TempPtr += sizeof (UINT32) + sizeof (UINT16);
    Description = (CHAR16 *) TempPtr;
    TempPtr += StrSize ((CHAR16 *) TempPtr);
    OptionDevicePath = (EFI_DEVICE_PATH_PROTOCOL *) TempPtr;
    OptionDevicePathSize = GetDevicePathSize (OptionDevicePath);

    //
    // Check whether the device path match
    //
    if ((OptionDevicePathSize == DevicePathSize) &&
        (CompareMem (DevicePath, OptionDevicePath, DevicePathSize) == 0)) {
      BdsDeleteBootOption (BootOrder[Index], BootOrder, &BootOrderSize);
      FreePool (BootOptionVar);
      break;
    }

    FreePool (BootOptionVar);
    Index++;
  }

  Status = gRT->SetVariable (
                  L"BootOrder",
                  &gEfiGlobalVariableGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                  BootOrderSize,
                  BootOrder
                  );

  FreePool (BootOrder);

  return Status;
}

EFI_STATUS
BdsDeleteAllInvalidEfiBootOption (
  VOID
  )
/*++

Routine Description:

  Delete all invalid EFI boot options. The probable invalid boot option could
  be Removable media or Network boot device.

Arguments:

  VOID

Returns:

  EFI_SUCCESS           - Delete all invalid boot option success

  EFI_NOT_FOUND         - Variable "BootOrder" is not found

  EFI_OUT_OF_RESOURCES  - Lack of memory resource

  Other                 - Error return value from SetVariable()

--*/
{
  UINT16                    *BootOrder;
  UINT8                     *BootOptionVar;
  UINTN                     BootOrderSize;
  UINTN                     BootOptionSize;
  EFI_STATUS                Status;
  UINTN                     Index;
  UINTN                     Index2;
  UINT16                    BootOption[BOOT_OPTION_MAX_CHAR];
  UINTN                     OptionDevicePathSize;
  EFI_DEVICE_PATH_PROTOCOL  *TempDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *LastDeviceNode;
  EFI_DEVICE_PATH_PROTOCOL  *OptionDevicePath;
  UINT8                     *TempPtr;
  CHAR16                    *Description;
  EFI_HANDLE                Handle;
  BOOLEAN                   NeedDelete;

  Status        = EFI_SUCCESS;
  BootOrder     = NULL;
  BootOrderSize = 0;

  BootOrder = BdsLibGetVariableAndSize (
                L"BootOrder",
                &gEfiGlobalVariableGuid,
                &BootOrderSize
                );
  if (NULL == BootOrder) {
    return EFI_NOT_FOUND;
  }

  Index = 0;
  while (Index < BootOrderSize / sizeof (UINT16)) {
    UnicodeSPrint (BootOption, sizeof (BootOption), L"Boot%04x", BootOrder[Index]);
    BootOptionVar = BdsLibGetVariableAndSize (
                      BootOption,
                      &gEfiGlobalVariableGuid,
                      &BootOptionSize
                      );
    if (NULL == BootOptionVar) {
      FreePool (BootOrder);
      return EFI_OUT_OF_RESOURCES;
    }

    TempPtr = BootOptionVar;
    TempPtr += sizeof (UINT32) + sizeof (UINT16);
    Description = (CHAR16 *) TempPtr;
    TempPtr += StrSize ((CHAR16 *) TempPtr);
    OptionDevicePath = (EFI_DEVICE_PATH_PROTOCOL *) TempPtr;
    OptionDevicePathSize = GetDevicePathSize (OptionDevicePath);

    //
    // Skip legacy boot option (BBS boot device)
    //
    if ((DevicePathType (OptionDevicePath) == BBS_DEVICE_PATH) &&
        (DevicePathSubType (OptionDevicePath) == BBS_BBS_DP)) {
      FreePool (BootOptionVar);
      Index++;
      continue;
    }

    TempDevicePath = OptionDevicePath;
    LastDeviceNode = OptionDevicePath;
    while (!EfiIsDevicePathEnd (TempDevicePath)) {
      LastDeviceNode = TempDevicePath;
      TempDevicePath = EfiNextDevicePathNode (TempDevicePath);
    }
    //
    // Skip the boot option that point to a file, since the device path in
    // removable media boot option doesn't contains a file name.
    //
    if (((DevicePathType (LastDeviceNode) == MEDIA_DEVICE_PATH) &&
         (DevicePathSubType (LastDeviceNode) == MEDIA_FILEPATH_DP)) ||
        //
        // Skip boot option for internal Shell, it's always valid
        //
        (EfiGetNameGuidFromFwVolDevicePathNode ((MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *) LastDeviceNode) != NULL)) {
      FreePool (BootOptionVar);
      Index++;
      continue;
    }

    NeedDelete = TRUE;
    //
    // Check if it's a valid boot option for removable media
    //
    TempDevicePath = OptionDevicePath;
    Status = gBS->LocateDevicePath (
                    &gEfiSimpleFileSystemProtocolGuid,
                    &TempDevicePath,
                    &Handle
                    );
    if (!EFI_ERROR (Status)) {
      NeedDelete = FALSE;
    }
    //
    // Check if it's a valid boot option for network boot device
    //
    TempDevicePath = OptionDevicePath;
    Status = gBS->LocateDevicePath (
                    &gEfiLoadFileProtocolGuid,
                    &TempDevicePath,
                    &Handle
                    );
    if (!EFI_ERROR (Status)) {
      NeedDelete = FALSE;
    }

    if (NeedDelete) {
      //
      // Delete this invalid boot option "Boot####"
      //
      Status = gRT->SetVariable (
                      BootOption,
                      &gEfiGlobalVariableGuid,
                      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                      0,
                      NULL
                      );
      //
      // Mark this boot option in boot order as deleted
      //
      BootOrder[Index] = 0xffff;
    }

    FreePool (BootOptionVar);
    Index++;
  }

  //
  // Adjust boot order array
  //
  Index2 = 0;
  for (Index = 0; Index < BootOrderSize / sizeof (UINT16); Index++) {
    if (BootOrder[Index] != 0xffff) {
      BootOrder[Index2] = BootOrder[Index];
      Index2 ++;
    }
  }
  Status = gRT->SetVariable (
                  L"BootOrder",
                  &gEfiGlobalVariableGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                  Index2 * sizeof (UINT16),
                  BootOrder
                  );

  FreePool (BootOrder);

  return Status;
}

EFI_STATUS
BdsLibEnumerateAllBootOption (
  IN OUT LIST_ENTRY      *BdsBootOptionList
  )
/*++

Routine Description:

  This function will enumerate all possible boot device in the system,
  it will only excute once of every boot.

Arguments:

  BdsBootOptionList - The header of the link list which indexed all
                      current boot options

Returns:

  EFI_SUCCESS - Finished all the boot device enumerate and create
                the boot option base on that boot device

--*/
{
  EFI_STATUS                    Status;
  UINT16                        BootOptionNumber;
  UINTN                         NumberFileSystemHandles;
  EFI_HANDLE                    *FileSystemHandles;
  EFI_BLOCK_IO_PROTOCOL         *BlkIo;
  UINTN                         Index;
  UINTN                         NumberLoadFileHandles;
  EFI_HANDLE                    *LoadFileHandles;
  VOID                          *ProtocolInstance;
  EFI_FIRMWARE_VOLUME_PROTOCOL  *Fv;
  UINTN                         FvHandleCount;
  EFI_HANDLE                    *FvHandleBuffer;
  EFI_FV_FILETYPE               Type;
  UINTN                         Size;
  EFI_FV_FILE_ATTRIBUTES        Attributes;
  UINT32                        AuthenticationStatus;
  EFI_DEVICE_PATH_PROTOCOL      *FilePath;
  EFI_HANDLE                    ImageHandle;
  EFI_LOADED_IMAGE_PROTOCOL     *ImageInfo;
  BOOLEAN                       NeedDelete;

  BootOptionNumber = 0;

  //
  // If the boot device enumerate happened, just get the boot
  // device from the boot order variable
  //
  if (mEnumBootDevice) {
    BdsLibBuildOptionFromVar (BdsBootOptionList, L"BootOrder");
    return EFI_SUCCESS;
  }
  //
  // Notes: this dirty code is to get the legacy boot option from the
  // BBS table and create to variable as the EFI boot option, it should
  // be removed after the CSM can provide legacy boot option directly
  //
  REFRESH_LEGACY_BOOT_OPTIONS;

  //
  // Delete invalid boot option
  //
  BdsDeleteAllInvalidEfiBootOption ();
  //
  // Parse removable media
  //
  gBS->LocateHandleBuffer (
        ByProtocol,
        &gEfiSimpleFileSystemProtocolGuid,
        NULL,
        &NumberFileSystemHandles,
        &FileSystemHandles
        );
  for (Index = 0; Index < NumberFileSystemHandles; Index++) {
    Status = gBS->HandleProtocol (
                    FileSystemHandles[Index],
                    &gEfiBlockIoProtocolGuid,
                    (VOID **) &BlkIo
                    );
    if (!EFI_ERROR (Status)) {
      if (!BlkIo->Media->RemovableMedia) {
        //
        // If the file system handle supports a BlkIo protocol,
        // skip the removable media devices
        //
        continue;
      }
    }

    //
    // Do the removable Media thing. \EFI\BOOT\boot{machinename}.EFI
    //  machinename is ia32, ia64, x64, ...
    //
    FilePath = FileDevicePath (FileSystemHandles[Index], EFI_REMOVABLE_MEDIA_FILE_NAME);
    NeedDelete = TRUE;
    Status = gBS->LoadImage (
                    TRUE,
                    mBdsImageHandle,
                    FilePath,
                    NULL,
                    0,
                    &ImageHandle
                    );
    if (!EFI_ERROR(Status)) {
      //
      // Verify the image is a EFI application (and not a driver)
      //
      Status = gBS->HandleProtocol (ImageHandle, &gEfiLoadedImageProtocolGuid, (VOID **) &ImageInfo);
      ASSERT (!EFI_ERROR(Status));

      if (ImageInfo->ImageCodeType == EfiLoaderCode) {
        NeedDelete = FALSE;
      }
    }

    if (NeedDelete) {
      //
      // No such file or the file is not a EFI application, delete this boot option
      //
      BdsLibDeleteOptionFromHandle (FileSystemHandles[Index]);
    } else {
      BdsLibBuildOptionFromHandle (FileSystemHandles[Index], BdsBootOptionList);
      BootOptionNumber++;
    }
  }

  if (NumberFileSystemHandles) {
    FreePool (FileSystemHandles);
  }
  //
  // Parse Network Boot Device
  //
  gBS->LocateHandleBuffer (
        ByProtocol,
        &gEfiSimpleNetworkProtocolGuid,
        NULL,
        &NumberLoadFileHandles,
        &LoadFileHandles
        );
  for (Index = 0; Index < NumberLoadFileHandles; Index++) {
    Status = gBS->HandleProtocol (
                    LoadFileHandles[Index],
                    &gEfiLoadFileProtocolGuid,
                    (VOID **) &ProtocolInstance
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    BdsLibBuildOptionFromHandle (LoadFileHandles[Index], BdsBootOptionList);
    BootOptionNumber++;
  }

  if (NumberLoadFileHandles) {
    FreePool (LoadFileHandles);
  }
  //
  // Check if we have on flash shell
  //
  gBS->LocateHandleBuffer (
        ByProtocol,
        &gEfiFirmwareVolumeProtocolGuid,
        NULL,
        &FvHandleCount,
        &FvHandleBuffer
        );
  for (Index = 0; Index < FvHandleCount; Index++) {
    gBS->HandleProtocol (
          FvHandleBuffer[Index],
          &gEfiFirmwareVolumeProtocolGuid,
          (VOID **) &Fv
          );

    Status = Fv->ReadFile (
                  Fv,
                  &gEfiShellFileGuid,
                  NULL,
                  &Size,
                  &Type,
                  &Attributes,
                  &AuthenticationStatus
                  );
    if (EFI_ERROR (Status)) {
      //
      // Skip if no shell file in the FV
      //
      continue;
    }
    //
    // Build the shell boot option
    //
    BdsLibBuildOptionFromShell (FvHandleBuffer[Index], BdsBootOptionList);
    BootOptionNumber++;
  }

  if (FvHandleCount) {
   FreePool (FvHandleBuffer);
  }
  //
  // Make sure every boot only have one time
  // boot device enumerate
  //
  BdsLibBuildOptionFromVar (BdsBootOptionList, L"BootOrder");
  mEnumBootDevice = TRUE;

  return EFI_SUCCESS;
}

VOID
BdsLibBuildOptionFromHandle (
  IN  EFI_HANDLE             Handle,
  IN  LIST_ENTRY             *BdsBootOptionList
  )
/*++

Routine Description:

  Build the boot option with the handle parsed in

Arguments:

  Handle - The handle which present the device path to create boot option

  BdsBootOptionList - The header of the link list which indexed all current
                      boot options

Returns:

  VOID

--*/
{
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  CHAR16                    *TempString;

  DevicePath  = DevicePathFromHandle (Handle);
  TempString  = DevicePathToStr (DevicePath);

  //
  // Create and register new boot option
  //
  BdsLibRegisterNewOption (BdsBootOptionList, DevicePath, TempString, L"BootOrder");
}

VOID
BdsLibBuildOptionFromShell (
  IN EFI_HANDLE              Handle,
  IN OUT LIST_ENTRY          *BdsBootOptionList
  )
/*++

Routine Description:

  Build the on flash shell boot option with the handle parsed in

Arguments:

  Handle - The handle which present the device path to create on flash shell
           boot option

  BdsBootOptionList - The header of the link list which indexed all current
                      boot options

Returns:

  None

--*/
{
  EFI_DEVICE_PATH_PROTOCOL          *DevicePath;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH ShellNode;

  DevicePath = DevicePathFromHandle (Handle);

  //
  // Build the shell device path
  //
  EfiInitializeFwVolDevicepathNode (&ShellNode, &gEfiShellFileGuid);
  DevicePath = AppendDevicePathNode (DevicePath, (EFI_DEVICE_PATH_PROTOCOL *) &ShellNode);

  //
  // Create and register the shell boot option
  //
  BdsLibRegisterNewOption (BdsBootOptionList, DevicePath, L"Internal EFI Shell", L"BootOrder");

}

VOID
BdsLibBootNext (
  VOID
  )
/*++

Routine Description:

  Boot from the EFI1.1 spec defined "BootNext" variable

Arguments:

  None

Returns:

  None

--*/
{
  UINT16            *BootNext;
  UINTN             BootNextSize;
  CHAR16            Buffer[20];
  BDS_COMMON_OPTION *BootOption;
  LIST_ENTRY        TempList;
  UINTN             ExitDataSize;
  CHAR16            *ExitData;

  //
  // Init the boot option name buffer and temp link list
  //
  InitializeListHead (&TempList);
  ZeroMem (Buffer, sizeof (Buffer));

  BootNext = BdsLibGetVariableAndSize (
              L"BootNext",
              &gEfiGlobalVariableGuid,
              &BootNextSize
              );

  //
  // Clear the boot next variable first
  //
  if (BootNext != NULL) {
    gRT->SetVariable (
          L"BootNext",
          &gEfiGlobalVariableGuid,
          EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
          0,
          BootNext
          );

    //
    // Start to build the boot option and try to boot
    //
    UnicodeSPrint (Buffer, sizeof (Buffer), L"Boot%04x", *BootNext);
    BootOption = BdsLibVariableToOption (&TempList, Buffer);
    BdsLibConnectDevicePath (BootOption->DevicePath);
    BdsLibBootViaBootOption (BootOption, BootOption->DevicePath, &ExitDataSize, &ExitData);
  }

}
