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

#include <Protocol/BlockIo.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/FirmwareVolumeBlock.h>
#include <Protocol/PxeBaseCode.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/SimpleNetwork.h>
#include <Protocol/Dhcp4.h>
#include <Protocol/Mtftp4.h>

#include <Guid/FileSystemInfo.h>

#define IS_DEVICE_PATH_NODE(node,type,subtype) (((node)->Type == (type)) && ((node)->SubType == (subtype)))

EFI_STATUS
BdsLoadOptionFileSystemList (
  IN OUT LIST_ENTRY* BdsLoadOptionList
  );

EFI_STATUS
BdsLoadOptionFileSystemCreateDevicePath (
  IN CHAR16*                    FileName,
  OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePathNodes
  );

EFI_STATUS
BdsLoadOptionFileSystemUpdateDevicePath (
  IN EFI_DEVICE_PATH            *OldDevicePath,
  IN CHAR16*                    FileName,
  OUT EFI_DEVICE_PATH_PROTOCOL  **NewDevicePath
  );

BOOLEAN
BdsLoadOptionFileSystemIsSupported (
  IN  EFI_DEVICE_PATH           *DevicePath
  );

EFI_STATUS
BdsLoadOptionMemMapList (
  IN OUT LIST_ENTRY* BdsLoadOptionList
  );

EFI_STATUS
BdsLoadOptionMemMapCreateDevicePath (
  IN CHAR16*                    FileName,
  OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePathNodes
  );

EFI_STATUS
BdsLoadOptionMemMapUpdateDevicePath (
  IN EFI_DEVICE_PATH            *OldDevicePath,
  IN CHAR16*                    FileName,
  OUT EFI_DEVICE_PATH_PROTOCOL  **NewDevicePath
  );

BOOLEAN
BdsLoadOptionMemMapIsSupported (
  IN  EFI_DEVICE_PATH           *DevicePath
  );

EFI_STATUS
BdsLoadOptionPxeList (
  IN OUT LIST_ENTRY* BdsLoadOptionList
  );

EFI_STATUS
BdsLoadOptionPxeCreateDevicePath (
  IN CHAR16*                    FileName,
  OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePathNodes
  );

EFI_STATUS
BdsLoadOptionPxeUpdateDevicePath (
  IN EFI_DEVICE_PATH            *OldDevicePath,
  IN CHAR16*                    FileName,
  OUT EFI_DEVICE_PATH_PROTOCOL  **NewDevicePath
  );

BOOLEAN
BdsLoadOptionPxeIsSupported (
  IN  EFI_DEVICE_PATH           *DevicePath
  );

EFI_STATUS
BdsLoadOptionTftpList (
  IN OUT LIST_ENTRY* BdsLoadOptionList
  );

EFI_STATUS
BdsLoadOptionTftpCreateDevicePath (
  IN CHAR16*                    FileName,
  OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePathNodes
  );

EFI_STATUS
BdsLoadOptionTftpUpdateDevicePath (
  IN EFI_DEVICE_PATH            *OldDevicePath,
  IN CHAR16*                    FileName,
  OUT EFI_DEVICE_PATH_PROTOCOL  **NewDevicePath
  );

BOOLEAN
BdsLoadOptionTftpIsSupported (
  IN  EFI_DEVICE_PATH           *DevicePath
  );

BDS_LOAD_OPTION_SUPPORT BdsLoadOptionSupportList[] = {
  {
    BDS_DEVICE_FILESYSTEM,
    BdsLoadOptionFileSystemList,
    BdsLoadOptionFileSystemIsSupported,
    BdsLoadOptionFileSystemCreateDevicePath,
    BdsLoadOptionFileSystemUpdateDevicePath,
    TRUE
  },
  {
    BDS_DEVICE_MEMMAP,
    BdsLoadOptionMemMapList,
    BdsLoadOptionMemMapIsSupported,
    BdsLoadOptionMemMapCreateDevicePath,
    BdsLoadOptionMemMapUpdateDevicePath,
    TRUE
  },
  {
    BDS_DEVICE_PXE,
    BdsLoadOptionPxeList,
    BdsLoadOptionPxeIsSupported,
    BdsLoadOptionPxeCreateDevicePath,
    BdsLoadOptionPxeUpdateDevicePath,
    FALSE
  },
  {
    BDS_DEVICE_TFTP,
    BdsLoadOptionTftpList,
    BdsLoadOptionTftpIsSupported,
    BdsLoadOptionTftpCreateDevicePath,
    BdsLoadOptionTftpUpdateDevicePath,
    TRUE
  }
};

EFI_STATUS
BootDeviceListSupportedInit (
  IN OUT LIST_ENTRY *SupportedDeviceList
  )
{
  UINTN   Index;

  // Initialize list of supported devices
  InitializeListHead (SupportedDeviceList);

  for (Index = 0; Index < BDS_DEVICE_MAX; Index++) {
    BdsLoadOptionSupportList[Index].ListDevices (SupportedDeviceList);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
BootDeviceListSupportedFree (
  IN LIST_ENTRY *SupportedDeviceList,
  IN BDS_SUPPORTED_DEVICE *Except
  )
{
  LIST_ENTRY  *Entry;
  BDS_SUPPORTED_DEVICE* SupportedDevice;

  Entry = GetFirstNode (SupportedDeviceList);
  while (Entry != SupportedDeviceList) {
    SupportedDevice = SUPPORTED_BOOT_DEVICE_FROM_LINK(Entry);
    Entry = RemoveEntryList (Entry);
    if (SupportedDevice != Except) {
      FreePool (SupportedDevice);
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
BootDeviceGetDeviceSupport (
  IN  EFI_DEVICE_PATH           *DevicePath,
  OUT BDS_LOAD_OPTION_SUPPORT   **DeviceSupport
  )
{
  UINTN Index;

  // Find which supported device is the most appropriate
  for (Index = 0; Index < BDS_DEVICE_MAX; Index++) {
    if (BdsLoadOptionSupportList[Index].IsSupported (DevicePath)) {
      *DeviceSupport = &BdsLoadOptionSupportList[Index];
      return EFI_SUCCESS;
    }
  }

  return EFI_UNSUPPORTED;
}

EFI_STATUS
BootDeviceGetType (
  IN  EFI_DEVICE_PATH* DevicePath,
  OUT ARM_BDS_LOADER_TYPE *BootType,
  OUT UINT32 *Attributes
  )
{
  EFI_STATUS              Status;
  BOOLEAN                 IsEfiApp;
  BOOLEAN                 IsBootLoader;
  BOOLEAN                 HasFDTSupport;
  CHAR16*                 FileName;
  EFI_DEVICE_PATH*        PrevDevicePathNode;
  EFI_DEVICE_PATH*        DevicePathNode;
  EFI_PHYSICAL_ADDRESS    Image;
  UINTN                   FileSize;
  EFI_IMAGE_DOS_HEADER*   DosHeader;
  UINTN                   PeCoffHeaderOffset;
  EFI_IMAGE_NT_HEADERS32* NtHeader;

  //
  // Check if the last node of the device path is a FilePath node
  //
  PrevDevicePathNode = NULL;
  DevicePathNode = DevicePath;
  while ((DevicePathNode != NULL) && !IsDevicePathEnd (DevicePathNode)) {
    PrevDevicePathNode = DevicePathNode;
    DevicePathNode = NextDevicePathNode (DevicePathNode);
  }

  if ((PrevDevicePathNode != NULL) &&
      (PrevDevicePathNode->Type == MEDIA_DEVICE_PATH) &&
      (PrevDevicePathNode->SubType == MEDIA_FILEPATH_DP))
  {
    FileName = ((FILEPATH_DEVICE_PATH*)PrevDevicePathNode)->PathName;
  } else {
    FileName = NULL;
  }

  if (FileName == NULL) {
    Print(L"Is an EFI Application? ");
    Status = GetHIInputBoolean (&IsEfiApp);
    if (EFI_ERROR(Status)) {
      return EFI_ABORTED;
    }
  } else if (HasFilePathEfiExtension(FileName)) {
    IsEfiApp = TRUE;
  } else {
    // Check if the file exist
    Status = BdsLoadImage (DevicePath, AllocateAnyPages, &Image, &FileSize);
    if (!EFI_ERROR (Status)) {

      DosHeader = (EFI_IMAGE_DOS_HEADER *)(UINTN) Image;
      if (DosHeader->e_magic == EFI_IMAGE_DOS_SIGNATURE) {
        //
        // DOS image header is present,
        // so read the PE header after the DOS image header.
        //
        PeCoffHeaderOffset = DosHeader->e_lfanew;
      } else {
        PeCoffHeaderOffset = 0;
      }

      //
      // Check PE/COFF image.
      //
      NtHeader = (EFI_IMAGE_NT_HEADERS32 *)(UINTN) (Image + PeCoffHeaderOffset);
      if (NtHeader->Signature != EFI_IMAGE_NT_SIGNATURE) {
        IsEfiApp = FALSE;
      } else {
        IsEfiApp = TRUE;
      }

      // Free memory
      gBS->FreePages (Image, EFI_SIZE_TO_PAGES(FileSize));
    } else {
      // If we did not manage to open it then ask for the type
      Print(L"Is an EFI Application? ");
      Status = GetHIInputBoolean (&IsEfiApp);
      if (EFI_ERROR(Status)) {
        return EFI_ABORTED;
      }
    }
  }

  if (IsEfiApp) {
    Print(L"Is your application an OS loader? ");
    Status = GetHIInputBoolean (&IsBootLoader);
    if (EFI_ERROR(Status)) {
      return EFI_ABORTED;
    }
    if (!IsBootLoader) {
      *Attributes |= LOAD_OPTION_CATEGORY_APP;
    }
    *BootType = BDS_LOADER_EFI_APPLICATION;
  } else {
    Print(L"Has FDT support? ");
    Status = GetHIInputBoolean (&HasFDTSupport);
    if (EFI_ERROR(Status)) {
      return EFI_ABORTED;
    }
    if (HasFDTSupport) {
      *BootType = BDS_LOADER_KERNEL_LINUX_FDT;
    } else {
      *BootType = BDS_LOADER_KERNEL_LINUX_ATAG;
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
BdsLoadOptionFileSystemList (
  IN OUT LIST_ENTRY* BdsLoadOptionList
  )
{
  EFI_STATUS                        Status;
  UINTN                             HandleCount;
  EFI_HANDLE                        *HandleBuffer;
  UINTN                             Index;
  BDS_SUPPORTED_DEVICE              *SupportedDevice;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL*  FileProtocol;
  EFI_FILE_HANDLE                   Fs;
  UINTN                             Size;
  EFI_FILE_SYSTEM_INFO*             FsInfo;
  EFI_DEVICE_PATH_PROTOCOL*         DevicePathProtocol;

  // List all the Simple File System Protocols
  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &HandleCount, &HandleBuffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiDevicePathProtocolGuid, (VOID **)&DevicePathProtocol);
    if (!EFI_ERROR(Status)) {
      // Allocate BDS Supported Device structure
      SupportedDevice = (BDS_SUPPORTED_DEVICE*)AllocatePool (sizeof(BDS_SUPPORTED_DEVICE));

      FileProtocol = NULL;
      Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiSimpleFileSystemProtocolGuid, (VOID **)&FileProtocol);
      ASSERT_EFI_ERROR(Status);

      FileProtocol->OpenVolume (FileProtocol, &Fs);

      // Generate a Description from the file system
      Size = 0;
      FsInfo = NULL;
      Status = Fs->GetInfo (Fs, &gEfiFileSystemInfoGuid, &Size, FsInfo);
      if (Status == EFI_BUFFER_TOO_SMALL) {
        FsInfo = AllocatePool (Size);
        Status = Fs->GetInfo (Fs, &gEfiFileSystemInfoGuid, &Size, FsInfo);
      }
      UnicodeSPrint (SupportedDevice->Description,BOOT_DEVICE_DESCRIPTION_MAX,L"%s (%d MB)",FsInfo->VolumeLabel,(UINT32)(FsInfo->VolumeSize / (1024 * 1024)));
      FreePool(FsInfo);
      Fs->Close (Fs);

      SupportedDevice->DevicePathProtocol = DevicePathProtocol;
      SupportedDevice->Support = &BdsLoadOptionSupportList[BDS_DEVICE_FILESYSTEM];

      InsertTailList (BdsLoadOptionList,&SupportedDevice->Link);
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
BdsLoadOptionFileSystemCreateDevicePath (
  IN CHAR16*                    FileName,
  OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePathNodes
  )
{
  EFI_STATUS  Status;
  FILEPATH_DEVICE_PATH* FilePathDevicePath;
  CHAR16      BootFilePath[BOOT_DEVICE_FILEPATH_MAX];
  UINTN       BootFilePathSize;

  Print(L"File path of the %s: ", FileName);
  Status = GetHIInputStr (BootFilePath, BOOT_DEVICE_FILEPATH_MAX);
  if (EFI_ERROR(Status)) {
    return EFI_ABORTED;
  }

  BootFilePathSize = StrSize (BootFilePath);
  if (BootFilePathSize == 2) {
    *DevicePathNodes = NULL;
    return EFI_NOT_FOUND;
  }

  // Create the FilePath Device Path node
  FilePathDevicePath = (FILEPATH_DEVICE_PATH*)AllocatePool(SIZE_OF_FILEPATH_DEVICE_PATH + BootFilePathSize + END_DEVICE_PATH_LENGTH);
  FilePathDevicePath->Header.Type = MEDIA_DEVICE_PATH;
  FilePathDevicePath->Header.SubType = MEDIA_FILEPATH_DP;
  SetDevicePathNodeLength (FilePathDevicePath, SIZE_OF_FILEPATH_DEVICE_PATH + BootFilePathSize);
  CopyMem (FilePathDevicePath->PathName, BootFilePath, BootFilePathSize);
  SetDevicePathEndNode ((VOID*)((UINTN)FilePathDevicePath + SIZE_OF_FILEPATH_DEVICE_PATH + BootFilePathSize));
  *DevicePathNodes = (EFI_DEVICE_PATH_PROTOCOL*)FilePathDevicePath;

  return Status;
}

EFI_STATUS
BdsLoadOptionFileSystemUpdateDevicePath (
  IN EFI_DEVICE_PATH            *OldDevicePath,
  IN CHAR16*                    FileName,
  OUT EFI_DEVICE_PATH_PROTOCOL  **NewDevicePath
  )
{
  EFI_STATUS  Status;
  CHAR16      BootFilePath[BOOT_DEVICE_FILEPATH_MAX];
  UINTN       BootFilePathSize;
  FILEPATH_DEVICE_PATH* EndingDevicePath;
  FILEPATH_DEVICE_PATH* FilePathDevicePath;
  EFI_DEVICE_PATH*  DevicePath;

  DevicePath = DuplicateDevicePath (OldDevicePath);

  EndingDevicePath = (FILEPATH_DEVICE_PATH*)GetLastDevicePathNode (DevicePath);

  Print(L"File path of the %s: ", FileName);
  StrnCpy (BootFilePath, EndingDevicePath->PathName, BOOT_DEVICE_FILEPATH_MAX);
  Status = EditHIInputStr (BootFilePath, BOOT_DEVICE_FILEPATH_MAX);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  BootFilePathSize = StrSize(BootFilePath);
  if (BootFilePathSize == 2) {
    *NewDevicePath = NULL;
    return EFI_NOT_FOUND;
  }

  // Create the FilePath Device Path node
  FilePathDevicePath = (FILEPATH_DEVICE_PATH*)AllocatePool(SIZE_OF_FILEPATH_DEVICE_PATH + BootFilePathSize);
  FilePathDevicePath->Header.Type = MEDIA_DEVICE_PATH;
  FilePathDevicePath->Header.SubType = MEDIA_FILEPATH_DP;
  SetDevicePathNodeLength (FilePathDevicePath, SIZE_OF_FILEPATH_DEVICE_PATH + BootFilePathSize);
  CopyMem (FilePathDevicePath->PathName, BootFilePath, BootFilePathSize);

  // Generate the new Device Path by replacing the last node by the updated node
  SetDevicePathEndNode (EndingDevicePath);
  *NewDevicePath = AppendDevicePathNode (DevicePath, (CONST EFI_DEVICE_PATH_PROTOCOL *)FilePathDevicePath);
  FreePool(DevicePath);

  return EFI_SUCCESS;
}

/**
  Check if a boot option path is a file system boot option path or not.

  The device specified by the beginning of the path has to support the Simple File
  System protocol. Furthermore, the remaining part of the path has to be composed of
  a single node of type MEDIA_DEVICE_PATH and sub-type MEDIA_FILEPATH_DP.

  @param[in]  DevicePath  Complete device path of a boot option.

  @retval  FALSE  The boot option path has not been identified as that of a
                  file system boot option.
  @retval  TRUE   The boot option path is a file system boot option.
**/
BOOLEAN
BdsLoadOptionFileSystemIsSupported (
  IN  EFI_DEVICE_PATH  *DevicePath
  )
{
  EFI_STATUS                        Status;
  EFI_HANDLE                        Handle;
  EFI_DEVICE_PATH                  *RemainingDevicePath;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileProtocol;

  Status = BdsConnectDevicePath (DevicePath, &Handle, &RemainingDevicePath);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  Status = gBS->HandleProtocol (
                   Handle,
                   &gEfiSimpleFileSystemProtocolGuid,
                   (VOID **)(&FileProtocol)
                   );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  if (!IS_DEVICE_PATH_NODE (RemainingDevicePath, MEDIA_DEVICE_PATH, MEDIA_FILEPATH_DP))
    return FALSE;

  return TRUE;
}

STATIC
BOOLEAN
IsParentDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL *ParentDevicePath,
  IN EFI_DEVICE_PATH_PROTOCOL *ChildDevicePath
  )
{
  UINTN ParentSize;
  UINTN ChildSize;

  ParentSize = GetDevicePathSize (ParentDevicePath);
  ChildSize = GetDevicePathSize (ChildDevicePath);

  if (ParentSize > ChildSize) {
    return FALSE;
  }

  if (CompareMem (ParentDevicePath, ChildDevicePath, ParentSize - END_DEVICE_PATH_LENGTH) != 0) {
    return FALSE;
  }

  return TRUE;
}

EFI_STATUS
BdsLoadOptionMemMapList (
  IN OUT LIST_ENTRY* BdsLoadOptionList
  )
{
  EFI_STATUS                          Status;
  UINTN                               HandleCount;
  EFI_HANDLE                         *HandleBuffer;
  UINTN                               DevicePathHandleCount;
  EFI_HANDLE                         *DevicePathHandleBuffer;
  BOOLEAN                             IsParent;
  UINTN                               Index;
  UINTN                               Index2;
  BDS_SUPPORTED_DEVICE               *SupportedDevice;
  EFI_DEVICE_PATH_PROTOCOL*           DevicePathProtocol;
  EFI_DEVICE_PATH*                    DevicePath;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL    *FileProtocol;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL *FvbProtocol;

  // List all the BlockIo Protocols
  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiBlockIoProtocolGuid, NULL, &HandleCount, &HandleBuffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    // We only select handles WITH a Device Path AND not part of Media (to
    // avoid duplication with HardDisk, CDROM, etc). Skip handles used by
    // Simple Filesystem or used for Variable Storage.


    Status = gBS->HandleProtocol (HandleBuffer[Index],
                                  &gEfiSimpleFileSystemProtocolGuid,
                                  (VOID *)&FileProtocol);
    if (!EFI_ERROR(Status)) {
      // SimpleFilesystem supported on this handle, skip
      continue;
    }

    Status = gBS->HandleProtocol (HandleBuffer[Index],
                                  &gEfiFirmwareVolumeBlockProtocolGuid,
                                  (VOID *)&FvbProtocol);
    if (!EFI_ERROR(Status)) {
      // Firmware Volme Block / Variable storage supported on this handle, skip
      continue;
    }

    Status = gBS->HandleProtocol (HandleBuffer[Index],
                                  &gEfiFirmwareVolumeBlock2ProtocolGuid,
                                  (VOID *)&FvbProtocol);
    if (!EFI_ERROR(Status)) {
      // Firmware Volme Block / Variable storage supported on this handle, skip
      continue;
    }

    Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiDevicePathProtocolGuid, (VOID **)&DevicePathProtocol);
    if (!EFI_ERROR(Status)) {
      // BlockIo is not part of Media Device Path
      DevicePath = DevicePathProtocol;
      while (!IsDevicePathEndType (DevicePath) && (DevicePathType (DevicePath) != MEDIA_DEVICE_PATH)) {
        DevicePath = NextDevicePathNode (DevicePath);
      }
      if (DevicePathType (DevicePath) == MEDIA_DEVICE_PATH) {
        continue;
      }

      // Open all the handle supporting the DevicePath protocol and verify this handle has not got any child
      Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiDevicePathProtocolGuid, NULL, &DevicePathHandleCount, &DevicePathHandleBuffer);
      ASSERT_EFI_ERROR (Status);
      IsParent = FALSE;
      for (Index2 = 0; (Index2 < DevicePathHandleCount) && !IsParent; Index2++) {
        if (HandleBuffer[Index] != DevicePathHandleBuffer[Index2]) {
          gBS->HandleProtocol (DevicePathHandleBuffer[Index2], &gEfiDevicePathProtocolGuid, (VOID **)&DevicePath);
          if (IsParentDevicePath (DevicePathProtocol, DevicePath)) {
            IsParent = TRUE;
          }
        }
      }
      if (IsParent) {
        continue;
      }

      // Allocate BDS Supported Device structure
      SupportedDevice = (BDS_SUPPORTED_DEVICE*)AllocatePool(sizeof(BDS_SUPPORTED_DEVICE));

      Status = GenerateDeviceDescriptionName (HandleBuffer[Index], SupportedDevice->Description);
      ASSERT_EFI_ERROR (Status);

      SupportedDevice->DevicePathProtocol = DevicePathProtocol;
      SupportedDevice->Support = &BdsLoadOptionSupportList[BDS_DEVICE_MEMMAP];

      InsertTailList (BdsLoadOptionList,&SupportedDevice->Link);
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
BdsLoadOptionMemMapCreateDevicePath (
  IN CHAR16*                    FileName,
  OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePathNodes
  )
{
  EFI_STATUS              Status;
  MEMMAP_DEVICE_PATH      *MemMapDevicePath;
  CHAR16                  StrStartingAddress[BOOT_DEVICE_ADDRESS_MAX];
  CHAR16                  StrEndingAddress[BOOT_DEVICE_ADDRESS_MAX];

  Print(L"Starting Address of the %s: ", FileName);
  Status = GetHIInputStr (StrStartingAddress, BOOT_DEVICE_ADDRESS_MAX);
  if (EFI_ERROR(Status)) {
    return EFI_ABORTED;
  }

  Print(L"Ending Address of the %s: ", FileName);
  Status = GetHIInputStr (StrEndingAddress, BOOT_DEVICE_ADDRESS_MAX);
  if (EFI_ERROR(Status)) {
    return EFI_ABORTED;
  }

  // Create the MemMap Device Path Node
  MemMapDevicePath = (MEMMAP_DEVICE_PATH*)AllocatePool (sizeof(MEMMAP_DEVICE_PATH) + END_DEVICE_PATH_LENGTH);
  MemMapDevicePath->Header.Type = HARDWARE_DEVICE_PATH;
  MemMapDevicePath->Header.SubType = HW_MEMMAP_DP;
  SetDevicePathNodeLength (MemMapDevicePath, sizeof(MEMMAP_DEVICE_PATH));
  MemMapDevicePath->MemoryType = EfiBootServicesData;
  MemMapDevicePath->StartingAddress = StrHexToUint64 (StrStartingAddress);
  MemMapDevicePath->EndingAddress = StrHexToUint64 (StrEndingAddress);

  // Set a Device Path End Node after the Memory Map Device Path Node
  SetDevicePathEndNode (MemMapDevicePath + 1);
  *DevicePathNodes = (EFI_DEVICE_PATH_PROTOCOL*)MemMapDevicePath;

  return Status;
}

EFI_STATUS
BdsLoadOptionMemMapUpdateDevicePath (
  IN EFI_DEVICE_PATH            *OldDevicePath,
  IN CHAR16*                    FileName,
  OUT EFI_DEVICE_PATH_PROTOCOL  **NewDevicePath
  )
{
  EFI_STATUS          Status;
  CHAR16              StrStartingAddress[BOOT_DEVICE_ADDRESS_MAX];
  CHAR16              StrEndingAddress[BOOT_DEVICE_ADDRESS_MAX];
  MEMMAP_DEVICE_PATH* EndingDevicePath;
  EFI_DEVICE_PATH*    DevicePath;

  DevicePath = DuplicateDevicePath (OldDevicePath);
  EndingDevicePath = (MEMMAP_DEVICE_PATH*)GetLastDevicePathNode (DevicePath);

  Print(L"Starting Address of the %s: ", FileName);
  UnicodeSPrint (StrStartingAddress, BOOT_DEVICE_ADDRESS_MAX, L"0x%X", (UINTN)EndingDevicePath->StartingAddress);
  Status = EditHIInputStr (StrStartingAddress, BOOT_DEVICE_ADDRESS_MAX);
  if (EFI_ERROR(Status)) {
    return EFI_ABORTED;
  }

  Print(L"Ending Address of the %s: ", FileName);
  UnicodeSPrint (StrEndingAddress, BOOT_DEVICE_ADDRESS_MAX, L"0x%X", (UINTN)EndingDevicePath->EndingAddress);
  Status = EditHIInputStr (StrEndingAddress, BOOT_DEVICE_ADDRESS_MAX);
  if (EFI_ERROR(Status)) {
    return EFI_ABORTED;
  }

  EndingDevicePath->StartingAddress = StrHexToUint64 (StrStartingAddress);
  EndingDevicePath->EndingAddress = StrHexToUint64 (StrEndingAddress);

  if (EFI_ERROR(Status)) {
    FreePool(DevicePath);
  } else {
    *NewDevicePath = DevicePath;
  }

  return Status;
}

/**
  Check if a boot option path is a memory map boot option path or not.

  The device specified by the beginning of the path has to support the BlockIo
  protocol. Furthermore, the remaining part of the path has to be composed of
  a single node of type HARDWARE_DEVICE_PATH and sub-type HW_MEMMAP_DP.

  @param[in]  DevicePath  Complete device path of a boot option.

  @retval  FALSE  The boot option path has not been identified as that of a
                  memory map boot option.
  @retval  TRUE   The boot option path is a a memory map boot option.
**/
BOOLEAN
BdsLoadOptionMemMapIsSupported (
  IN  EFI_DEVICE_PATH  *DevicePath
  )
{
  EFI_STATUS              Status;
  EFI_HANDLE              Handle;
  EFI_DEVICE_PATH        *RemainingDevicePath;
  EFI_BLOCK_IO_PROTOCOL  *BlockIoProtocol;

  Status = BdsConnectDevicePath (DevicePath, &Handle, &RemainingDevicePath);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiBlockIoProtocolGuid,
                  (VOID **)(&BlockIoProtocol)
                  );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  if (!IS_DEVICE_PATH_NODE (RemainingDevicePath, HARDWARE_DEVICE_PATH, HW_MEMMAP_DP))
    return FALSE;

  return TRUE;
}

EFI_STATUS
BdsLoadOptionPxeList (
  IN OUT LIST_ENTRY* BdsLoadOptionList
  )
{
  EFI_STATUS                        Status;
  UINTN                             HandleCount;
  EFI_HANDLE                        *HandleBuffer;
  UINTN                             Index;
  BDS_SUPPORTED_DEVICE              *SupportedDevice;
  EFI_DEVICE_PATH_PROTOCOL*         DevicePathProtocol;
  EFI_SIMPLE_NETWORK_PROTOCOL*      SimpleNet;
  CHAR16                            DeviceDescription[BOOT_DEVICE_DESCRIPTION_MAX];
  EFI_MAC_ADDRESS                   *Mac;

  // List all the PXE Protocols
  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiPxeBaseCodeProtocolGuid, NULL, &HandleCount, &HandleBuffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    // We only select the handle WITH a Device Path AND the PXE Protocol
    Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiDevicePathProtocolGuid, (VOID **)&DevicePathProtocol);
    if (!EFI_ERROR(Status)) {
      // Allocate BDS Supported Device structure
      SupportedDevice = (BDS_SUPPORTED_DEVICE*)AllocatePool(sizeof(BDS_SUPPORTED_DEVICE));

      Status = gBS->LocateProtocol (&gEfiSimpleNetworkProtocolGuid, NULL, (VOID **)&SimpleNet);
      if (!EFI_ERROR(Status)) {
        Mac = &SimpleNet->Mode->CurrentAddress;
        UnicodeSPrint (DeviceDescription,BOOT_DEVICE_DESCRIPTION_MAX,L"MAC Address: %02x:%02x:%02x:%02x:%02x:%02x", Mac->Addr[0],  Mac->Addr[1],  Mac->Addr[2],  Mac->Addr[3],  Mac->Addr[4],  Mac->Addr[5]);
      } else {
        Status = GenerateDeviceDescriptionName (HandleBuffer[Index], DeviceDescription);
        ASSERT_EFI_ERROR (Status);
      }
      UnicodeSPrint (SupportedDevice->Description,BOOT_DEVICE_DESCRIPTION_MAX,L"PXE on %s",DeviceDescription);

      SupportedDevice->DevicePathProtocol = DevicePathProtocol;
      SupportedDevice->Support = &BdsLoadOptionSupportList[BDS_DEVICE_PXE];

      InsertTailList (BdsLoadOptionList,&SupportedDevice->Link);
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
BdsLoadOptionPxeCreateDevicePath (
  IN CHAR16*                    FileName,
  OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePathNodes
  )
{
  *DevicePathNodes = (EFI_DEVICE_PATH_PROTOCOL *) AllocatePool (END_DEVICE_PATH_LENGTH);
  SetDevicePathEndNode (*DevicePathNodes);

  return EFI_SUCCESS;
}

/**
  Update the parameters of a Pxe boot option

  @param[in]   OldDevicePath  Current complete device path of the Pxe boot option.
                              This has to be a valid complete Pxe boot option path.
  @param[in]   FileName       Description of the file the path is asked for
  @param[out]  NewDevicePath  Pointer to the new complete device path.

  @retval  EFI_SUCCESS            Update completed
  @retval  EFI_OUT_OF_RESOURCES   Fail to perform the update due to lack of resource
**/
EFI_STATUS
BdsLoadOptionPxeUpdateDevicePath (
  IN EFI_DEVICE_PATH            *OldDevicePath,
  IN CHAR16*                    FileName,
  OUT EFI_DEVICE_PATH_PROTOCOL  **NewDevicePath
  )
{
  //
  // Make a copy of the complete device path that is made of :
  // the device path of the device supporting the Pxe base code protocol
  // followed by an end node.
  //
  *NewDevicePath = DuplicateDevicePath (OldDevicePath);
  if (*NewDevicePath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  } else {
    return EFI_SUCCESS;
  }
}

BOOLEAN
BdsLoadOptionPxeIsSupported (
  IN  EFI_DEVICE_PATH  *DevicePath
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE Handle;
  EFI_DEVICE_PATH_PROTOCOL  *RemainingDevicePath;
  EFI_PXE_BASE_CODE_PROTOCOL  *PxeBcProtocol;

  Status = BdsConnectDevicePath (DevicePath, &Handle, &RemainingDevicePath);
  if (EFI_ERROR(Status)) {
    return FALSE;
  }

  if (!IsDevicePathEnd(RemainingDevicePath)) {
    return FALSE;
  }

  Status = gBS->HandleProtocol (Handle, &gEfiPxeBaseCodeProtocolGuid, (VOID **)&PxeBcProtocol);
  if (EFI_ERROR (Status)) {
    return FALSE;
  } else {
    return TRUE;
  }
}

/**
  Add to the list of boot devices the devices allowing a TFTP boot

  @param[in]   BdsLoadOptionList  List of devices to boot from

  @retval  EFI_SUCCESS            Update completed
  @retval  EFI_OUT_OF_RESOURCES   Fail to perform the update due to lack of resource
**/
EFI_STATUS
BdsLoadOptionTftpList (
  IN OUT LIST_ENTRY* BdsLoadOptionList
  )
{
  EFI_STATUS                   Status;
  UINTN                        HandleCount;
  EFI_HANDLE                   *HandleBuffer;
  EFI_HANDLE                   Handle;
  UINTN                        Index;
  EFI_DEVICE_PATH_PROTOCOL     *DevicePathProtocol;
  VOID                         *Interface;
  EFI_SIMPLE_NETWORK_PROTOCOL  *SimpleNetworkProtocol;
  BDS_SUPPORTED_DEVICE         *SupportedDevice;
  EFI_MAC_ADDRESS              *Mac;

  //
  // List all the handles on which the Simple Network Protocol is installed.
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSimpleNetworkProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    Handle = HandleBuffer[Index];
    //
    // We select the handles that support :
    // . the Device Path Protocol
    // . the MTFTP4 Protocol
    //
    Status = gBS->HandleProtocol (
                    Handle,
                    &gEfiDevicePathProtocolGuid,
                    (VOID **)&DevicePathProtocol
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    Status = gBS->HandleProtocol (
                    Handle,
                    &gEfiMtftp4ServiceBindingProtocolGuid,
                    &Interface
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    Status = gBS->HandleProtocol (
                    Handle,
                    &gEfiSimpleNetworkProtocolGuid,
                    (VOID **)&SimpleNetworkProtocol
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    // Allocate BDS Supported Device structure
    SupportedDevice = (BDS_SUPPORTED_DEVICE*)AllocatePool (sizeof (BDS_SUPPORTED_DEVICE));
    if (SupportedDevice == NULL) {
      continue;
    }

    Mac = &SimpleNetworkProtocol->Mode->CurrentAddress;
    UnicodeSPrint (
      SupportedDevice->Description,
      BOOT_DEVICE_DESCRIPTION_MAX,
      L"TFTP on MAC Address: %02x:%02x:%02x:%02x:%02x:%02x",
      Mac->Addr[0],  Mac->Addr[1],  Mac->Addr[2],  Mac->Addr[3],  Mac->Addr[4],  Mac->Addr[5]
      );

    SupportedDevice->DevicePathProtocol = DevicePathProtocol;
    SupportedDevice->Support = &BdsLoadOptionSupportList[BDS_DEVICE_TFTP];

    InsertTailList (BdsLoadOptionList, &SupportedDevice->Link);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
BdsLoadOptionTftpCreateDevicePath (
  IN CHAR16*                    FileName,
  OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePathNodes
  )
{
  EFI_STATUS            Status;
  BOOLEAN               IsDHCP;
  EFI_IP_ADDRESS        LocalIp;
  EFI_IP_ADDRESS        SubnetMask;
  EFI_IP_ADDRESS        GatewayIp;
  EFI_IP_ADDRESS        RemoteIp;
  IPv4_DEVICE_PATH      *IPv4DevicePathNode;
  FILEPATH_DEVICE_PATH  *FilePathDevicePath;
  CHAR16                BootFilePath[BOOT_DEVICE_FILEPATH_MAX];
  UINTN                 BootFilePathSize;

  Print (L"Get the IP address from DHCP: ");
  Status = GetHIInputBoolean (&IsDHCP);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  if (!IsDHCP) {
    Print (L"Local static IP address: ");
    Status = GetHIInputIP (&LocalIp);
    if (EFI_ERROR (Status)) {
      return EFI_ABORTED;
    }
    Print (L"Get the network mask: ");
    Status = GetHIInputIP (&SubnetMask);
    if (EFI_ERROR (Status)) {
      return EFI_ABORTED;
    }
    Print (L"Get the gateway IP address: ");
    Status = GetHIInputIP (&GatewayIp);
    if (EFI_ERROR (Status)) {
      return EFI_ABORTED;
    }
  }

  Print (L"Get the TFTP server IP address: ");
  Status = GetHIInputIP (&RemoteIp);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  Print (L"File path of the %s : ", FileName);
  Status = GetHIInputStr (BootFilePath, BOOT_DEVICE_FILEPATH_MAX);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  BootFilePathSize = StrSize(BootFilePath);
  if (BootFilePathSize == 2) {
    return EFI_NOT_FOUND;
  }

  // Allocate the memory for the IPv4 + File Path Device Path Nodes
  IPv4DevicePathNode = (IPv4_DEVICE_PATH*)AllocatePool(sizeof(IPv4_DEVICE_PATH) + SIZE_OF_FILEPATH_DEVICE_PATH + BootFilePathSize + END_DEVICE_PATH_LENGTH);

  // Create the IPv4 Device Path
  IPv4DevicePathNode->Header.Type    = MESSAGING_DEVICE_PATH;
  IPv4DevicePathNode->Header.SubType = MSG_IPv4_DP;
  SetDevicePathNodeLength (&IPv4DevicePathNode->Header, sizeof(IPv4_DEVICE_PATH));

  if (!IsDHCP) {
    CopyMem (&IPv4DevicePathNode->LocalIpAddress, &LocalIp.v4, sizeof (EFI_IPv4_ADDRESS));
    CopyMem (&IPv4DevicePathNode->SubnetMask, &SubnetMask.v4, sizeof (EFI_IPv4_ADDRESS));
    CopyMem (&IPv4DevicePathNode->GatewayIpAddress, &GatewayIp.v4, sizeof (EFI_IPv4_ADDRESS));
  }

  CopyMem (&IPv4DevicePathNode->RemoteIpAddress, &RemoteIp.v4, sizeof (EFI_IPv4_ADDRESS));
  IPv4DevicePathNode->LocalPort  = 0;
  IPv4DevicePathNode->RemotePort = 0;
  IPv4DevicePathNode->Protocol = EFI_IP_PROTO_TCP;
  IPv4DevicePathNode->StaticIpAddress = (IsDHCP != TRUE);

  // Create the FilePath Device Path node
  FilePathDevicePath = (FILEPATH_DEVICE_PATH*)(IPv4DevicePathNode + 1);
  FilePathDevicePath->Header.Type = MEDIA_DEVICE_PATH;
  FilePathDevicePath->Header.SubType = MEDIA_FILEPATH_DP;
  SetDevicePathNodeLength (FilePathDevicePath, SIZE_OF_FILEPATH_DEVICE_PATH + BootFilePathSize);
  CopyMem (FilePathDevicePath->PathName, BootFilePath, BootFilePathSize);

  // Set the End Device Path Node
  SetDevicePathEndNode ((VOID*)((UINTN)FilePathDevicePath + SIZE_OF_FILEPATH_DEVICE_PATH + BootFilePathSize));
  *DevicePathNodes = (EFI_DEVICE_PATH_PROTOCOL*)IPv4DevicePathNode;

  return Status;
}

/**
  Update the parameters of a TFTP boot option

  The function asks sequentially to update the IPv4 parameters as well as the boot file path,
  providing the previously set value if any.

  @param[in]   OldDevicePath  Current complete device path of the Tftp boot option.
                              This has to be a valid complete Tftp boot option path.
                              By complete, we mean that it is not only the Tftp
                              specific end part built by the
                              "BdsLoadOptionTftpCreateDevicePath()" function.
                              This path is handled as read only.
  @param[in]   FileName       Description of the file the path is asked for
  @param[out]  NewDevicePath  Pointer to the new complete device path.

  @retval  EFI_SUCCESS            Update completed
  @retval  EFI_ABORTED            Update aborted by the user
  @retval  EFI_OUT_OF_RESOURCES   Fail to perform the update due to lack of resource
**/
EFI_STATUS
BdsLoadOptionTftpUpdateDevicePath (
  IN   EFI_DEVICE_PATH            *OldDevicePath,
  IN   CHAR16                     *FileName,
  OUT  EFI_DEVICE_PATH_PROTOCOL  **NewDevicePath
  )
{
  EFI_STATUS             Status;
  EFI_DEVICE_PATH       *DevicePath;
  EFI_DEVICE_PATH       *DevicePathNode;
  UINT8                 *Ipv4NodePtr;
  IPv4_DEVICE_PATH       Ipv4Node;
  BOOLEAN                IsDHCP;
  EFI_IP_ADDRESS         OldIp;
  EFI_IP_ADDRESS         OldSubnetMask;
  EFI_IP_ADDRESS         OldGatewayIp;
  EFI_IP_ADDRESS         LocalIp;
  EFI_IP_ADDRESS         SubnetMask;
  EFI_IP_ADDRESS         GatewayIp;
  EFI_IP_ADDRESS         RemoteIp;
  UINT8                 *FileNodePtr;
  CHAR16                 BootFilePath[BOOT_DEVICE_FILEPATH_MAX];
  UINTN                  PathSize;
  UINTN                  BootFilePathSize;
  FILEPATH_DEVICE_PATH  *NewFilePathNode;

  Ipv4NodePtr = NULL;

  //
  // Make a copy of the complete device path that is made of :
  // the device path of the device that support the Simple Network protocol
  // followed by an IPv4 node (type IPv4_DEVICE_PATH),
  // followed by a file path node (type FILEPATH_DEVICE_PATH) and ended up
  // by an end node. The IPv6 case is not handled yet.
  //

  DevicePath = DuplicateDevicePath (OldDevicePath);
  if (DevicePath == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }

  //
  // Because of the check done by "BdsLoadOptionTftpIsSupported()" prior to the
  // call to this function, we know that the device path ends with an IPv4 node
  // followed by a file path node and finally an end node. To get the address of
  // the last IPv4 node, we loop over the whole device path, noting down the
  // address of each encountered IPv4 node.
  //

  for (DevicePathNode = DevicePath;
       !IsDevicePathEnd (DevicePathNode);
       DevicePathNode = NextDevicePathNode (DevicePathNode))
  {
    if (IS_DEVICE_PATH_NODE (DevicePathNode, MESSAGING_DEVICE_PATH, MSG_IPv4_DP)) {
      Ipv4NodePtr = (UINT8*)DevicePathNode;
    }
  }

  // Copy for alignment of the IPv4 node data
  CopyMem (&Ipv4Node, Ipv4NodePtr, sizeof (IPv4_DEVICE_PATH));

  Print (L"Get the IP address from DHCP: ");
  Status = GetHIInputBoolean (&IsDHCP);
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  if (!IsDHCP) {
    Print (L"Local static IP address: ");
    if (Ipv4Node.StaticIpAddress) {
      CopyMem (&OldIp.v4, &Ipv4Node.LocalIpAddress, sizeof (EFI_IPv4_ADDRESS));
      Status = EditHIInputIP (&OldIp, &LocalIp);
    } else {
      Status = GetHIInputIP (&LocalIp);
    }
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    Print (L"Get the network mask: ");
    if (Ipv4Node.StaticIpAddress) {
      CopyMem (&OldSubnetMask.v4, &Ipv4Node.SubnetMask, sizeof (EFI_IPv4_ADDRESS));
      Status = EditHIInputIP (&OldSubnetMask, &SubnetMask);
    } else {
      Status = GetHIInputIP (&SubnetMask);
    }
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    Print (L"Get the gateway IP address: ");
    if (Ipv4Node.StaticIpAddress) {
      CopyMem (&OldGatewayIp.v4, &Ipv4Node.GatewayIpAddress, sizeof (EFI_IPv4_ADDRESS));
      Status = EditHIInputIP (&OldGatewayIp, &GatewayIp);
    } else {
      Status = GetHIInputIP (&GatewayIp);
    }
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }
  }

  Print (L"TFTP server IP address: ");
  // Copy remote IPv4 address into IPv4 or IPv6 union
  CopyMem (&OldIp.v4, &Ipv4Node.RemoteIpAddress, sizeof (EFI_IPv4_ADDRESS));

  Status = EditHIInputIP (&OldIp, &RemoteIp);
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  // Get the path of the boot file and its size in number of bytes
  FileNodePtr = Ipv4NodePtr + sizeof (IPv4_DEVICE_PATH);
  BootFilePathSize = DevicePathNodeLength (FileNodePtr) - SIZE_OF_FILEPATH_DEVICE_PATH;

  //
  // Ask for update of the boot file path
  //
  do {
    // Copy for 2-byte alignment of the Unicode string
    CopyMem (
      BootFilePath, FileNodePtr + SIZE_OF_FILEPATH_DEVICE_PATH,
      MIN (BootFilePathSize, BOOT_DEVICE_FILEPATH_MAX)
      );
    BootFilePath[BOOT_DEVICE_FILEPATH_MAX - 1] = L'\0';

    Print (L"File path of the %s: ", FileName);
    Status = EditHIInputStr (BootFilePath, BOOT_DEVICE_FILEPATH_MAX);
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }
    PathSize = StrSize (BootFilePath);
    if (PathSize > 2) {
      break;
    }
    // Empty string, give the user another try
    Print (L"Empty string - Invalid path\n");
  } while (PathSize <= 2) ;

  //
  // Update the IPv4 node. IPv6 case not handled yet.
  //
  if (IsDHCP) {
    Ipv4Node.StaticIpAddress = FALSE;
    ZeroMem (&Ipv4Node.LocalIpAddress, sizeof (EFI_IPv4_ADDRESS));
    ZeroMem (&Ipv4Node.SubnetMask, sizeof (EFI_IPv4_ADDRESS));
    ZeroMem (&Ipv4Node.GatewayIpAddress, sizeof (EFI_IPv4_ADDRESS));
  } else {
    Ipv4Node.StaticIpAddress = TRUE;
    CopyMem (&Ipv4Node.LocalIpAddress, &LocalIp.v4, sizeof (EFI_IPv4_ADDRESS));
    CopyMem (&Ipv4Node.SubnetMask, &SubnetMask.v4, sizeof (EFI_IPv4_ADDRESS));
    CopyMem (&Ipv4Node.GatewayIpAddress, &GatewayIp.v4, sizeof (EFI_IPv4_ADDRESS));
  }

  CopyMem (&Ipv4Node.RemoteIpAddress, &RemoteIp.v4, sizeof (EFI_IPv4_ADDRESS));
  CopyMem (Ipv4NodePtr, &Ipv4Node, sizeof (IPv4_DEVICE_PATH));

  //
  // Create the new file path node
  //
  NewFilePathNode = (FILEPATH_DEVICE_PATH*)AllocatePool (
                                             SIZE_OF_FILEPATH_DEVICE_PATH +
                                             PathSize
                                             );
  NewFilePathNode->Header.Type    = MEDIA_DEVICE_PATH;
  NewFilePathNode->Header.SubType = MEDIA_FILEPATH_DP;
  SetDevicePathNodeLength (
    NewFilePathNode,
    SIZE_OF_FILEPATH_DEVICE_PATH + PathSize
    );
  CopyMem (NewFilePathNode->PathName, BootFilePath, PathSize);

  //
  // Generate the new Device Path by replacing the file path node at address
  // "FileNodePtr" by the new one "NewFilePathNode" and return its address.
  //
  SetDevicePathEndNode (FileNodePtr);
  *NewDevicePath = AppendDevicePathNode (
                     DevicePath,
                     (CONST EFI_DEVICE_PATH_PROTOCOL*)NewFilePathNode
                     );

ErrorExit:
  if (DevicePath != NULL) {
    FreePool (DevicePath) ;
  }

  return Status;
}

BOOLEAN
BdsLoadOptionTftpIsSupported (
  IN  EFI_DEVICE_PATH           *DevicePath
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE Handle;
  EFI_DEVICE_PATH  *RemainingDevicePath;
  EFI_DEVICE_PATH  *NextDevicePath;
  EFI_PXE_BASE_CODE_PROTOCOL  *PxeBcProtocol;

  Status = BdsConnectDevicePath (DevicePath, &Handle, &RemainingDevicePath);
  if (EFI_ERROR(Status)) {
    return FALSE;
  }

  // Validate the Remaining Device Path
  if (IsDevicePathEnd(RemainingDevicePath)) {
    return FALSE;
  }
  if (!IS_DEVICE_PATH_NODE(RemainingDevicePath,MESSAGING_DEVICE_PATH,MSG_IPv4_DP) &&
      !IS_DEVICE_PATH_NODE(RemainingDevicePath,MESSAGING_DEVICE_PATH,MSG_IPv6_DP)) {
    return FALSE;
  }
  NextDevicePath = NextDevicePathNode (RemainingDevicePath);
  if (IsDevicePathEnd(NextDevicePath)) {
    return FALSE;
  }
  if (!IS_DEVICE_PATH_NODE(NextDevicePath,MEDIA_DEVICE_PATH,MEDIA_FILEPATH_DP)) {
    return FALSE;
  }

  Status = gBS->HandleProtocol (Handle, &gEfiPxeBaseCodeProtocolGuid, (VOID **)&PxeBcProtocol);
  if (EFI_ERROR (Status)) {
    return FALSE;
  } else {
    return TRUE;
  }
}
