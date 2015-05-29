/** @file
*
*  Copyright (c) 2011-2014, ARM Limited. All rights reserved.
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

#include <Library/NetLib.h>

#include <Protocol/Bds.h>
#include <Protocol/UsbIo.h>
#include <Protocol/DiskIo.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleNetwork.h>
#include <Protocol/Dhcp4.h>
#include <Protocol/Mtftp4.h>


#define IS_DEVICE_PATH_NODE(node,type,subtype) (((node)->Type == (type)) && ((node)->SubType == (subtype)))

/* Type and defines to set up the DHCP4 options */

typedef struct {
  EFI_DHCP4_PACKET_OPTION Head;
  UINT8                   Route;
} DHCP4_OPTION;

#define DHCP_TAG_PARA_LIST  55
#define DHCP_TAG_NETMASK     1
#define DHCP_TAG_ROUTER      3

/*
   Constant strings and define related to the message indicating the amount of
   progress in the dowloading of a TFTP file.
*/

// Frame for the progression slider
STATIC CONST CHAR16 mTftpProgressFrame[] = L"[                                        ]";

// Number of steps in the progression slider
#define TFTP_PROGRESS_SLIDER_STEPS  ((sizeof (mTftpProgressFrame) / sizeof (CHAR16)) - 3)

// Size in number of characters plus one (final zero) of the message to
// indicate the progress of a tftp download. The format is "[(progress slider:
// 40 characters)] (nb of KBytes downloaded so far: 7 characters) Kb". There
// are thus the number of characters in mTftpProgressFrame[] plus 11 characters
// (2 // spaces, "Kb" and seven characters for the number of KBytes).
#define TFTP_PROGRESS_MESSAGE_SIZE  ((sizeof (mTftpProgressFrame) / sizeof (CHAR16)) + 12)

// String to delete the tftp progress message to be able to update it :
// (TFTP_PROGRESS_MESSAGE_SIZE-1) '\b'
STATIC CONST CHAR16 mTftpProgressDelete[] = L"\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b";


// Extract the FilePath from the Device Path
CHAR16*
BdsExtractFilePathFromDevicePath (
  IN  CONST CHAR16    *StrDevicePath,
  IN  UINTN           NumberDevicePathNode
  )
{
  UINTN       Node;
  CHAR16      *Str;

  Str = (CHAR16*)StrDevicePath;
  Node = 0;
  while ((Str != NULL) && (*Str != L'\0') && (Node < NumberDevicePathNode)) {
    if ((*Str == L'/') || (*Str == L'\\')) {
        Node++;
    }
    Str++;
  }

  if (*Str == L'\0') {
    return NULL;
  } else {
    return Str;
  }
}

BOOLEAN
BdsIsRemovableUsb (
  IN  EFI_DEVICE_PATH*  DevicePath
  )
{
  return ((DevicePathType (DevicePath) == MESSAGING_DEVICE_PATH) &&
          ((DevicePathSubType (DevicePath) == MSG_USB_CLASS_DP) ||
           (DevicePathSubType (DevicePath) == MSG_USB_WWID_DP)));
}

EFI_STATUS
BdsGetDeviceUsb (
  IN  EFI_DEVICE_PATH*  RemovableDevicePath,
  OUT EFI_HANDLE*       DeviceHandle,
  OUT EFI_DEVICE_PATH** NewDevicePath
  )
{
  EFI_STATUS                    Status;
  UINTN                         Index;
  UINTN                         UsbIoHandleCount;
  EFI_HANDLE                    *UsbIoBuffer;
  EFI_DEVICE_PATH*              UsbIoDevicePath;
  EFI_DEVICE_PATH*              TmpDevicePath;
  USB_WWID_DEVICE_PATH*         WwidDevicePath1;
  USB_WWID_DEVICE_PATH*         WwidDevicePath2;
  USB_CLASS_DEVICE_PATH*        UsbClassDevicePath1;
  USB_CLASS_DEVICE_PATH*        UsbClassDevicePath2;

  // Get all the UsbIo handles
  UsbIoHandleCount = 0;
  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiUsbIoProtocolGuid, NULL, &UsbIoHandleCount, &UsbIoBuffer);
  if (EFI_ERROR (Status) || (UsbIoHandleCount == 0)) {
    return Status;
  }

  // Check if one of the handles matches the USB description
  for (Index = 0; Index < UsbIoHandleCount; Index++) {
    Status = gBS->HandleProtocol (UsbIoBuffer[Index], &gEfiDevicePathProtocolGuid, (VOID **) &UsbIoDevicePath);
    if (!EFI_ERROR (Status)) {
      TmpDevicePath = UsbIoDevicePath;
      while (!IsDevicePathEnd (TmpDevicePath)) {
        // Check if the Device Path node is a USB Removable device Path node
        if (BdsIsRemovableUsb (TmpDevicePath)) {
          if (TmpDevicePath->SubType == MSG_USB_WWID_DP) {
            WwidDevicePath1 = (USB_WWID_DEVICE_PATH*)RemovableDevicePath;
            WwidDevicePath2 = (USB_WWID_DEVICE_PATH*)TmpDevicePath;
            if ((WwidDevicePath1->VendorId == WwidDevicePath2->VendorId) &&
                (WwidDevicePath1->ProductId == WwidDevicePath2->ProductId) &&
                (CompareMem (WwidDevicePath1+1, WwidDevicePath2+1, DevicePathNodeLength(WwidDevicePath1)-sizeof (USB_WWID_DEVICE_PATH)) == 0))
            {
              *DeviceHandle = UsbIoBuffer[Index];
              // Add the additional original Device Path Nodes (eg: FilePath Device Path Node) to the new Device Path
              *NewDevicePath = AppendDevicePath (UsbIoDevicePath, NextDevicePathNode (RemovableDevicePath));
              return EFI_SUCCESS;
            }
          } else {
            UsbClassDevicePath1 = (USB_CLASS_DEVICE_PATH*)RemovableDevicePath;
            UsbClassDevicePath2 = (USB_CLASS_DEVICE_PATH*)TmpDevicePath;
            if ((UsbClassDevicePath1->VendorId != 0xFFFF) && (UsbClassDevicePath1->VendorId == UsbClassDevicePath2->VendorId) &&
                (UsbClassDevicePath1->ProductId != 0xFFFF) && (UsbClassDevicePath1->ProductId == UsbClassDevicePath2->ProductId) &&
                (UsbClassDevicePath1->DeviceClass != 0xFF) && (UsbClassDevicePath1->DeviceClass == UsbClassDevicePath2->DeviceClass) &&
                (UsbClassDevicePath1->DeviceSubClass != 0xFF) && (UsbClassDevicePath1->DeviceSubClass == UsbClassDevicePath2->DeviceSubClass) &&
                (UsbClassDevicePath1->DeviceProtocol != 0xFF) && (UsbClassDevicePath1->DeviceProtocol == UsbClassDevicePath2->DeviceProtocol))
            {
              *DeviceHandle = UsbIoBuffer[Index];
              // Add the additional original Device Path Nodes (eg: FilePath Device Path Node) to the new Device Path
              *NewDevicePath = AppendDevicePath (UsbIoDevicePath, NextDevicePathNode (RemovableDevicePath));
              return EFI_SUCCESS;
            }
          }
        }
        TmpDevicePath = NextDevicePathNode (TmpDevicePath);
      }

    }
  }

  return EFI_NOT_FOUND;
}

BOOLEAN
BdsIsRemovableHd (
  IN  EFI_DEVICE_PATH*  DevicePath
  )
{
  return IS_DEVICE_PATH_NODE (DevicePath, MEDIA_DEVICE_PATH, MEDIA_HARDDRIVE_DP);
}

EFI_STATUS
BdsGetDeviceHd (
  IN  EFI_DEVICE_PATH*  RemovableDevicePath,
  OUT EFI_HANDLE*       DeviceHandle,
  OUT EFI_DEVICE_PATH** NewDevicePath
  )
{
  EFI_STATUS                    Status;
  UINTN                         Index;
  UINTN                         PartitionHandleCount;
  EFI_HANDLE                    *PartitionBuffer;
  EFI_DEVICE_PATH*              PartitionDevicePath;
  EFI_DEVICE_PATH*              TmpDevicePath;
  HARDDRIVE_DEVICE_PATH*        HardDriveDevicePath1;
  HARDDRIVE_DEVICE_PATH*        HardDriveDevicePath2;

  // Get all the DiskIo handles
  PartitionHandleCount = 0;
  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiDiskIoProtocolGuid, NULL, &PartitionHandleCount, &PartitionBuffer);
  if (EFI_ERROR (Status) || (PartitionHandleCount == 0)) {
    return Status;
  }

  // Check if one of the handles matches the Hard Disk Description
  for (Index = 0; Index < PartitionHandleCount; Index++) {
    Status = gBS->HandleProtocol (PartitionBuffer[Index], &gEfiDevicePathProtocolGuid, (VOID **) &PartitionDevicePath);
    if (!EFI_ERROR (Status)) {
      TmpDevicePath = PartitionDevicePath;
      while (!IsDevicePathEnd (TmpDevicePath)) {
        // Check if the Device Path node is a HD Removable device Path node
        if (BdsIsRemovableHd (TmpDevicePath)) {
          HardDriveDevicePath1 = (HARDDRIVE_DEVICE_PATH*)RemovableDevicePath;
          HardDriveDevicePath2 = (HARDDRIVE_DEVICE_PATH*)TmpDevicePath;
          if ((HardDriveDevicePath1->SignatureType == HardDriveDevicePath2->SignatureType) &&
              (CompareGuid ((EFI_GUID *)HardDriveDevicePath1->Signature, (EFI_GUID *)HardDriveDevicePath2->Signature) == TRUE) &&
              (HardDriveDevicePath1->PartitionNumber == HardDriveDevicePath2->PartitionNumber))
          {
            *DeviceHandle = PartitionBuffer[Index];
            // Add the additional original Device Path Nodes (eg: FilePath Device Path Node) to the new Device Path
            *NewDevicePath = AppendDevicePath (PartitionDevicePath, NextDevicePathNode (RemovableDevicePath));
            return EFI_SUCCESS;
          }
        }
        TmpDevicePath = NextDevicePathNode (TmpDevicePath);
      }

    }
  }

  return EFI_NOT_FOUND;
}

/*BOOLEAN
BdsIsRemovableCdrom (
  IN  EFI_DEVICE_PATH*  DevicePath
  )
{
  return IS_DEVICE_PATH_NODE (DevicePath, MEDIA_DEVICE_PATH, MEDIA_CDROM_DP);
}

EFI_STATUS
BdsGetDeviceCdrom (
  IN  EFI_DEVICE_PATH*  RemovableDevicePath,
  OUT EFI_HANDLE*       DeviceHandle,
  OUT EFI_DEVICE_PATH** DevicePath
  )
{
  ASSERT(0);
  return EFI_UNSUPPORTED;
}*/

typedef BOOLEAN
(*BDS_IS_REMOVABLE) (
  IN  EFI_DEVICE_PATH*  DevicePath
  );

typedef EFI_STATUS
(*BDS_GET_DEVICE) (
  IN  EFI_DEVICE_PATH*  RemovableDevicePath,
  OUT EFI_HANDLE*       DeviceHandle,
  OUT EFI_DEVICE_PATH** DevicePath
  );

typedef struct {
  BDS_IS_REMOVABLE    IsRemovable;
  BDS_GET_DEVICE      GetDevice;
} BDS_REMOVABLE_DEVICE_SUPPORT;

BDS_REMOVABLE_DEVICE_SUPPORT  RemovableDeviceSupport[] = {
  { BdsIsRemovableUsb, BdsGetDeviceUsb },
  { BdsIsRemovableHd, BdsGetDeviceHd },
  //{ BdsIsRemovableCdrom, BdsGetDeviceCdrom }
};

STATIC
BOOLEAN
IsRemovableDevice (
  IN  EFI_DEVICE_PATH*  DevicePath
  )
{
  UINTN             Index;
  EFI_DEVICE_PATH*  TmpDevicePath;

  TmpDevicePath = DevicePath;
  while (!IsDevicePathEnd (TmpDevicePath)) {
    for (Index = 0; Index < sizeof (RemovableDeviceSupport) / sizeof (BDS_REMOVABLE_DEVICE_SUPPORT); Index++) {
      if (RemovableDeviceSupport[Index].IsRemovable (TmpDevicePath)) {
        return TRUE;
      }
    }
    TmpDevicePath = NextDevicePathNode (TmpDevicePath);
  }

  return FALSE;
}

STATIC
EFI_STATUS
TryRemovableDevice (
  IN  EFI_DEVICE_PATH*  DevicePath,
  OUT EFI_HANDLE*       DeviceHandle,
  OUT EFI_DEVICE_PATH** NewDevicePath
  )
{
  EFI_STATUS        Status;
  UINTN             Index;
  EFI_DEVICE_PATH*  TmpDevicePath;
  BDS_REMOVABLE_DEVICE_SUPPORT* RemovableDevice;
  EFI_DEVICE_PATH* RemovableDevicePath;
  BOOLEAN         RemovableFound;

  RemovableDevice     = NULL;
  RemovableDevicePath = NULL;
  RemovableFound      = FALSE;
  TmpDevicePath       = DevicePath;

  while (!IsDevicePathEnd (TmpDevicePath) && !RemovableFound) {
    for (Index = 0; Index < sizeof (RemovableDeviceSupport) / sizeof (BDS_REMOVABLE_DEVICE_SUPPORT); Index++) {
      RemovableDevice = &RemovableDeviceSupport[Index];
      if (RemovableDevice->IsRemovable (TmpDevicePath)) {
        RemovableDevicePath = TmpDevicePath;
        RemovableFound = TRUE;
        break;
      }
    }
    TmpDevicePath = NextDevicePathNode (TmpDevicePath);
  }

  if (!RemovableFound) {
    return EFI_NOT_FOUND;
  }

  // Search into the current started drivers
  Status = RemovableDevice->GetDevice (RemovableDevicePath, DeviceHandle, NewDevicePath);
  if (Status == EFI_NOT_FOUND) {
    // Connect all the drivers
    BdsConnectAllDrivers ();

    // Search again into all the drivers
    Status = RemovableDevice->GetDevice (RemovableDevicePath, DeviceHandle, NewDevicePath);
  }

  return Status;
}

STATIC
EFI_STATUS
BdsConnectAndUpdateDevicePath (
  IN OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePath,
  OUT    EFI_HANDLE                *Handle,
  OUT    EFI_DEVICE_PATH_PROTOCOL  **RemainingDevicePath
  )
{
  EFI_DEVICE_PATH*            Remaining;
  EFI_DEVICE_PATH*            NewDevicePath;
  EFI_STATUS                  Status;
  EFI_HANDLE                  PreviousHandle;

  if ((DevicePath == NULL) || (*DevicePath == NULL) || (Handle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  PreviousHandle = NULL;
  do {
    Remaining = *DevicePath;

    // The LocateDevicePath() function locates all devices on DevicePath that support Protocol and returns
    // the handle to the device that is closest to DevicePath. On output, the device path pointer is modified
    // to point to the remaining part of the device path
    Status = gBS->LocateDevicePath (&gEfiDevicePathProtocolGuid, &Remaining, Handle);

    if (!EFI_ERROR (Status)) {
      if (*Handle == PreviousHandle) {
        //
        // If no forward progress is made try invoking the Dispatcher.
        // A new FV may have been added to the system and new drivers
        // may now be found.
        // Status == EFI_SUCCESS means a driver was dispatched
        // Status == EFI_NOT_FOUND means no new drivers were dispatched
        //
        Status = gDS->Dispatch ();
      }

      if (!EFI_ERROR (Status)) {
        PreviousHandle = *Handle;

        // Recursive = FALSE: We do not want to start the whole device tree
        Status = gBS->ConnectController (*Handle, NULL, Remaining, FALSE);
      }
    }
  } while (!EFI_ERROR (Status) && !IsDevicePathEnd (Remaining));

  if (!EFI_ERROR (Status)) {
    // Now, we have got the whole Device Path connected, call again ConnectController to ensure all the supported Driver
    // Binding Protocol are connected (such as DiskIo and SimpleFileSystem)
    Remaining = *DevicePath;
    Status = gBS->LocateDevicePath (&gEfiDevicePathProtocolGuid, &Remaining, Handle);
    if (!EFI_ERROR (Status)) {
      Status = gBS->ConnectController (*Handle, NULL, Remaining, FALSE);
      if (EFI_ERROR (Status)) {
        // If the last node is a Memory Map Device Path just return EFI_SUCCESS.
        if ((Remaining->Type == HARDWARE_DEVICE_PATH) && (Remaining->SubType == HW_MEMMAP_DP)) {
            Status = EFI_SUCCESS;
        }
      }
    }
  } else if (!IsDevicePathEnd (Remaining) && !IsRemovableDevice (Remaining)) {

    /*// If the remaining Device Path is a FilePath or MemoryMap then we consider the Device Path has been loaded correctly
    if ((Remaining->Type == MEDIA_DEVICE_PATH) && (Remaining->SubType == MEDIA_FILEPATH_DP)) {
      Status = EFI_SUCCESS;
    } else if ((Remaining->Type == HARDWARE_DEVICE_PATH) && (Remaining->SubType == HW_MEMMAP_DP)) {
      Status = EFI_SUCCESS;
    }*/

    //TODO: Should we just return success and leave the caller decide if it is the expected RemainingPath
    Status = EFI_SUCCESS;
  } else {
    Status = TryRemovableDevice (*DevicePath, Handle, &NewDevicePath);
    if (!EFI_ERROR (Status)) {
      Status = BdsConnectAndUpdateDevicePath (&NewDevicePath, Handle, RemainingDevicePath);
      *DevicePath = NewDevicePath;
      return Status;
    }
  }

  if (RemainingDevicePath) {
    *RemainingDevicePath = Remaining;
  }

  return Status;
}

/**
  Connect a Device Path and return the handle of the driver that support this DevicePath

  @param  DevicePath            Device Path of the File to connect
  @param  Handle                Handle of the driver that support this DevicePath
  @param  RemainingDevicePath   Remaining DevicePath nodes that do not match the driver DevicePath

  @retval EFI_SUCCESS           A driver that matches the Device Path has been found
  @retval EFI_NOT_FOUND         No handles match the search.
  @retval EFI_INVALID_PARAMETER DevicePath or Handle is NULL

**/
EFI_STATUS
BdsConnectDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL* DevicePath,
  OUT EFI_HANDLE                *Handle,
  OUT EFI_DEVICE_PATH_PROTOCOL  **RemainingDevicePath
  )
{
  return BdsConnectAndUpdateDevicePath (&DevicePath, Handle, RemainingDevicePath);
}

BOOLEAN
BdsFileSystemSupport (
  IN EFI_DEVICE_PATH *DevicePath,
  IN EFI_HANDLE Handle,
  IN EFI_DEVICE_PATH *RemainingDevicePath
  )
{
  EFI_STATUS  Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL     *FsProtocol;

  Status = gBS->HandleProtocol (Handle, &gEfiSimpleFileSystemProtocolGuid, (VOID **)&FsProtocol);

  return (!EFI_ERROR (Status) && IS_DEVICE_PATH_NODE (RemainingDevicePath, MEDIA_DEVICE_PATH, MEDIA_FILEPATH_DP));
}

EFI_STATUS
BdsFileSystemLoadImage (
  IN OUT EFI_DEVICE_PATH       **DevicePath,
  IN     EFI_HANDLE            Handle,
  IN     EFI_DEVICE_PATH       *RemainingDevicePath,
  IN     EFI_ALLOCATE_TYPE     Type,
  IN OUT EFI_PHYSICAL_ADDRESS  *Image,
  OUT    UINTN                 *ImageSize
  )
{
  EFI_STATUS                       Status;
  FILEPATH_DEVICE_PATH             *FilePathDevicePath;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FsProtocol;
  EFI_FILE_PROTOCOL                *Fs;
  EFI_FILE_INFO                    *FileInfo;
  EFI_FILE_PROTOCOL                *File;
  UINTN                            Size;

  ASSERT (IS_DEVICE_PATH_NODE (RemainingDevicePath, MEDIA_DEVICE_PATH, MEDIA_FILEPATH_DP));

  FilePathDevicePath = (FILEPATH_DEVICE_PATH*)RemainingDevicePath;

  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID**)&FsProtocol,
                  gImageHandle,
                  Handle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Try to Open the volume and get root directory
  Status = FsProtocol->OpenVolume (FsProtocol, &Fs);
  if (EFI_ERROR (Status)) {
    goto CLOSE_PROTOCOL;
  }

  Status = Fs->Open (Fs, &File, FilePathDevicePath->PathName, EFI_FILE_MODE_READ, 0);
  if (EFI_ERROR (Status)) {
    goto CLOSE_PROTOCOL;
  }

  Size = 0;
  File->GetInfo (File, &gEfiFileInfoGuid, &Size, NULL);
  FileInfo = AllocatePool (Size);
  Status = File->GetInfo (File, &gEfiFileInfoGuid, &Size, FileInfo);
  if (EFI_ERROR (Status)) {
    goto CLOSE_FILE;
  }

  // Get the file size
  Size = FileInfo->FileSize;
  if (ImageSize) {
    *ImageSize = Size;
  }
  FreePool (FileInfo);

  Status = gBS->AllocatePages (Type, EfiBootServicesCode, EFI_SIZE_TO_PAGES(Size), Image);
  // Try to allocate in any pages if failed to allocate memory at the defined location
  if ((Status == EFI_OUT_OF_RESOURCES) && (Type != AllocateAnyPages)) {
    Status = gBS->AllocatePages (AllocateAnyPages, EfiBootServicesCode, EFI_SIZE_TO_PAGES(Size), Image);
  }
  if (!EFI_ERROR (Status)) {
    Status = File->Read (File, &Size, (VOID*)(UINTN)(*Image));
  }

CLOSE_FILE:
  File->Close (File);

CLOSE_PROTOCOL:
  gBS->CloseProtocol (
         Handle,
         &gEfiSimpleFileSystemProtocolGuid,
         gImageHandle,
         Handle);

  return Status;
}

BOOLEAN
BdsMemoryMapSupport (
  IN EFI_DEVICE_PATH *DevicePath,
  IN EFI_HANDLE Handle,
  IN EFI_DEVICE_PATH *RemainingDevicePath
  )
{
  return IS_DEVICE_PATH_NODE (DevicePath, HARDWARE_DEVICE_PATH, HW_MEMMAP_DP) ||
         IS_DEVICE_PATH_NODE (RemainingDevicePath, HARDWARE_DEVICE_PATH, HW_MEMMAP_DP);
}

EFI_STATUS
BdsMemoryMapLoadImage (
  IN OUT EFI_DEVICE_PATH       **DevicePath,
  IN     EFI_HANDLE            Handle,
  IN     EFI_DEVICE_PATH       *RemainingDevicePath,
  IN     EFI_ALLOCATE_TYPE     Type,
  IN OUT EFI_PHYSICAL_ADDRESS* Image,
  OUT    UINTN                 *ImageSize
  )
{
  EFI_STATUS            Status;
  MEMMAP_DEVICE_PATH*   MemMapPathDevicePath;
  UINTN                 Size;

  if (IS_DEVICE_PATH_NODE (RemainingDevicePath, HARDWARE_DEVICE_PATH, HW_MEMMAP_DP)) {
    MemMapPathDevicePath = (MEMMAP_DEVICE_PATH*)RemainingDevicePath;
  } else {
    ASSERT (IS_DEVICE_PATH_NODE (*DevicePath, HARDWARE_DEVICE_PATH, HW_MEMMAP_DP));
    MemMapPathDevicePath = (MEMMAP_DEVICE_PATH*)*DevicePath;
  }

  Size = MemMapPathDevicePath->EndingAddress - MemMapPathDevicePath->StartingAddress;
  if (Size == 0) {
      return EFI_INVALID_PARAMETER;
  }

  Status = gBS->AllocatePages (Type, EfiBootServicesCode, EFI_SIZE_TO_PAGES(Size), Image);
  // Try to allocate in any pages if failed to allocate memory at the defined location
  if ((Status == EFI_OUT_OF_RESOURCES) && (Type != AllocateAnyPages)) {
    Status = gBS->AllocatePages (AllocateAnyPages, EfiBootServicesCode, EFI_SIZE_TO_PAGES(Size), Image);
  }
  if (!EFI_ERROR (Status)) {
    CopyMem ((VOID*)(UINTN)(*Image), (CONST VOID*)(UINTN)MemMapPathDevicePath->StartingAddress, Size);

    if (ImageSize != NULL) {
        *ImageSize = Size;
    }
  }

  return Status;
}

BOOLEAN
BdsFirmwareVolumeSupport (
  IN EFI_DEVICE_PATH *DevicePath,
  IN EFI_HANDLE Handle,
  IN EFI_DEVICE_PATH *RemainingDevicePath
  )
{
  return IS_DEVICE_PATH_NODE (RemainingDevicePath, MEDIA_DEVICE_PATH, MEDIA_PIWG_FW_FILE_DP);
}

EFI_STATUS
BdsFirmwareVolumeLoadImage (
  IN OUT EFI_DEVICE_PATH       **DevicePath,
  IN     EFI_HANDLE            Handle,
  IN     EFI_DEVICE_PATH       *RemainingDevicePath,
  IN     EFI_ALLOCATE_TYPE     Type,
  IN OUT EFI_PHYSICAL_ADDRESS* Image,
  OUT    UINTN                 *ImageSize
  )
{
  EFI_STATUS            Status;
  EFI_FIRMWARE_VOLUME2_PROTOCOL     *FwVol;
  EFI_GUID                          *FvNameGuid;
  EFI_SECTION_TYPE                  SectionType;
  EFI_FV_FILETYPE                   FvType;
  EFI_FV_FILE_ATTRIBUTES            Attrib;
  UINT32                            AuthenticationStatus;
  VOID* ImageBuffer;

  ASSERT (IS_DEVICE_PATH_NODE (RemainingDevicePath, MEDIA_DEVICE_PATH, MEDIA_PIWG_FW_FILE_DP));

  Status = gBS->HandleProtocol (Handle, &gEfiFirmwareVolume2ProtocolGuid, (VOID **)&FwVol);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  FvNameGuid = EfiGetNameGuidFromFwVolDevicePathNode ((CONST MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *)RemainingDevicePath);
  if (FvNameGuid == NULL) {
    Status = EFI_INVALID_PARAMETER;
  }

  SectionType = EFI_SECTION_PE32;
  AuthenticationStatus = 0;
  //Note: ReadSection at the opposite of ReadFile does not allow to pass ImageBuffer == NULL to get the size of the file.
  ImageBuffer = NULL;
  Status = FwVol->ReadSection (
                    FwVol,
                    FvNameGuid,
                    SectionType,
                    0,
                    &ImageBuffer,
                    ImageSize,
                    &AuthenticationStatus
                    );
  if (!EFI_ERROR (Status)) {
#if 0
    // In case the buffer has some address requirements, we must copy the buffer to a buffer following the requirements
    if (Type != AllocateAnyPages) {
      Status = gBS->AllocatePages (Type, EfiBootServicesCode, EFI_SIZE_TO_PAGES(*ImageSize),Image);
      if (!EFI_ERROR (Status)) {
        CopyMem ((VOID*)(UINTN)(*Image), ImageBuffer, *ImageSize);
        FreePool (ImageBuffer);
      }
    }
#else
    // We must copy the buffer into a page allocations. Otherwise, the caller could call gBS->FreePages() on the pool allocation
    Status = gBS->AllocatePages (Type, EfiBootServicesCode, EFI_SIZE_TO_PAGES(*ImageSize), Image);
    // Try to allocate in any pages if failed to allocate memory at the defined location
    if ((Status == EFI_OUT_OF_RESOURCES) && (Type != AllocateAnyPages)) {
      Status = gBS->AllocatePages (AllocateAnyPages, EfiBootServicesCode, EFI_SIZE_TO_PAGES(*ImageSize), Image);
    }
    if (!EFI_ERROR (Status)) {
      CopyMem ((VOID*)(UINTN)(*Image), ImageBuffer, *ImageSize);
      FreePool (ImageBuffer);
    }
#endif
  } else {
    // Try a raw file, since a PE32 SECTION does not exist
    Status = FwVol->ReadFile (
                        FwVol,
                        FvNameGuid,
                        NULL,
                        ImageSize,
                        &FvType,
                        &Attrib,
                        &AuthenticationStatus
                        );
    if (!EFI_ERROR (Status)) {
      Status = gBS->AllocatePages (Type, EfiBootServicesCode, EFI_SIZE_TO_PAGES(*ImageSize), Image);
      // Try to allocate in any pages if failed to allocate memory at the defined location
      if ((Status == EFI_OUT_OF_RESOURCES) && (Type != AllocateAnyPages)) {
        Status = gBS->AllocatePages (AllocateAnyPages, EfiBootServicesCode, EFI_SIZE_TO_PAGES(*ImageSize), Image);
      }
      if (!EFI_ERROR (Status)) {
        Status = FwVol->ReadFile (
                                FwVol,
                                FvNameGuid,
                                (VOID**)Image,
                                ImageSize,
                                &FvType,
                                &Attrib,
                                &AuthenticationStatus
                                );
      }
    }
  }
  return Status;
}

BOOLEAN
BdsPxeSupport (
  IN EFI_DEVICE_PATH*           DevicePath,
  IN EFI_HANDLE                 Handle,
  IN EFI_DEVICE_PATH*           RemainingDevicePath
  )
{
  EFI_STATUS                  Status;
  EFI_PXE_BASE_CODE_PROTOCOL* PxeBcProtocol;

  if (!IsDevicePathEnd (RemainingDevicePath)) {
    return FALSE;
  }

  Status = gBS->HandleProtocol (Handle, &gEfiPxeBaseCodeProtocolGuid, (VOID **)&PxeBcProtocol);
  if (EFI_ERROR (Status)) {
    return FALSE;
  } else {
    return TRUE;
  }
}

EFI_STATUS
BdsPxeLoadImage (
  IN OUT EFI_DEVICE_PATH       **DevicePath,
  IN     EFI_HANDLE            Handle,
  IN     EFI_DEVICE_PATH       *RemainingDevicePath,
  IN     EFI_ALLOCATE_TYPE     Type,
  IN OUT EFI_PHYSICAL_ADDRESS* Image,
  OUT    UINTN                 *ImageSize
  )
{
  EFI_STATUS              Status;
  EFI_LOAD_FILE_PROTOCOL  *LoadFileProtocol;
  UINTN                   BufferSize;
  EFI_PXE_BASE_CODE_PROTOCOL *Pxe;

  // Get Load File Protocol attached to the PXE protocol
  Status = gBS->HandleProtocol (Handle, &gEfiLoadFileProtocolGuid, (VOID **)&LoadFileProtocol);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = LoadFileProtocol->LoadFile (LoadFileProtocol, *DevicePath, TRUE, &BufferSize, NULL);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    Status = gBS->AllocatePages (Type, EfiBootServicesCode, EFI_SIZE_TO_PAGES(BufferSize), Image);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = LoadFileProtocol->LoadFile (LoadFileProtocol, *DevicePath, TRUE, &BufferSize, (VOID*)(UINTN)(*Image));
    if (!EFI_ERROR (Status) && (ImageSize != NULL)) {
      *ImageSize = BufferSize;
    }
  }

  if (Status == EFI_ALREADY_STARTED) {
    Status = gBS->LocateProtocol (&gEfiPxeBaseCodeProtocolGuid, NULL, (VOID **)&Pxe);
    if (!EFI_ERROR(Status)) {
      // If PXE is already started, we stop it
      Pxe->Stop (Pxe);
      // And we try again
      return BdsPxeLoadImage (DevicePath, Handle, RemainingDevicePath, Type, Image, ImageSize);
    }
  }
  return Status;
}

BOOLEAN
BdsTftpSupport (
  IN EFI_DEVICE_PATH  *DevicePath,
  IN EFI_HANDLE       Handle,
  IN EFI_DEVICE_PATH  *RemainingDevicePath
  )
{
  EFI_STATUS       Status;
  EFI_DEVICE_PATH  *NextDevicePath;
  VOID             *Interface;

  // Validate the Remaining Device Path
  if (IsDevicePathEnd (RemainingDevicePath)) {
    return FALSE;
  }
  if (!IS_DEVICE_PATH_NODE (RemainingDevicePath, MESSAGING_DEVICE_PATH, MSG_IPv4_DP) &&
      !IS_DEVICE_PATH_NODE (RemainingDevicePath, MESSAGING_DEVICE_PATH, MSG_IPv6_DP)) {
    return FALSE;
  }
  NextDevicePath = NextDevicePathNode (RemainingDevicePath);
  if (IsDevicePathEnd (NextDevicePath)) {
    return FALSE;
  }
  if (!IS_DEVICE_PATH_NODE (NextDevicePath, MEDIA_DEVICE_PATH, MEDIA_FILEPATH_DP)) {
    return FALSE;
  }

  Status = gBS->HandleProtocol (
                  Handle, &gEfiDevicePathProtocolGuid,
                  &Interface
                  );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  //
  // Check that the controller (identified by its handle "Handle") supports the
  // MTFTPv4 Service Binding Protocol. If it does, it means that it supports the
  // EFI MTFTPv4 Protocol needed to download the image through TFTP.
  //
  Status = gBS->HandleProtocol (
                  Handle, &gEfiMtftp4ServiceBindingProtocolGuid,
                  &Interface
                  );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  return TRUE;
}

/**
  Worker function that get the size in numbers of bytes of a file from a TFTP
  server before to download the file.

  @param[in]   Mtftp4    MTFTP4 protocol interface
  @param[in]   FilePath  Path of the file, Ascii encoded
  @param[out]  FileSize  Address where to store the file size in number of
                         bytes.

  @retval  EFI_SUCCESS   The size of the file was returned.
  @retval  !EFI_SUCCESS  The size of the file was not returned.

**/
STATIC
EFI_STATUS
Mtftp4GetFileSize (
  IN  EFI_MTFTP4_PROTOCOL  *Mtftp4,
  IN  CHAR8                *FilePath,
  OUT UINT64               *FileSize
  )
{
  EFI_STATUS         Status;
  EFI_MTFTP4_OPTION  ReqOpt[1];
  EFI_MTFTP4_PACKET  *Packet;
  UINT32             PktLen;
  EFI_MTFTP4_OPTION  *TableOfOptions;
  EFI_MTFTP4_OPTION  *Option;
  UINT32             OptCnt;
  UINT8              OptBuf[128];

  ReqOpt[0].OptionStr = (UINT8*)"tsize";
  OptBuf[0] = '0';
  OptBuf[1] = 0;
  ReqOpt[0].ValueStr = OptBuf;

  Status = Mtftp4->GetInfo (
             Mtftp4,
             NULL,
             (UINT8*)FilePath,
             NULL,
             1,
             ReqOpt,
             &PktLen,
             &Packet
             );

  if (EFI_ERROR (Status)) {
    goto Error;
  }

  Status = Mtftp4->ParseOptions (
                     Mtftp4,
                     PktLen,
                     Packet,
                     (UINT32 *) &OptCnt,
                     &TableOfOptions
                     );
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  Option = TableOfOptions;
  while (OptCnt != 0) {
    if (AsciiStrnCmp ((CHAR8 *)Option->OptionStr, "tsize", 5) == 0) {
      *FileSize = AsciiStrDecimalToUint64 ((CHAR8 *)Option->ValueStr);
      break;
    }
    OptCnt--;
    Option++;
  }
  FreePool (TableOfOptions);

  if (OptCnt == 0) {
    Status = EFI_UNSUPPORTED;
  }

Error :

  return Status;
}

/**
  Update the progress of a file download
  This procedure is called each time a new TFTP packet is received.

  @param[in]  This       MTFTP4 protocol interface
  @param[in]  Token      Parameters for the download of the file
  @param[in]  PacketLen  Length of the packet
  @param[in]  Packet     Address of the packet

  @retval  EFI_SUCCESS  All packets are accepted.

**/
STATIC
EFI_STATUS
Mtftp4CheckPacket (
  IN EFI_MTFTP4_PROTOCOL  *This,
  IN EFI_MTFTP4_TOKEN     *Token,
  IN UINT16               PacketLen,
  IN EFI_MTFTP4_PACKET    *Packet
  )
{
  BDS_TFTP_CONTEXT  *Context;
  CHAR16            Progress[TFTP_PROGRESS_MESSAGE_SIZE];
  UINT64            NbOfKb;
  UINTN             Index;
  UINTN             LastStep;
  UINTN             Step;
  UINT64            LastNbOf50Kb;
  UINT64            NbOf50Kb;

  if ((NTOHS (Packet->OpCode)) == EFI_MTFTP4_OPCODE_DATA) {
    Context = (BDS_TFTP_CONTEXT*)Token->Context;

    if (Context->DownloadedNbOfBytes == 0) {
      if (Context->FileSize > 0) {
        Print (L"%s       0 Kb", mTftpProgressFrame);
      } else {
        Print (L"    0 Kb");
      }
    }

    //
    // The data is the packet are prepended with two UINT16 :
    // . OpCode = EFI_MTFTP4_OPCODE_DATA
    // . Block  = the number of this block of data
    //
    Context->DownloadedNbOfBytes += PacketLen - sizeof (Packet->OpCode) - sizeof (Packet->Data.Block);
    NbOfKb = Context->DownloadedNbOfBytes / 1024;

    Progress[0] = L'\0';
    if (Context->FileSize > 0) {
      LastStep  = (Context->LastReportedNbOfBytes * TFTP_PROGRESS_SLIDER_STEPS) / Context->FileSize;
      Step      = (Context->DownloadedNbOfBytes   * TFTP_PROGRESS_SLIDER_STEPS) / Context->FileSize;
      if (Step > LastStep) {
        Print (mTftpProgressDelete);
        StrCpy (Progress, mTftpProgressFrame);
        for (Index = 1; Index < Step; Index++) {
          Progress[Index] = L'=';
        }
        Progress[Step] = L'>';

        UnicodeSPrint (
          Progress + (sizeof (mTftpProgressFrame) / sizeof (CHAR16)) - 1,
          sizeof (Progress) - sizeof (mTftpProgressFrame),
          L" %7d Kb",
          NbOfKb
          );
        Context->LastReportedNbOfBytes = Context->DownloadedNbOfBytes;
      }
    } else {
      //
      // Case when we do not know the size of the final file.
      // We print the updated size every 50KB of downloaded data
      //
      LastNbOf50Kb = Context->LastReportedNbOfBytes / (50*1024);
      NbOf50Kb     = Context->DownloadedNbOfBytes   / (50*1024);
      if (NbOf50Kb > LastNbOf50Kb) {
        Print (L"\b\b\b\b\b\b\b\b\b\b");
        UnicodeSPrint (Progress, sizeof (Progress), L"%7d Kb", NbOfKb);
        Context->LastReportedNbOfBytes = Context->DownloadedNbOfBytes;
      }
    }
    if (Progress[0] != L'\0') {
      Print (L"%s", Progress);
    }
  }

  return EFI_SUCCESS;
}

/**
  Download an image from a TFTP server

  @param[in]   DevicePath           Device path of the TFTP boot option
  @param[in]   ControllerHandle     Handle of the network controller
  @param[in]   RemainingDevicePath  Device path of the TFTP boot option but
                                    the first node that identifies the network controller
  @param[in]   Type                 Type to allocate memory pages
  @param[out]  Image                Address of the bufer where the image is stored in
                                    case of success
  @param[out]  ImageSize            Size in number of bytes of the i;age in case of
                                    success

  @retval  EFI_SUCCESS   The image was returned.
  @retval  !EFI_SUCCESS  Something went wrong.

**/
EFI_STATUS
BdsTftpLoadImage (
  IN OUT EFI_DEVICE_PATH       **DevicePath,
  IN     EFI_HANDLE            ControllerHandle,
  IN     EFI_DEVICE_PATH       *RemainingDevicePath,
  IN     EFI_ALLOCATE_TYPE     Type,
  IN OUT EFI_PHYSICAL_ADDRESS  *Image,
  OUT    UINTN                 *ImageSize
  )
{
  EFI_STATUS               Status;
  EFI_HANDLE               Dhcp4ChildHandle;
  EFI_DHCP4_PROTOCOL       *Dhcp4;
  BOOLEAN                  Dhcp4ToStop;
  EFI_HANDLE               Mtftp4ChildHandle;
  EFI_MTFTP4_PROTOCOL      *Mtftp4;
  DHCP4_OPTION             ParaList;
  EFI_DHCP4_PACKET_OPTION  *OptionList[2];
  EFI_DHCP4_CONFIG_DATA    Dhcp4CfgData;
  EFI_DHCP4_MODE_DATA      Dhcp4Mode;
  EFI_MTFTP4_CONFIG_DATA   Mtftp4CfgData;
  IPv4_DEVICE_PATH         *IPv4DevicePathNode;
  CHAR16                   *PathName;
  CHAR8                    *AsciiFilePath;
  EFI_MTFTP4_TOKEN         Mtftp4Token;
  UINT64                   FileSize;
  UINT64                   TftpBufferSize;
  BDS_TFTP_CONTEXT         *TftpContext;

  ASSERT(IS_DEVICE_PATH_NODE (RemainingDevicePath, MESSAGING_DEVICE_PATH, MSG_IPv4_DP));
  IPv4DevicePathNode = (IPv4_DEVICE_PATH*)RemainingDevicePath;

  Dhcp4ChildHandle  = NULL;
  Dhcp4             = NULL;
  Dhcp4ToStop       = FALSE;
  Mtftp4ChildHandle = NULL;
  Mtftp4            = NULL;
  AsciiFilePath     = NULL;
  TftpContext       = NULL;

  if (!IPv4DevicePathNode->StaticIpAddress) {
    //
    // Using the DHCP4 Service Binding Protocol, create a child handle of the DHCP4 service and
    // install the DHCP4 protocol on it. Then, open the DHCP protocol.
    //
    Status = NetLibCreateServiceChild (
               ControllerHandle,
               gImageHandle,
               &gEfiDhcp4ServiceBindingProtocolGuid,
               &Dhcp4ChildHandle
               );
    if (!EFI_ERROR (Status)) {
      Status = gBS->OpenProtocol (
                      Dhcp4ChildHandle,
                      &gEfiDhcp4ProtocolGuid,
                      (VOID **) &Dhcp4,
                      gImageHandle,
                      ControllerHandle,
                      EFI_OPEN_PROTOCOL_BY_DRIVER
                      );
    }
    if (EFI_ERROR (Status)) {
      Print (L"Unable to open DHCP4 protocol\n");
      goto Error;
    }
  }

  //
  // Using the MTFTP4 Service Binding Protocol, create a child handle of the MTFTP4 service and
  // install the MTFTP4 protocol on it. Then, open the MTFTP4 protocol.
  //
  Status = NetLibCreateServiceChild (
             ControllerHandle,
             gImageHandle,
             &gEfiMtftp4ServiceBindingProtocolGuid,
             &Mtftp4ChildHandle
             );
  if (!EFI_ERROR (Status)) {
    Status = gBS->OpenProtocol (
                    Mtftp4ChildHandle,
                    &gEfiMtftp4ProtocolGuid,
                    (VOID **) &Mtftp4,
                    gImageHandle,
                    ControllerHandle,
                    EFI_OPEN_PROTOCOL_BY_DRIVER
                    );
  }
  if (EFI_ERROR (Status)) {
    Print (L"Unable to open MTFTP4 protocol\n");
    goto Error;
  }

  if (!IPv4DevicePathNode->StaticIpAddress) {
    //
    // Configure the DHCP4, all default settings. It is acceptable for the configuration to
    // fail if the return code is equal to EFI_ACCESS_DENIED which means that the configuration
    // has been done by another instance of the DHCP4 protocol or that the DHCP configuration
    // process has been started but is not completed yet.
    //
    ZeroMem (&Dhcp4CfgData, sizeof (EFI_DHCP4_CONFIG_DATA));
    ParaList.Head.OpCode     = DHCP_TAG_PARA_LIST;
    ParaList.Head.Length     = 2;
    ParaList.Head.Data[0]    = DHCP_TAG_NETMASK;
    ParaList.Route           = DHCP_TAG_ROUTER;
    OptionList[0]            = &ParaList.Head;
    Dhcp4CfgData.OptionCount = 1;
    Dhcp4CfgData.OptionList  = OptionList;

    Status = Dhcp4->Configure (Dhcp4, &Dhcp4CfgData);
    if (EFI_ERROR (Status)) {
      if (Status != EFI_ACCESS_DENIED) {
        Print (L"Error while configuring the DHCP4 protocol\n");
        goto Error;
      }
    }

    //
    // Start the DHCP configuration. This may have already been done thus do not leave in error
    // if the return code is EFI_ALREADY_STARTED.
    //
    Status = Dhcp4->Start (Dhcp4, NULL);
    if (EFI_ERROR (Status)) {
      if (Status != EFI_ALREADY_STARTED) {
        Print (L"DHCP configuration failed\n");
        goto Error;
      }
    } else {
      Dhcp4ToStop = TRUE;
    }

    Status = Dhcp4->GetModeData (Dhcp4, &Dhcp4Mode);
    if (EFI_ERROR (Status)) {
      goto Error;
    }

    if (Dhcp4Mode.State != Dhcp4Bound) {
      Status = EFI_TIMEOUT;
      Print (L"DHCP configuration failed\n");
      goto Error;
    }
  }

  //
  // Configure the TFTP4 protocol
  //

  ZeroMem (&Mtftp4CfgData, sizeof (EFI_MTFTP4_CONFIG_DATA));
  Mtftp4CfgData.UseDefaultSetting = FALSE;
  Mtftp4CfgData.TimeoutValue      = 4;
  Mtftp4CfgData.TryCount          = 6;

  if (IPv4DevicePathNode->StaticIpAddress) {
    CopyMem (&Mtftp4CfgData.StationIp , &IPv4DevicePathNode->LocalIpAddress, sizeof (EFI_IPv4_ADDRESS));
    CopyMem (&Mtftp4CfgData.SubnetMask, &IPv4DevicePathNode->SubnetMask, sizeof (EFI_IPv4_ADDRESS));
    CopyMem (&Mtftp4CfgData.GatewayIp , &IPv4DevicePathNode->GatewayIpAddress, sizeof (EFI_IPv4_ADDRESS));
  } else {
    CopyMem (&Mtftp4CfgData.StationIp , &Dhcp4Mode.ClientAddress, sizeof (EFI_IPv4_ADDRESS));
    CopyMem (&Mtftp4CfgData.SubnetMask, &Dhcp4Mode.SubnetMask   , sizeof (EFI_IPv4_ADDRESS));
    CopyMem (&Mtftp4CfgData.GatewayIp , &Dhcp4Mode.RouterAddress, sizeof (EFI_IPv4_ADDRESS));
  }

  CopyMem (&Mtftp4CfgData.ServerIp  , &IPv4DevicePathNode->RemoteIpAddress, sizeof (EFI_IPv4_ADDRESS));

  Status = Mtftp4->Configure (Mtftp4, &Mtftp4CfgData);
  if (EFI_ERROR (Status)) {
    Print (L"Error while configuring the MTFTP4 protocol\n");
    goto Error;
  }

  // The Device Path might contain multiple FilePath nodes
  PathName      = ConvertDevicePathToText ((EFI_DEVICE_PATH_PROTOCOL*)(IPv4DevicePathNode + 1), FALSE, FALSE);
  AsciiFilePath = AllocatePool (StrLen (PathName) + 1);
  UnicodeStrToAsciiStr (PathName, AsciiFilePath);

  //
  // Try to get the size of the file in bytes from the server. If it fails,
  // start with a 8MB buffer to download the file.
  //
  FileSize = 0;
  if (Mtftp4GetFileSize (Mtftp4, AsciiFilePath, &FileSize) == EFI_SUCCESS) {
    TftpBufferSize = FileSize;
  } else {
    TftpBufferSize = SIZE_8MB;
  }

  TftpContext = AllocatePool (sizeof (BDS_TFTP_CONTEXT));
  if (TftpContext == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }
  TftpContext->FileSize = FileSize;

  for (; TftpBufferSize <= FixedPcdGet32 (PcdMaxTftpFileSize);
         TftpBufferSize = (TftpBufferSize + SIZE_8MB) & (~(SIZE_8MB-1))) {
    //
    // Allocate a buffer to hold the whole file.
    //
    Status = gBS->AllocatePages (
                    Type,
                    EfiBootServicesCode,
                    EFI_SIZE_TO_PAGES (TftpBufferSize),
                    Image
                    );
    if (EFI_ERROR (Status)) {
      Print (L"Failed to allocate space for image\n");
      goto Error;
    }

    TftpContext->DownloadedNbOfBytes   = 0;
    TftpContext->LastReportedNbOfBytes = 0;

    ZeroMem (&Mtftp4Token, sizeof (EFI_MTFTP4_TOKEN));
    Mtftp4Token.Filename    = (UINT8*)AsciiFilePath;
    Mtftp4Token.BufferSize  = TftpBufferSize;
    Mtftp4Token.Buffer      = (VOID *)(UINTN)*Image;
    Mtftp4Token.CheckPacket = Mtftp4CheckPacket;
    Mtftp4Token.Context     = (VOID*)TftpContext;

    Print (L"Downloading the file <%a> from the TFTP server\n", AsciiFilePath);
    Status = Mtftp4->ReadFile (Mtftp4, &Mtftp4Token);
    Print (L"\n");
    if (EFI_ERROR (Status)) {
      gBS->FreePages (*Image, EFI_SIZE_TO_PAGES (TftpBufferSize));
      if (Status == EFI_BUFFER_TOO_SMALL) {
        Print (L"Downloading failed, file larger than expected.\n");
        continue;
      } else {
        goto Error;
      }
    }

    *ImageSize = Mtftp4Token.BufferSize;
    break;
  }

Error:
  if (Dhcp4ChildHandle != NULL) {
    if (Dhcp4 != NULL) {
      if (Dhcp4ToStop) {
        Dhcp4->Stop (Dhcp4);
      }
      gBS->CloseProtocol (
             Dhcp4ChildHandle,
             &gEfiDhcp4ProtocolGuid,
             gImageHandle,
             ControllerHandle
            );
    }
    NetLibDestroyServiceChild (
      ControllerHandle,
      gImageHandle,
      &gEfiDhcp4ServiceBindingProtocolGuid,
      Dhcp4ChildHandle
      );
  }

  if (Mtftp4ChildHandle != NULL) {
    if (Mtftp4 != NULL) {
      if (AsciiFilePath != NULL) {
        FreePool (AsciiFilePath);
      }
      if (TftpContext != NULL) {
        FreePool (TftpContext);
      }
      gBS->CloseProtocol (
             Mtftp4ChildHandle,
             &gEfiMtftp4ProtocolGuid,
             gImageHandle,
             ControllerHandle
            );
    }
    NetLibDestroyServiceChild (
      ControllerHandle,
      gImageHandle,
      &gEfiMtftp4ServiceBindingProtocolGuid,
      Mtftp4ChildHandle
      );
  }

  if (EFI_ERROR (Status)) {
    *Image = 0;
    Print (L"Failed to download the file - Error=%r\n", Status);
  }

  return Status;
}

BDS_FILE_LOADER FileLoaders[] = {
    { BdsFileSystemSupport, BdsFileSystemLoadImage },
    { BdsFirmwareVolumeSupport, BdsFirmwareVolumeLoadImage },
    //{ BdsLoadFileSupport, BdsLoadFileLoadImage },
    { BdsMemoryMapSupport, BdsMemoryMapLoadImage },
    { BdsPxeSupport, BdsPxeLoadImage },
    { BdsTftpSupport, BdsTftpLoadImage },
    { NULL, NULL }
};

EFI_STATUS
BdsLoadImageAndUpdateDevicePath (
  IN OUT EFI_DEVICE_PATH       **DevicePath,
  IN     EFI_ALLOCATE_TYPE     Type,
  IN OUT EFI_PHYSICAL_ADDRESS* Image,
  OUT    UINTN                 *FileSize
  )
{
  EFI_STATUS      Status;
  EFI_HANDLE      Handle;
  EFI_DEVICE_PATH *RemainingDevicePath;
  BDS_FILE_LOADER*  FileLoader;

  Status = BdsConnectAndUpdateDevicePath (DevicePath, &Handle, &RemainingDevicePath);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  FileLoader = FileLoaders;
  while (FileLoader->Support != NULL) {
    if (FileLoader->Support (*DevicePath, Handle, RemainingDevicePath)) {
      return FileLoader->LoadImage (DevicePath, Handle, RemainingDevicePath, Type, Image, FileSize);
    }
    FileLoader++;
  }

  return EFI_UNSUPPORTED;
}

EFI_STATUS
BdsLoadImage (
  IN     EFI_DEVICE_PATH       *DevicePath,
  IN     EFI_ALLOCATE_TYPE     Type,
  IN OUT EFI_PHYSICAL_ADDRESS* Image,
  OUT    UINTN                 *FileSize
  )
{
  return BdsLoadImageAndUpdateDevicePath (&DevicePath, Type, Image, FileSize);
}

/**
  Start an EFI Application from a Device Path

  @param  ParentImageHandle     Handle of the calling image
  @param  DevicePath            Location of the EFI Application

  @retval EFI_SUCCESS           All drivers have been connected
  @retval EFI_NOT_FOUND         The Linux kernel Device Path has not been found
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource memory to store the matching results.

**/
EFI_STATUS
BdsStartEfiApplication (
  IN EFI_HANDLE                  ParentImageHandle,
  IN EFI_DEVICE_PATH_PROTOCOL    *DevicePath,
  IN UINTN                       LoadOptionsSize,
  IN VOID*                       LoadOptions
  )
{
  EFI_STATUS                   Status;
  EFI_HANDLE                   ImageHandle;
  EFI_PHYSICAL_ADDRESS         BinaryBuffer;
  UINTN                        BinarySize;
  EFI_LOADED_IMAGE_PROTOCOL*   LoadedImage;

  // Find the nearest supported file loader
  Status = BdsLoadImageAndUpdateDevicePath (&DevicePath, AllocateAnyPages, &BinaryBuffer, &BinarySize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Load the image from the Buffer with Boot Services function
  Status = gBS->LoadImage (TRUE, ParentImageHandle, DevicePath, (VOID*)(UINTN)BinaryBuffer, BinarySize, &ImageHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Passed LoadOptions to the EFI Application
  if (LoadOptionsSize != 0) {
    Status = gBS->HandleProtocol (ImageHandle, &gEfiLoadedImageProtocolGuid, (VOID **) &LoadedImage);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    LoadedImage->LoadOptionsSize  = LoadOptionsSize;
    LoadedImage->LoadOptions      = LoadOptions;
  }

  // Before calling the image, enable the Watchdog Timer for  the 5 Minute period
  gBS->SetWatchdogTimer (5 * 60, 0x0000, 0x00, NULL);
  // Start the image
  Status = gBS->StartImage (ImageHandle, NULL, NULL);
  // Clear the Watchdog Timer after the image returns
  gBS->SetWatchdogTimer (0x0000, 0x0000, 0x0000, NULL);

  return Status;
}
