/** @file
*
*  Copyright (c) 2011-2015, ARM Limited. All rights reserved.
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

/**
  Locate an EFI application in a the Firmware Volumes by its Name

  @param  EfiAppGuid            Guid of the EFI Application into the Firmware Volume
  @param  DevicePath            EFI Device Path of the EFI application

  @return EFI_SUCCESS           The function completed successfully.
  @return EFI_NOT_FOUND         The protocol could not be located.
  @return EFI_OUT_OF_RESOURCES  There are not enough resources to find the protocol.

**/
EFI_STATUS
LocateEfiApplicationInFvByName (
  IN  CONST CHAR16*             EfiAppName,
  OUT EFI_DEVICE_PATH           **DevicePath
  )
{
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
  EFI_HANDLE                    *HandleBuffer;
  UINTN                         NumberOfHandles;
  UINTN                         Index;
  EFI_FIRMWARE_VOLUME2_PROTOCOL *FvInstance;

  ASSERT (DevicePath != NULL);

  // Length of FilePath
  UiStringLen = StrLen (EfiAppName);

  // Locate all the Firmware Volume protocols.
  Status = gBS->LocateHandleBuffer (
                   ByProtocol,
                   &gEfiFirmwareVolume2ProtocolGuid,
                   NULL,
                   &NumberOfHandles,
                   &HandleBuffer
                   );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *DevicePath = NULL;

  // Looking for FV with ACPI storage file
  for (Index = 0; Index < NumberOfHandles; Index++) {
    //
    // Get the protocol on this handle
    // This should not fail because of LocateHandleBuffer
    //
    Status = gBS->HandleProtocol (
                     HandleBuffer[Index],
                     &gEfiFirmwareVolume2ProtocolGuid,
                     (VOID**) &FvInstance
                     );
    if (EFI_ERROR (Status)) {
      goto FREE_HANDLE_BUFFER;
    }

    // Allocate Key
    Key = AllocatePool (FvInstance->KeySize);
    ASSERT (Key != NULL);
    ZeroMem (Key, FvInstance->KeySize);

    do {
      // Search in all files
      FileType = EFI_FV_FILETYPE_ALL;

      Status = FvInstance->GetNextFile (FvInstance, Key, &FileType, &NameGuid, &Attributes, &Size);
      if (!EFI_ERROR (Status)) {
        UiSection = NULL;
        FileStatus = FvInstance->ReadSection (
                      FvInstance,
                      &NameGuid,
                      EFI_SECTION_USER_INTERFACE,
                      0,
                      (VOID **)&UiSection,
                      &Size,
                      &Authentication
                      );
        if (!EFI_ERROR (FileStatus)) {
          if (StrnCmp (EfiAppName, UiSection, UiStringLen) == 0) {
            //
            // We found a UiString match.
            //
            Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiDevicePathProtocolGuid, (VOID **)&FvDevicePath);

            // Generate the Device Path for the file
            EfiInitializeFwVolDevicepathNode (&FileDevicePath, &NameGuid);
            *DevicePath = AppendDevicePathNode (FvDevicePath, (EFI_DEVICE_PATH_PROTOCOL *)&FileDevicePath);
            ASSERT (*DevicePath != NULL);

            FreePool (Key);
            FreePool (UiSection);
            FreePool (HandleBuffer);
            return FileStatus;
          }
          FreePool (UiSection);
        }
      }
    } while (!EFI_ERROR (Status));

    FreePool (Key);
  }

FREE_HANDLE_BUFFER:
  FreePool (HandleBuffer);
  return EFI_NOT_FOUND;
}

/**
  Locate an EFI application in a the Firmware Volumes by its GUID

  @param  EfiAppGuid            Guid of the EFI Application into the Firmware Volume
  @param  DevicePath            EFI Device Path of the EFI application

  @return EFI_SUCCESS           The function completed successfully.
  @return EFI_NOT_FOUND         The protocol could not be located.
  @return EFI_OUT_OF_RESOURCES  There are not enough resources to find the protocol.

**/
EFI_STATUS
LocateEfiApplicationInFvByGuid (
  IN  CONST EFI_GUID            *EfiAppGuid,
  OUT EFI_DEVICE_PATH           **DevicePath
  )
{
  EFI_STATUS                    Status;
  EFI_DEVICE_PATH               *FvDevicePath;
  EFI_HANDLE                    *HandleBuffer;
  UINTN                         NumberOfHandles;
  UINTN                         Index;
  EFI_FIRMWARE_VOLUME2_PROTOCOL *FvInstance;
  EFI_FV_FILE_ATTRIBUTES        Attributes;
  UINT32                        AuthenticationStatus;
  EFI_FV_FILETYPE               Type;
  UINTN                         Size;
  CHAR16                        *UiSection;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH FvFileDevicePath;

  ASSERT (DevicePath != NULL);

  // Locate all the Firmware Volume protocols.
  Status = gBS->LocateHandleBuffer (
                   ByProtocol,
                   &gEfiFirmwareVolume2ProtocolGuid,
                   NULL,
                   &NumberOfHandles,
                   &HandleBuffer
                   );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *DevicePath = NULL;

  // Looking for FV with ACPI storage file
  for (Index = 0; Index < NumberOfHandles; Index++) {
    //
    // Get the protocol on this handle
    // This should not fail because of LocateHandleBuffer
    //
    Status = gBS->HandleProtocol (
                     HandleBuffer[Index],
                     &gEfiFirmwareVolume2ProtocolGuid,
                     (VOID**) &FvInstance
                     );
    if (EFI_ERROR (Status)) {
      goto FREE_HANDLE_BUFFER;
    }

    Status = FvInstance->ReadFile (
                  FvInstance,
                  EfiAppGuid,
                  NULL,
                  &Size,
                  &Type,
                  &Attributes,
                  &AuthenticationStatus
                  );
    if (EFI_ERROR (Status)) {
      //
      // Skip if no EFI application file in the FV
      //
      continue;
    } else {
      UiSection = NULL;
      Status = FvInstance->ReadSection (
                    FvInstance,
                    EfiAppGuid,
                    EFI_SECTION_USER_INTERFACE,
                    0,
                    (VOID **)&UiSection,
                    &Size,
                    &AuthenticationStatus
                    );
      if (!EFI_ERROR (Status)) {
        //
        // Create the EFI Device Path for the application using the Filename of the application
        //
        *DevicePath = FileDevicePath (HandleBuffer[Index], UiSection);
      } else {
        Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiDevicePathProtocolGuid, (VOID**)&FvDevicePath);
        ASSERT_EFI_ERROR (Status);

        //
        // Create the EFI Device Path for the application using the EFI GUID of the application
        //
        EfiInitializeFwVolDevicepathNode (&FvFileDevicePath, EfiAppGuid);

        *DevicePath = AppendDevicePathNode (FvDevicePath, (EFI_DEVICE_PATH_PROTOCOL *)&FvFileDevicePath);
        ASSERT (*DevicePath != NULL);
      }
      break;
    }
  }

FREE_HANDLE_BUFFER:
  //
  // Free any allocated buffers
  //
  FreePool (HandleBuffer);

  if (*DevicePath == NULL) {
    return EFI_NOT_FOUND;
  } else {
    return EFI_SUCCESS;
  }
}
