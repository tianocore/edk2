/** @file
  Common code PlatformBootManager and PlatformBootManagerLight.

  Copyright (C) 2025, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/QemuFwCfgLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/PrintLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PlatformBootManagerCommonLib.h>
#include <Library/BaseMemoryLib.h>
#include <Guid/GlobalVariable.h>
#include <Protocol/LoadedImage.h>
#include <Library/UefiLib.h>
#include <Protocol/FirmwareVolume2.h>

/**
  Return the index of the load option in the load option array.

  The function consider two load options are equal when the
  FilePath are equal.

  @param Key    Pointer to the load option to be found.
  @param Array  Pointer to the array of load options to be found.
  @param Count  Number of entries in the Array.

  @retval -1          Key wasn't found in the Array.
  @retval 0 ~ Count-1 The index of the Key in the Array.
**/
INTN
STATIC
OvmfFindLoadOption (
  IN CONST EFI_BOOT_MANAGER_LOAD_OPTION  *Key,
  IN CONST EFI_BOOT_MANAGER_LOAD_OPTION  *Array,
  IN UINTN                               Count
  )
{
  UINTN  Index;

  for (Index = 0; Index < Count; Index++) {
    if (CompareMem (Key->FilePath, Array[Index].FilePath, GetDevicePathSize (Key->FilePath)) == 0) {
      return (INTN)Index;
    }
  }

  return -1;
}

/**
This function checks whether a File exists within the firmware volume.

  @param[in] FilePath  Path of the file.

  @return              TRUE if the file exists within the volume, false
                       otherwise.
**/
BOOLEAN
static
FileIsInFv (
  EFI_DEVICE_PATH_PROTOCOL  *FilePath
  )
{
  UINT32                             AuthenticationStatus;
  EFI_FV_FILE_ATTRIBUTES             FileAttributes;
  EFI_DEVICE_PATH_PROTOCOL           *SearchNode;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH  *FvFileNode;
  EFI_FIRMWARE_VOLUME2_PROTOCOL      *FvProtocol;
  UINTN                              BufferSize;
  EFI_FV_FILETYPE                    FoundType;
  EFI_HANDLE                         FvHandle;
  EFI_STATUS                         Status;

  //
  // Locate the Firmware Volume2 protocol instance that is denoted by the
  // boot option. If this lookup fails (i.e., the boot option references a
  // firmware volume that doesn't exist), then we'll proceed to delete the
  // boot option.
  //
  SearchNode = FilePath;
  Status     = gBS->LocateDevicePath (
                      &gEfiFirmwareVolume2ProtocolGuid,
                      &SearchNode,
                      &FvHandle
                      );

  //
  // File not Found
  //
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  //
  // The firmware volume was found; now let's see if it contains the FvFile
  // identified by GUID.
  //
  Status = gBS->HandleProtocol (
                  FvHandle,
                  &gEfiFirmwareVolume2ProtocolGuid,
                  (VOID **)&FvProtocol
                  );
  ASSERT_EFI_ERROR (Status);

  FvFileNode = (MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *)NextDevicePathNode (FilePath);
  //
  // Buffer==NULL means we request metadata only: BufferSize, FoundType,
  // FileAttributes.
  //
  Status = FvProtocol->ReadFile (
                         FvProtocol,
                         &FvFileNode->FvFileName,  // NameGuid
                         NULL,                     // Buffer
                         &BufferSize,
                         &FoundType,
                         &FileAttributes,
                         &AuthenticationStatus
                         );

  //
  // File not Found
  //
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  //
  // The FvFile was found.
  //
  return TRUE;
}

VOID
PlatformRegisterFvBootOption (
  EFI_GUID  *FileGuid,
  CHAR16    *Description,
  UINT32    Attributes,
  BOOLEAN   Enabled
  )
{
  EFI_STATUS                         Status;
  INTN                               OptionIndex;
  EFI_BOOT_MANAGER_LOAD_OPTION       NewOption;
  EFI_BOOT_MANAGER_LOAD_OPTION       *BootOptions;
  UINTN                              BootOptionCount;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH  FileNode;
  EFI_LOADED_IMAGE_PROTOCOL          *LoadedImage;
  EFI_DEVICE_PATH_PROTOCOL           *DevicePath;

  Status = gBS->HandleProtocol (
                  gImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **)&LoadedImage
                  );
  ASSERT_EFI_ERROR (Status);

  EfiInitializeFwVolDevicepathNode (&FileNode, FileGuid);
  DevicePath = DevicePathFromHandle (LoadedImage->DeviceHandle);
  ASSERT (DevicePath != NULL);
  DevicePath = AppendDevicePathNode (
                 DevicePath,
                 (EFI_DEVICE_PATH_PROTOCOL *)&FileNode
                 );
  ASSERT (DevicePath != NULL);

  //
  // File is not in firmware, so it is going to be deleted anyway by
  // RemoveStaleFvFileOptions, let's not add it.
  //
  if (!FileIsInFv (DevicePath)) {
    FreePool (DevicePath);
    return;
  }

  Status = EfiBootManagerInitializeLoadOption (
             &NewOption,
             LoadOptionNumberUnassigned,
             LoadOptionTypeBoot,
             Attributes,
             Description,
             DevicePath,
             NULL,
             0
             );
  ASSERT_EFI_ERROR (Status);
  FreePool (DevicePath);

  BootOptions = EfiBootManagerGetLoadOptions (
                  &BootOptionCount,
                  LoadOptionTypeBoot
                  );

  OptionIndex = OvmfFindLoadOption (
                  &NewOption,
                  BootOptions,
                  BootOptionCount
                  );

  if ((OptionIndex == -1) && Enabled) {
    Status = EfiBootManagerAddLoadOptionVariable (&NewOption, MAX_UINTN);
    ASSERT_EFI_ERROR (Status);
  } else if ((OptionIndex != -1) && !Enabled) {
    Status = EfiBootManagerDeleteLoadOptionVariable (
               BootOptions[OptionIndex].OptionNumber,
               LoadOptionTypeBoot
               );
    ASSERT_EFI_ERROR (Status);
  } else if ((OptionIndex != -1) && Enabled && (BootOptions[OptionIndex].Attributes != NewOption.Attributes)) {
    // Option Found and enabled but with different Attributes!
    Status = EfiBootManagerDeleteLoadOptionVariable (
               BootOptions[OptionIndex].OptionNumber,
               LoadOptionTypeBoot
               );
    ASSERT_EFI_ERROR (Status);

    Status = EfiBootManagerAddLoadOptionVariable (&NewOption, MAX_UINTN);
    ASSERT_EFI_ERROR (Status);
  }

  EfiBootManagerFreeLoadOption (&NewOption);
  EfiBootManagerFreeLoadOptions (BootOptions, BootOptionCount);
}

/**
  Remove all MemoryMapped(...)/FvFile(...) and Fv(...)/FvFile(...) boot options
  whose device paths do not resolve exactly to an FvFile in the system.

  This removes any boot options that point to binaries built into the firmware
  and have become stale due to any of the following:
  - DXEFV's base address or size changed (historical),
  - DXEFV's FvNameGuid changed,
  - the FILE_GUID of the pointed-to binary changed,
  - the referenced binary is no longer built into the firmware.

  EfiBootManagerFindLoadOption() used in PlatformRegisterFvBootOption() only
  avoids exact duplicates.
**/
VOID
RemoveStaleFvFileOptions (
  VOID
  )
{
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOptions;
  UINTN                         BootOptionCount;
  UINTN                         Index;

  BootOptions = EfiBootManagerGetLoadOptions (
                  &BootOptionCount,
                  LoadOptionTypeBoot
                  );

  for (Index = 0; Index < BootOptionCount; ++Index) {
    EFI_DEVICE_PATH_PROTOCOL  *Node1, *Node2;
    EFI_STATUS                Status;

    //
    // If the device path starts with neither MemoryMapped(...) nor Fv(...),
    // then keep the boot option.
    //

    Node1 = BootOptions[Index].FilePath;
    if (!((DevicePathType (Node1) == HARDWARE_DEVICE_PATH) &&
          (DevicePathSubType (Node1) == HW_MEMMAP_DP)) &&
        !((DevicePathType (Node1) == MEDIA_DEVICE_PATH) &&
          (DevicePathSubType (Node1) == MEDIA_PIWG_FW_VOL_DP)))
    {
      continue;
    }

    //
    // If the second device path node is not FvFile(...), then keep the boot
    // option.
    //
    Node2 = NextDevicePathNode (Node1);
    if ((DevicePathType (Node2) != MEDIA_DEVICE_PATH) ||
        (DevicePathSubType (Node2) != MEDIA_PIWG_FW_FILE_DP))
    {
      continue;
    }

    // If file is in firmware then keep the entry
    if (FileIsInFv (BootOptions[Index].FilePath)) {
      continue;
    }

    //
    // Delete the boot option.
    //
    Status = EfiBootManagerDeleteLoadOptionVariable (
               BootOptions[Index].OptionNumber,
               LoadOptionTypeBoot
               );
    DEBUG_CODE_BEGIN ();
    CHAR16  *DevicePathString;

    DevicePathString = ConvertDevicePathToText (
                         BootOptions[Index].FilePath,
                         FALSE,
                         FALSE
                         );
    DEBUG ((
      EFI_ERROR (Status) ? DEBUG_WARN : DEBUG_VERBOSE,
      "%a: removing stale Boot#%04x %s: %r\n",
      __func__,
      (UINT32)BootOptions[Index].OptionNumber,
      DevicePathString == NULL ? L"<unavailable>" : DevicePathString,
      Status
      ));
    if (DevicePathString != NULL) {
      FreePool (DevicePathString);
    }

    DEBUG_CODE_END ();
  }

  EfiBootManagerFreeLoadOptions (BootOptions, BootOptionCount);
}
