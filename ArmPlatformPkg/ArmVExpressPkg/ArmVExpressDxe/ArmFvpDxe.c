/** @file

  Copyright (c) 2013-2015, ARM Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "ArmVExpressInternal.h"

#include <PiDxe.h>
#include <Library/BaseMemoryLib.h>
#include <Library/VirtioMmioDeviceLib.h>
#include <Library/ArmShellCmdLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>

#include <Protocol/FirmwareVolume2.h>

#define ARM_FVP_BASE_VIRTIO_BLOCK_BASE    0x1c130000

#pragma pack(1)
typedef struct {
  VENDOR_DEVICE_PATH                  Vendor;
  EFI_DEVICE_PATH_PROTOCOL            End;
} VIRTIO_BLK_DEVICE_PATH;
#pragma pack()

VIRTIO_BLK_DEVICE_PATH mVirtioBlockDevicePath =
{
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)( sizeof(VENDOR_DEVICE_PATH) ),
        (UINT8)((sizeof(VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    EFI_CALLER_ID_GUID,
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      sizeof (EFI_DEVICE_PATH_PROTOCOL),
      0
    }
  }
};

STATIC
EFI_STATUS
InternalFindFdtByGuid (
  IN OUT   EFI_DEVICE_PATH  **FdtDevicePath,
  IN CONST EFI_GUID         *FdtGuid
  )
{
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH    FileDevicePath;
  EFI_HANDLE                           *HandleBuffer;
  UINTN                                HandleCount;
  UINTN                                Index;
  EFI_FIRMWARE_VOLUME2_PROTOCOL        *FvProtocol;
  EFI_GUID                             NameGuid;
  UINTN                                Size;
  VOID                                 *Key;
  EFI_FV_FILETYPE                      FileType;
  EFI_FV_FILE_ATTRIBUTES               Attributes;
  EFI_DEVICE_PATH                      *FvDevicePath;
  EFI_STATUS                           Status;

  if (FdtGuid == NULL) {
    return EFI_NOT_FOUND;
  }

  EfiInitializeFwVolDevicepathNode (&FileDevicePath, FdtGuid);

  HandleBuffer = NULL;
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiFirmwareVolume2ProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiFirmwareVolume2ProtocolGuid,
                    (VOID **) &FvProtocol
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    // Allocate Key
    Key = AllocatePool (FvProtocol->KeySize);
    ASSERT (Key != NULL);
    ZeroMem (Key, FvProtocol->KeySize);

    do {
      FileType = EFI_FV_FILETYPE_RAW;
      Status = FvProtocol->GetNextFile (FvProtocol, Key, &FileType, &NameGuid, &Attributes, &Size);
      if (Status == EFI_NOT_FOUND) {
        break;
      }
      if (EFI_ERROR (Status)) {
        return Status;
      }

      //
      // Check whether this file is the one we are looking for. If so,
      // create a device path for it and return it to the caller.
      //
      if (CompareGuid (&NameGuid, FdtGuid)) {
          Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiDevicePathProtocolGuid, (VOID **)&FvDevicePath);
          if (!EFI_ERROR (Status)) {
            *FdtDevicePath = AppendDevicePathNode (FvDevicePath,
                               (EFI_DEVICE_PATH_PROTOCOL *)&FileDevicePath);
          }
          goto Done;
      }
    } while (TRUE);
    FreePool (Key);
  }

  if (Index == HandleCount) {
    Status = EFI_NOT_FOUND;
  }
  return Status;

Done:
  FreePool (Key);
  return Status;
}

/**
 * Generic UEFI Entrypoint for 'ArmFvpDxe' driver
 * See UEFI specification for the details of the parameters
 */
EFI_STATUS
EFIAPI
ArmFvpInitialise (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  CONST ARM_VEXPRESS_PLATFORM* Platform;
  EFI_STATUS                   Status;
  CHAR16                       *TextDevicePath;
  UINTN                        TextDevicePathSize;
  VOID                         *Buffer;
  EFI_DEVICE_PATH              *FdtDevicePath;

  Status = gBS->InstallProtocolInterface (&ImageHandle,
                 &gEfiDevicePathProtocolGuid, EFI_NATIVE_INTERFACE,
                 &mVirtioBlockDevicePath);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ArmVExpressGetPlatform (&Platform);
  if (!EFI_ERROR (Status)) {
    FdtDevicePath = NULL;
    Status = InternalFindFdtByGuid (&FdtDevicePath, Platform->FdtGuid);
    if (!EFI_ERROR (Status)) {
      TextDevicePath = ConvertDevicePathToText (FdtDevicePath, FALSE, FALSE);
      if (TextDevicePath != NULL) {
        TextDevicePathSize = StrSize (TextDevicePath);
      }
      FreePool (FdtDevicePath);
    } else {
      TextDevicePathSize  = StrSize ((CHAR16*)PcdGetPtr (PcdFvpFdtDevicePathsBase)) - sizeof (CHAR16);
      TextDevicePathSize += StrSize (Platform->FdtName);

      TextDevicePath = AllocatePool (TextDevicePathSize);
      if (TextDevicePath != NULL) {
        StrCpy (TextDevicePath, ((CHAR16*)PcdGetPtr (PcdFvpFdtDevicePathsBase)));
        StrCat (TextDevicePath, Platform->FdtName);
      }
    }
    if (TextDevicePath != NULL) {
      Buffer = PcdSetPtr (PcdFdtDevicePaths, &TextDevicePathSize, TextDevicePath);
      if (Buffer == NULL) {
        DEBUG ((
          EFI_D_ERROR,
          "ArmFvpDxe: Setting of FDT device path in PcdFdtDevicePaths failed - %r\n", EFI_BUFFER_TOO_SMALL
          ));
      }
      FreePool (TextDevicePath);
    }
  }

  // Declare the Virtio BlockIo device
  Status = VirtioMmioInstallDevice (ARM_FVP_BASE_VIRTIO_BLOCK_BASE, ImageHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "ArmFvpDxe: Failed to install Virtio block device\n"));
  }

  // Install dynamic Shell command to run baremetal binaries.
  Status = ShellDynCmdRunAxfInstall (ImageHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "ArmFvpDxe: Failed to install ShellDynCmdRunAxf\n"));
  }

  return Status;
}
