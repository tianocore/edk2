/** @file
  The entry of the embedded BDS. This BDS does not follow the Boot Manager requirements
  of the UEFI specification as it is designed to implement an embedded systmes
  propriatary boot scheme.

  This template assume a DXE driver produces a SerialIo protocol not using the EFI
  driver module and it will attempt to connect a console on top of this.


  Copyright (c) 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "BdsEntry.h"


EFI_STATUS
FindApplicationMatchingUiSection (
  IN  CHAR16      *UiString,
  OUT EFI_HANDLE  *FvHandle,
  OUT EFI_GUID    *NameGuid
  )
{
  EFI_STATUS                    Status;
  EFI_STATUS                    NextStatus;
  UINTN                         NoHandles;
  EFI_HANDLE                    *Buffer;
  UINTN                         Index;
  EFI_FV_FILETYPE               FileType;
  EFI_FIRMWARE_VOLUME2_PROTOCOL *Fv;
  VOID                          *Key;
  EFI_FV_FILE_ATTRIBUTES        Attributes;
  UINTN                         Size;
  UINTN                         UiStringLen;
  CHAR16                        *UiSection;
  UINT32                        Authentication;


  UiStringLen = 0;
  if (UiString != NULL) {
    DEBUG ((DEBUG_ERROR, "UiString %s\n", UiString));
    UiStringLen = StrLen (UiString);
  }

  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiFirmwareVolume2ProtocolGuid, NULL, &NoHandles, &Buffer);
  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < NoHandles; Index++) {
      Status = gBS->HandleProtocol (Buffer[Index], &gEfiFirmwareVolume2ProtocolGuid, (VOID **)&Fv);
      if (!EFI_ERROR (Status)) {
        Key = AllocatePool (Fv->KeySize);
        ASSERT (Key != NULL);
        ZeroMem (Key, Fv->KeySize);

        FileType = EFI_FV_FILETYPE_APPLICATION;

        do {
          NextStatus = Fv->GetNextFile (Fv, Key, &FileType, NameGuid, &Attributes, &Size);
          if (!EFI_ERROR (NextStatus)) {
            if (UiString == NULL) {
              //
              // If UiString is NULL match first application we find.
              //
              *FvHandle = Buffer[Index];
              FreePool (Key);
              return Status;
            }

            UiSection = NULL;
            Status = Fv->ReadSection (
                          Fv,
                          NameGuid,
                          EFI_SECTION_USER_INTERFACE,
                          0,
                          (VOID **)&UiSection,
                          &Size,
                          &Authentication
                          );
            if (!EFI_ERROR (Status)) {
              if (StrnCmp (UiString, UiSection, UiStringLen) == 0) {
                //
                // We found a UiString match.
                //
                *FvHandle = Buffer[Index];
                FreePool (Key);
                FreePool (UiSection);
                return Status;
              }
              FreePool (UiSection);
            }
          }
        } while (!EFI_ERROR (NextStatus));

        FreePool (Key);
      }
    }

    FreePool (Buffer);
   }

  return EFI_NOT_FOUND;
}


EFI_DEVICE_PATH *
FvFileDevicePath (
  IN  EFI_HANDLE   FvHandle,
  IN  EFI_GUID     *NameGuid
  )
{
  EFI_DEVICE_PATH_PROTOCOL          *DevicePath;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH NewNode;

  DevicePath = DevicePathFromHandle (FvHandle);

  EfiInitializeFwVolDevicepathNode (&NewNode, NameGuid);

  return AppendDevicePathNode (DevicePath, (EFI_DEVICE_PATH_PROTOCOL *)&NewNode);
}



EFI_STATUS
LoadPeCoffSectionFromFv (
 IN  EFI_HANDLE   FvHandle,
 IN  EFI_GUID     *NameGuid
 )
{
  EFI_STATUS                    Status;
  EFI_DEVICE_PATH_PROTOCOL      *DevicePath;
  EFI_HANDLE                    ImageHandle;

  DevicePath = FvFileDevicePath (FvHandle, NameGuid);

  Status = gBS->LoadImage (TRUE, gImageHandle, DevicePath, NULL, 0, &ImageHandle);
  if (!EFI_ERROR (Status)) {
    PERF_END (NULL, "BDS", NULL, 0);
    Status = gBS->StartImage (ImageHandle, NULL, NULL);
  }

  return Status;
}

