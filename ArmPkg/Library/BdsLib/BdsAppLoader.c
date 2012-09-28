/** @file
*
*  Copyright (c) 2011-2012, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include "BdsInternal.h"

//#include <Library/DxeServicesLib.h>

STATIC
EFI_STATUS
BdsLoadFileFromFirmwareVolume (
  IN  EFI_HANDLE      FvHandle,
  IN  CHAR16    *FilePath,
  IN  EFI_FV_FILETYPE FileTypeFilter,
  OUT EFI_DEVICE_PATH     **EfiAppDevicePath
  )
{
  EFI_FIRMWARE_VOLUME2_PROTOCOL *FvProtocol;
  VOID                          *Key;
  EFI_STATUS                    Status, FileStatus;
  EFI_GUID                      NameGuid;
  EFI_FV_FILETYPE               FileType;
  EFI_FV_FILE_ATTRIBUTES        Attributes;
  UINTN                         Size;
  UINTN                         UiStringLen;
  CHAR16                        *UiSection;
  UINT32                        Authentication;
  EFI_DEVICE_PATH               *FvDevicePath;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH    FileDevicePath;

  Status = gBS->HandleProtocol (FvHandle,&gEfiFirmwareVolume2ProtocolGuid, (VOID **)&FvProtocol);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  // Length of FilePath
  UiStringLen = StrLen (FilePath);

  // Allocate Key
  Key = AllocatePool (FvProtocol->KeySize);
  ASSERT (Key != NULL);
  ZeroMem (Key, FvProtocol->KeySize);

  do {
    // Search in all files
    FileType = FileTypeFilter;

    Status = FvProtocol->GetNextFile (FvProtocol, Key, &FileType, &NameGuid, &Attributes, &Size);
    if (!EFI_ERROR (Status)) {
      UiSection = NULL;
      FileStatus = FvProtocol->ReadSection (
                    FvProtocol,
                    &NameGuid,
                    EFI_SECTION_USER_INTERFACE,
                    0,
                    (VOID **)&UiSection,
                    &Size,
                    &Authentication
                    );
      if (!EFI_ERROR (FileStatus)) {
        if (StrnCmp (FilePath, UiSection, UiStringLen) == 0) {
          //
          // We found a UiString match.
          //
          Status = gBS->HandleProtocol (FvHandle, &gEfiDevicePathProtocolGuid, (VOID **)&FvDevicePath);

          // Generate the Device Path for the file
          //DevicePath = DuplicateDevicePath(FvDevicePath);
          EfiInitializeFwVolDevicepathNode (&FileDevicePath, &NameGuid);
          *EfiAppDevicePath = AppendDevicePathNode (FvDevicePath, (EFI_DEVICE_PATH_PROTOCOL *)&FileDevicePath);

          FreePool (Key);
          FreePool (UiSection);
          return FileStatus;
        }
        FreePool (UiSection);
      }
    }
  } while (!EFI_ERROR (Status));

  FreePool(Key);
  return Status;
}

/**
  Start an EFI Application from any Firmware Volume

  @param  EfiApp                EFI Application Name

  @retval EFI_SUCCESS           All drivers have been connected
  @retval EFI_NOT_FOUND         The Linux kernel Device Path has not been found
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource memory to store the matching results.

**/
EFI_STATUS
BdsLoadApplication (
  IN EFI_HANDLE                  ParentImageHandle,
  IN CHAR16*                     EfiApp,
  IN UINTN                       LoadOptionsSize,
  IN VOID*                       LoadOptions
  )
{
  EFI_STATUS                      Status;
  UINTN                           NoHandles, HandleIndex;
  EFI_HANDLE                      *Handles;
  EFI_DEVICE_PATH                 *EfiAppDevicePath;

  // Need to connect every drivers to ensure no dependencies are missing for the application
  Status = BdsConnectAllDrivers();
  if (EFI_ERROR(Status)) {
    DEBUG ((EFI_D_ERROR, "FAIL to connect all drivers\n"));
    return Status;
  }

  // Search the application in any Firmware Volume
  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiFirmwareVolume2ProtocolGuid, NULL, &NoHandles, &Handles);
  if (EFI_ERROR (Status) || (NoHandles == 0)) {
    DEBUG ((EFI_D_ERROR, "FAIL to find Firmware Volume\n"));
    return Status;
  }

  // Search in all Firmware Volume for the EFI Application
  for (HandleIndex = 0; HandleIndex < NoHandles; HandleIndex++) {
    EfiAppDevicePath = NULL;
    Status = BdsLoadFileFromFirmwareVolume (Handles[HandleIndex], EfiApp, EFI_FV_FILETYPE_APPLICATION, &EfiAppDevicePath);
    if (!EFI_ERROR (Status)) {
      // Start the application
      Status = BdsStartEfiApplication (ParentImageHandle, EfiAppDevicePath, LoadOptionsSize, LoadOptions);
      return Status;
    }
  }

  return Status;
}
