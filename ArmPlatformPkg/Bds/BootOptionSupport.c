/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
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
#include <Protocol/PxeBaseCode.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/SimpleNetwork.h>

#include <Guid/FileSystemInfo.h>

#define IS_DEVICE_PATH_NODE(node,type,subtype) (((node)->Type == (type)) && ((node)->SubType == (subtype)))

EFI_STATUS
BdsLoadOptionFileSystemList (
  IN OUT LIST_ENTRY* BdsLoadOptionList
  );

EFI_STATUS
BdsLoadOptionFileSystemCreateDevicePath (
  IN CHAR16*                    FileName,
  OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePathNodes,
  OUT ARM_BDS_LOADER_TYPE       *BootType,
  OUT UINT32                    *Attributes
  );

EFI_STATUS
BdsLoadOptionFileSystemUpdateDevicePath (
  IN EFI_DEVICE_PATH            *OldDevicePath,
  IN CHAR16*                    FileName,
  OUT EFI_DEVICE_PATH_PROTOCOL  **NewDevicePath,
  OUT ARM_BDS_LOADER_TYPE       *BootType,
  OUT UINT32                    *Attributes
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
  OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePathNodes,
  OUT ARM_BDS_LOADER_TYPE       *BootType,
  OUT UINT32                    *Attributes
  );

EFI_STATUS
BdsLoadOptionMemMapUpdateDevicePath (
  IN EFI_DEVICE_PATH            *OldDevicePath,
  IN CHAR16*                    FileName,
  OUT EFI_DEVICE_PATH_PROTOCOL  **NewDevicePath,
  OUT ARM_BDS_LOADER_TYPE       *BootType,
  OUT UINT32                    *Attributes
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
  OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePathNodes,
  OUT ARM_BDS_LOADER_TYPE       *BootType,
  OUT UINT32                    *Attributes
  );

EFI_STATUS
BdsLoadOptionPxeUpdateDevicePath (
  IN EFI_DEVICE_PATH            *OldDevicePath,
  IN CHAR16*                    FileName,
  OUT EFI_DEVICE_PATH_PROTOCOL  **NewDevicePath,
  OUT ARM_BDS_LOADER_TYPE       *BootType,
  OUT UINT32                    *Attributes
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
  OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePathNodes,
  OUT ARM_BDS_LOADER_TYPE       *BootType,
  OUT UINT32                    *Attributes
  );

EFI_STATUS
BdsLoadOptionTftpUpdateDevicePath (
  IN EFI_DEVICE_PATH            *OldDevicePath,
  IN CHAR16*                    FileName,
  OUT EFI_DEVICE_PATH_PROTOCOL  **NewDevicePath,
  OUT ARM_BDS_LOADER_TYPE       *BootType,
  OUT UINT32                    *Attributes
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
    BdsLoadOptionFileSystemUpdateDevicePath
  },
  {
    BDS_DEVICE_MEMMAP,
    BdsLoadOptionMemMapList,
    BdsLoadOptionMemMapIsSupported,
    BdsLoadOptionMemMapCreateDevicePath,
    BdsLoadOptionMemMapUpdateDevicePath
  },
  {
    BDS_DEVICE_PXE,
    BdsLoadOptionPxeList,
    BdsLoadOptionPxeIsSupported,
    BdsLoadOptionPxeCreateDevicePath,
    BdsLoadOptionPxeUpdateDevicePath
  },
  {
    BDS_DEVICE_TFTP,
    BdsLoadOptionTftpList,
    BdsLoadOptionTftpIsSupported,
    BdsLoadOptionTftpCreateDevicePath,
    BdsLoadOptionTftpUpdateDevicePath
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

STATIC
EFI_STATUS
BootDeviceGetType (
  IN  CHAR16* FileName,
  OUT ARM_BDS_LOADER_TYPE *BootType,
  OUT UINT32 *Attributes
  )
{
  EFI_STATUS Status;
  BOOLEAN IsEfiApp;
  BOOLEAN IsBootLoader;
  BOOLEAN     HasFDTSupport;

  if (FileName == NULL) {
    Print(L"Is an EFI Application? ");
    Status = GetHIInputBoolean (&IsEfiApp);
    if (EFI_ERROR(Status)) {
      return EFI_ABORTED;
    }
  } else if (HasFilePathEfiExtension(FileName)) {
    IsEfiApp = TRUE;
  } else {
    IsEfiApp = FALSE;
  }

  if (IsEfiApp) {
    Print(L"Is your application is an OS loader? ");
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
  OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePathNodes,
  OUT ARM_BDS_LOADER_TYPE       *BootType,
  OUT UINT32                    *Attributes
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

  if (BootType != NULL || Attributes != NULL) {
    Status = BootDeviceGetType (FilePathDevicePath->PathName, BootType, Attributes);
  }

  if (EFI_ERROR(Status)) {
    FreePool (FilePathDevicePath);
  } else {
    *DevicePathNodes = (EFI_DEVICE_PATH_PROTOCOL*)FilePathDevicePath;
  }

  return Status;
}

EFI_STATUS
BdsLoadOptionFileSystemUpdateDevicePath (
  IN EFI_DEVICE_PATH            *OldDevicePath,
  IN CHAR16*                    FileName,
  OUT EFI_DEVICE_PATH_PROTOCOL  **NewDevicePath,
  OUT ARM_BDS_LOADER_TYPE       *BootType,
  OUT UINT32                    *Attributes
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

  if (BootType != NULL || Attributes != NULL) {
    return BootDeviceGetType (FilePathDevicePath->PathName, BootType, Attributes);
  }

  return EFI_SUCCESS;
}

BOOLEAN
BdsLoadOptionFileSystemIsSupported (
  IN  EFI_DEVICE_PATH           *DevicePath
  )
{
  EFI_DEVICE_PATH*  DevicePathNode;

  DevicePathNode = GetLastDevicePathNode (DevicePath);

  return IS_DEVICE_PATH_NODE(DevicePathNode,MEDIA_DEVICE_PATH,MEDIA_FILEPATH_DP);
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
  EFI_STATUS                        Status;
  UINTN                             HandleCount;
  EFI_HANDLE                        *HandleBuffer;
  UINTN                             DevicePathHandleCount;
  EFI_HANDLE                        *DevicePathHandleBuffer;
  BOOLEAN                           IsParent;
  UINTN                             Index;
  UINTN                             Index2;
  BDS_SUPPORTED_DEVICE              *SupportedDevice;
  EFI_DEVICE_PATH_PROTOCOL*         DevicePathProtocol;
  EFI_DEVICE_PATH*                  DevicePath;

  // List all the BlockIo Protocols
  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiBlockIoProtocolGuid, NULL, &HandleCount, &HandleBuffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    // We only select the handle WITH a Device Path AND not part of Media (to avoid duplication with HardDisk, CDROM, etc)
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
  OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePathNodes,
  OUT ARM_BDS_LOADER_TYPE       *BootType,
  OUT UINT32                    *Attributes
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

  if (BootType != NULL || Attributes != NULL) {
    Status = BootDeviceGetType (NULL, BootType, Attributes);
  }

  if (EFI_ERROR(Status)) {
    FreePool (MemMapDevicePath);
  } else {
    *DevicePathNodes = (EFI_DEVICE_PATH_PROTOCOL*)MemMapDevicePath;
  }

  return Status;
}

EFI_STATUS
BdsLoadOptionMemMapUpdateDevicePath (
  IN EFI_DEVICE_PATH            *OldDevicePath,
  IN CHAR16*                    FileName,
  OUT EFI_DEVICE_PATH_PROTOCOL  **NewDevicePath,
  OUT ARM_BDS_LOADER_TYPE       *BootType,
  OUT UINT32                    *Attributes
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

  if (BootType != NULL || Attributes != NULL) {
    Status = BootDeviceGetType (NULL, BootType, Attributes);
  }

  if (EFI_ERROR(Status)) {
    FreePool(DevicePath);
  } else {
    *NewDevicePath = DevicePath;
  }

  return Status;
}

BOOLEAN
BdsLoadOptionMemMapIsSupported (
  IN  EFI_DEVICE_PATH           *DevicePath
  )
{
  EFI_DEVICE_PATH*  DevicePathNode;

  DevicePathNode = GetLastDevicePathNode (DevicePath);

  return IS_DEVICE_PATH_NODE(DevicePathNode,HARDWARE_DEVICE_PATH,HW_MEMMAP_DP);
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
  OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePathNodes,
  OUT ARM_BDS_LOADER_TYPE       *BootType,
  OUT UINT32                    *Attributes
  )
{
  *DevicePathNodes = (EFI_DEVICE_PATH_PROTOCOL *) AllocatePool (END_DEVICE_PATH_LENGTH);
  SetDevicePathEndNode (*DevicePathNodes);
  *BootType = BDS_LOADER_EFI_APPLICATION;
  return EFI_SUCCESS;
}

EFI_STATUS
BdsLoadOptionPxeUpdateDevicePath (
  IN EFI_DEVICE_PATH            *OldDevicePath,
  IN CHAR16*                    FileName,
  OUT EFI_DEVICE_PATH_PROTOCOL  **NewDevicePath,
  OUT ARM_BDS_LOADER_TYPE       *BootType,
  OUT UINT32                    *Attributes
  )
{
  ASSERT (0);
  return EFI_SUCCESS;
}

BOOLEAN
BdsLoadOptionPxeIsSupported (
  IN  EFI_DEVICE_PATH           *DevicePath
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

EFI_STATUS
BdsLoadOptionTftpList (
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
    // We only select the handle WITH a Device Path AND the PXE Protocol AND the TFTP Protocol (the TFTP protocol is required to start PXE)
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
      UnicodeSPrint (SupportedDevice->Description,BOOT_DEVICE_DESCRIPTION_MAX,L"TFTP on %s",DeviceDescription);

      SupportedDevice->DevicePathProtocol = DevicePathProtocol;
      SupportedDevice->Support = &BdsLoadOptionSupportList[BDS_DEVICE_TFTP];

      InsertTailList (BdsLoadOptionList,&SupportedDevice->Link);
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
BdsLoadOptionTftpCreateDevicePath (
  IN CHAR16*                    FileName,
  OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePathNodes,
  OUT ARM_BDS_LOADER_TYPE       *BootType,
  OUT UINT32                    *Attributes
  )
{
  EFI_STATUS    Status;
  BOOLEAN       IsDHCP;
  EFI_IP_ADDRESS  LocalIp;
  EFI_IP_ADDRESS  RemoteIp;
  IPv4_DEVICE_PATH*   IPv4DevicePathNode;
  FILEPATH_DEVICE_PATH* FilePathDevicePath;
  CHAR16      BootFilePath[BOOT_DEVICE_FILEPATH_MAX];
  UINTN       BootFilePathSize;

  Print(L"Get the IP address from DHCP: ");
  Status = GetHIInputBoolean (&IsDHCP);
  if (EFI_ERROR(Status)) {
    return EFI_ABORTED;
  }

  if (!IsDHCP) {
    Print(L"Get the static IP address: ");
    Status = GetHIInputIP (&LocalIp);
    if (EFI_ERROR(Status)) {
      return EFI_ABORTED;
    }
  }

  Print(L"Get the TFTP server IP address: ");
  Status = GetHIInputIP (&RemoteIp);
  if (EFI_ERROR(Status)) {
    return EFI_ABORTED;
  }

  Print(L"File path of the %s : ", FileName);
  Status = GetHIInputStr (BootFilePath, BOOT_DEVICE_FILEPATH_MAX);
  if (EFI_ERROR(Status)) {
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
  CopyMem (&IPv4DevicePathNode->LocalIpAddress, &LocalIp.v4, sizeof (EFI_IPv4_ADDRESS));
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

  if (BootType != NULL || Attributes != NULL) {
    Status = BootDeviceGetType (NULL, BootType, Attributes);
  }

  if (EFI_ERROR(Status)) {
    FreePool (IPv4DevicePathNode);
  } else {
    *DevicePathNodes = (EFI_DEVICE_PATH_PROTOCOL*)IPv4DevicePathNode;
  }

  return Status;
}

EFI_STATUS
BdsLoadOptionTftpUpdateDevicePath (
  IN EFI_DEVICE_PATH            *OldDevicePath,
  IN CHAR16*                    FileName,
  OUT EFI_DEVICE_PATH_PROTOCOL  **NewDevicePath,
  OUT ARM_BDS_LOADER_TYPE       *BootType,
  OUT UINT32                    *Attributes
  )
{
  ASSERT (0);
  return EFI_SUCCESS;
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
