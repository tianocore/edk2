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
  IN  BDS_SUPPORTED_DEVICE* BdsLoadOption,
  OUT EFI_DEVICE_PATH_PROTOCOL **DevicePathNode,
  OUT BDS_LOADER_TYPE   *BootType,
  OUT UINT32      *Attributes
  );

EFI_STATUS
BdsLoadOptionFileSystemUpdateDevicePath (
  IN EFI_DEVICE_PATH *OldDevicePath,
  OUT EFI_DEVICE_PATH_PROTOCOL** NewDevicePath,
  OUT BDS_LOADER_TYPE *BootType,
  OUT UINT32 *Attributes
  );

BOOLEAN
BdsLoadOptionFileSystemIsSupported (
  IN BDS_LOAD_OPTION* BdsLoadOption
  );

EFI_STATUS
BdsLoadOptionMemMapList (
  IN OUT LIST_ENTRY* BdsLoadOptionList
  );

EFI_STATUS
BdsLoadOptionMemMapCreateDevicePath (
  IN  BDS_SUPPORTED_DEVICE* BdsLoadOption,
  OUT EFI_DEVICE_PATH_PROTOCOL **DevicePathNode,
  OUT BDS_LOADER_TYPE   *BootType,
  OUT UINT32      *Attributes
  );

EFI_STATUS
BdsLoadOptionMemMapUpdateDevicePath (
  IN EFI_DEVICE_PATH *OldDevicePath,
  OUT EFI_DEVICE_PATH_PROTOCOL** NewDevicePath,
  OUT BDS_LOADER_TYPE *BootType,
  OUT UINT32 *Attributes
  );

BOOLEAN
BdsLoadOptionMemMapIsSupported (
  IN BDS_LOAD_OPTION* BdsLoadOption
  );

EFI_STATUS
BdsLoadOptionPxeList (
  IN OUT LIST_ENTRY* BdsLoadOptionList
  );

EFI_STATUS
BdsLoadOptionPxeCreateDevicePath (
  IN  BDS_SUPPORTED_DEVICE* BdsLoadOption,
  OUT EFI_DEVICE_PATH_PROTOCOL **DevicePathNode,
  OUT BDS_LOADER_TYPE   *BootType,
  OUT UINT32      *Attributes
  );

EFI_STATUS
BdsLoadOptionPxeUpdateDevicePath (
  IN EFI_DEVICE_PATH *OldDevicePath,
  OUT EFI_DEVICE_PATH_PROTOCOL** NewDevicePath,
  OUT BDS_LOADER_TYPE *BootType,
  OUT UINT32 *Attributes
  );

BOOLEAN
BdsLoadOptionPxeIsSupported (
  IN BDS_LOAD_OPTION* BdsLoadOption
  );

EFI_STATUS
BdsLoadOptionTftpList (
  IN OUT LIST_ENTRY* BdsLoadOptionList
  );

EFI_STATUS
BdsLoadOptionTftpCreateDevicePath (
  IN  BDS_SUPPORTED_DEVICE* BdsLoadOption,
  OUT EFI_DEVICE_PATH_PROTOCOL **DevicePathNode,
  OUT BDS_LOADER_TYPE   *BootType,
  OUT UINT32      *Attributes
  );

EFI_STATUS
BdsLoadOptionTftpUpdateDevicePath (
  IN EFI_DEVICE_PATH *OldDevicePath,
  OUT EFI_DEVICE_PATH_PROTOCOL** NewDevicePath,
  OUT BDS_LOADER_TYPE *BootType,
  OUT UINT32 *Attributes
  );

BOOLEAN
BdsLoadOptionTftpIsSupported (
  IN BDS_LOAD_OPTION* BdsLoadOption
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
    BdsLoadOptionSupportList[Index].ListDevices(SupportedDeviceList);
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
      FreePool(SupportedDevice);
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
BootDeviceGetDeviceSupport (
  IN  BDS_LOAD_OPTION *BootOption,
  OUT BDS_LOAD_OPTION_SUPPORT**  DeviceSupport
  )
{
  UINTN Index;

  // Find which supported device is the most appropriate
  for (Index = 0; Index < BDS_DEVICE_MAX; Index++) {
    if (BdsLoadOptionSupportList[Index].IsSupported(BootOption)) {
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
  OUT BDS_LOADER_TYPE *BootType,
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
      SupportedDevice = (BDS_SUPPORTED_DEVICE*)AllocatePool(sizeof(BDS_SUPPORTED_DEVICE));

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
  IN  BDS_SUPPORTED_DEVICE* BdsLoadOption,
  OUT EFI_DEVICE_PATH_PROTOCOL **DevicePathNode,
  OUT BDS_LOADER_TYPE   *BootType,
  OUT UINT32      *Attributes
  )
{
  EFI_STATUS  Status;
  FILEPATH_DEVICE_PATH* FilePathDevicePath;
  CHAR8       AsciiBootFilePath[BOOT_DEVICE_FILEPATH_MAX];
  CHAR16      *BootFilePath;
  UINTN       BootFilePathSize;

  Status = GetHIInputAscii (AsciiBootFilePath,BOOT_DEVICE_FILEPATH_MAX);
  if (EFI_ERROR(Status)) {
    return EFI_ABORTED;
  }

  if (AsciiStrSize(AsciiBootFilePath) == 1) {
    *DevicePathNode = NULL;
    return EFI_NOT_FOUND;
  }

  // Convert Ascii into Unicode
  BootFilePath = (CHAR16*)AllocatePool(AsciiStrSize(AsciiBootFilePath) * sizeof(CHAR16));
  AsciiStrToUnicodeStr (AsciiBootFilePath, BootFilePath);
  BootFilePathSize = StrSize(BootFilePath);

  // Create the FilePath Device Path node
  FilePathDevicePath = (FILEPATH_DEVICE_PATH*)AllocatePool(SIZE_OF_FILEPATH_DEVICE_PATH + BootFilePathSize);
  FilePathDevicePath->Header.Type = MEDIA_DEVICE_PATH;
  FilePathDevicePath->Header.SubType = MEDIA_FILEPATH_DP;
  SetDevicePathNodeLength (FilePathDevicePath, SIZE_OF_FILEPATH_DEVICE_PATH + BootFilePathSize);
  CopyMem (FilePathDevicePath->PathName, BootFilePath, BootFilePathSize);
  FreePool (BootFilePath);

  if (BootType != NULL || Attributes != NULL) {
    Status = BootDeviceGetType (FilePathDevicePath->PathName, BootType, Attributes);
  }

  if (EFI_ERROR(Status)) {
    FreePool (FilePathDevicePath);
  } else {
    *DevicePathNode = (EFI_DEVICE_PATH_PROTOCOL*)FilePathDevicePath;
  }

  return Status;
}

EFI_STATUS
BdsLoadOptionFileSystemUpdateDevicePath (
  IN EFI_DEVICE_PATH *OldDevicePath,
  OUT EFI_DEVICE_PATH_PROTOCOL** NewDevicePath,
  OUT BDS_LOADER_TYPE *BootType,
  OUT UINT32 *Attributes
  )
{
  EFI_STATUS  Status;
  CHAR8       AsciiBootFilePath[BOOT_DEVICE_FILEPATH_MAX];
  CHAR16      *BootFilePath;
  UINTN       BootFilePathSize;
  FILEPATH_DEVICE_PATH* EndingDevicePath;
  FILEPATH_DEVICE_PATH* FilePathDevicePath;
  EFI_DEVICE_PATH*  DevicePath;

  DevicePath = DuplicateDevicePath(OldDevicePath);

  EndingDevicePath = (FILEPATH_DEVICE_PATH*)GetLastDevicePathNode (DevicePath);
 
  UnicodeStrToAsciiStr (EndingDevicePath->PathName,AsciiBootFilePath);
  Status = EditHIInputAscii (AsciiBootFilePath,BOOT_DEVICE_FILEPATH_MAX);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  if (AsciiStrSize(AsciiBootFilePath) == 1) {
    *NewDevicePath = NULL;
    return EFI_NOT_FOUND;
  }

  // Convert Ascii into Unicode
  BootFilePath = (CHAR16*)AllocatePool(AsciiStrSize(AsciiBootFilePath) * sizeof(CHAR16));
  AsciiStrToUnicodeStr (AsciiBootFilePath, BootFilePath);
  BootFilePathSize = StrSize(BootFilePath);

  // Create the FilePath Device Path node
  FilePathDevicePath = (FILEPATH_DEVICE_PATH*)AllocatePool(SIZE_OF_FILEPATH_DEVICE_PATH + BootFilePathSize);
  FilePathDevicePath->Header.Type = MEDIA_DEVICE_PATH;
  FilePathDevicePath->Header.SubType = MEDIA_FILEPATH_DP;
  SetDevicePathNodeLength (FilePathDevicePath, SIZE_OF_FILEPATH_DEVICE_PATH + BootFilePathSize);
  CopyMem (FilePathDevicePath->PathName, BootFilePath, BootFilePathSize);
  FreePool (BootFilePath);

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
  IN BDS_LOAD_OPTION* BdsLoadOption
  )
{
  EFI_DEVICE_PATH*  DevicePathNode;

  DevicePathNode = GetLastDevicePathNode (BdsLoadOption->FilePathList);

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
  IN  BDS_SUPPORTED_DEVICE* BdsLoadOption,
  OUT EFI_DEVICE_PATH_PROTOCOL **DevicePathNode,
  OUT BDS_LOADER_TYPE   *BootType,
  OUT UINT32      *Attributes
  )
{
  EFI_STATUS  Status;
  MEMMAP_DEVICE_PATH* MemMapDevicePath;
  CHAR8       AsciiStartingAddress[BOOT_DEVICE_ADDRESS_MAX];
  CHAR8       AsciiEndingAddress[BOOT_DEVICE_ADDRESS_MAX];

  Print(L"Starting Address of the binary: ");
  Status = GetHIInputAscii (AsciiStartingAddress,BOOT_DEVICE_ADDRESS_MAX);
  if (EFI_ERROR(Status)) {
    return EFI_ABORTED;
  }

  Print(L"Ending Address of the binary: ");
  Status = GetHIInputAscii (AsciiEndingAddress,BOOT_DEVICE_ADDRESS_MAX);
  if (EFI_ERROR(Status)) {
    return EFI_ABORTED;
  }

  // Create the MemMap Device Path Node
  MemMapDevicePath = (MEMMAP_DEVICE_PATH*)AllocatePool(sizeof(MEMMAP_DEVICE_PATH));
  MemMapDevicePath->Header.Type = HARDWARE_DEVICE_PATH;
  MemMapDevicePath->Header.SubType = HW_MEMMAP_DP;
  MemMapDevicePath->MemoryType = EfiBootServicesData;
  MemMapDevicePath->StartingAddress = AsciiStrHexToUint64 (AsciiStartingAddress);
  MemMapDevicePath->EndingAddress = AsciiStrHexToUint64 (AsciiEndingAddress);

  Status = BootDeviceGetType (NULL, BootType, Attributes);
  if (EFI_ERROR(Status)) {
    FreePool (MemMapDevicePath);
  } else {
    *DevicePathNode = (EFI_DEVICE_PATH_PROTOCOL*)MemMapDevicePath;
  }

  return Status;
}

EFI_STATUS
BdsLoadOptionMemMapUpdateDevicePath (
  IN EFI_DEVICE_PATH *OldDevicePath,
  OUT EFI_DEVICE_PATH_PROTOCOL** NewDevicePath,
  OUT BDS_LOADER_TYPE *BootType,
  OUT UINT32 *Attributes
  )
{
  EFI_STATUS          Status;
  CHAR8               AsciiStartingAddress[BOOT_DEVICE_ADDRESS_MAX];
  CHAR8               AsciiEndingAddress[BOOT_DEVICE_ADDRESS_MAX];
  MEMMAP_DEVICE_PATH* EndingDevicePath;
  EFI_DEVICE_PATH*    DevicePath;

  DevicePath = DuplicateDevicePath (OldDevicePath);
  EndingDevicePath = (MEMMAP_DEVICE_PATH*)GetLastDevicePathNode (DevicePath);

  Print(L"Starting Address of the binary: ");
  AsciiSPrint (AsciiStartingAddress,BOOT_DEVICE_ADDRESS_MAX,"0x%X",(UINTN)EndingDevicePath->StartingAddress);
  Status = EditHIInputAscii (AsciiStartingAddress,BOOT_DEVICE_ADDRESS_MAX);
  if (EFI_ERROR(Status)) {
    return EFI_ABORTED;
  }

  Print(L"Ending Address of the binary: ");
  AsciiSPrint (AsciiEndingAddress,BOOT_DEVICE_ADDRESS_MAX,"0x%X",(UINTN)EndingDevicePath->EndingAddress);
  Status = EditHIInputAscii (AsciiEndingAddress,BOOT_DEVICE_ADDRESS_MAX);
  if (EFI_ERROR(Status)) {
    return EFI_ABORTED;
  }

  EndingDevicePath->StartingAddress = AsciiStrHexToUint64 (AsciiStartingAddress);
  EndingDevicePath->EndingAddress = AsciiStrHexToUint64 (AsciiEndingAddress);

  Status = BootDeviceGetType (NULL, BootType, Attributes);
  if (EFI_ERROR(Status)) {
    FreePool(DevicePath);
  } else {
    *NewDevicePath = DevicePath;
  }

  return Status;
}

BOOLEAN
BdsLoadOptionMemMapIsSupported (
  IN BDS_LOAD_OPTION* BdsLoadOption
  )
{
  EFI_DEVICE_PATH*  DevicePathNode;

  DevicePathNode = GetLastDevicePathNode (BdsLoadOption->FilePathList);

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
  IN  BDS_SUPPORTED_DEVICE* BdsLoadOption,
  OUT EFI_DEVICE_PATH_PROTOCOL **DevicePathNode,
  OUT BDS_LOADER_TYPE   *BootType,
  OUT UINT32      *Attributes
  )
{
  *DevicePathNode = (EFI_DEVICE_PATH_PROTOCOL *) AllocatePool (END_DEVICE_PATH_LENGTH);
  SetDevicePathEndNode (*DevicePathNode);
  *BootType = BDS_LOADER_EFI_APPLICATION;
  return EFI_SUCCESS;
}

EFI_STATUS
BdsLoadOptionPxeUpdateDevicePath (
  IN EFI_DEVICE_PATH *OldDevicePath,
  OUT EFI_DEVICE_PATH_PROTOCOL** NewDevicePath,
  OUT BDS_LOADER_TYPE *BootType,
  OUT UINT32 *Attributes
  )
{
  ASSERT (0);
  return EFI_SUCCESS;
}

BOOLEAN
BdsLoadOptionPxeIsSupported (
  IN BDS_LOAD_OPTION* BdsLoadOption
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE Handle;
  EFI_DEVICE_PATH_PROTOCOL  *RemainingDevicePath;
  EFI_PXE_BASE_CODE_PROTOCOL  *PxeBcProtocol;

  Status = BdsConnectDevicePath (BdsLoadOption->FilePathList, &Handle, &RemainingDevicePath);
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
      UnicodeSPrint (SupportedDevice->Description,BOOT_DEVICE_DESCRIPTION_MAX,L"TFP on %s",DeviceDescription);

      SupportedDevice->DevicePathProtocol = DevicePathProtocol;
      SupportedDevice->Support = &BdsLoadOptionSupportList[BDS_DEVICE_TFTP];

      InsertTailList (BdsLoadOptionList,&SupportedDevice->Link);
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
BdsLoadOptionTftpCreateDevicePath (
  IN  BDS_SUPPORTED_DEVICE* BdsLoadOption,
  OUT EFI_DEVICE_PATH_PROTOCOL **DevicePathNode,
  OUT BDS_LOADER_TYPE   *BootType,
  OUT UINT32      *Attributes
  )
{
  EFI_STATUS    Status;
  BOOLEAN       IsDHCP;
  EFI_IP_ADDRESS  LocalIp;
  EFI_IP_ADDRESS  RemoteIp;
  IPv4_DEVICE_PATH*   IPv4DevicePathNode;
  FILEPATH_DEVICE_PATH* FilePathDevicePath;
  CHAR8       AsciiBootFilePath[BOOT_DEVICE_FILEPATH_MAX];
  CHAR16*     BootFilePath;
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

  Print(L"File path of the EFI Application or the kernel: ");
  Status = GetHIInputAscii (AsciiBootFilePath,BOOT_DEVICE_FILEPATH_MAX);
  if (EFI_ERROR(Status)) {
    return EFI_ABORTED;
  }

  // Convert Ascii into Unicode
  BootFilePath = (CHAR16*)AllocatePool(AsciiStrSize(AsciiBootFilePath) * sizeof(CHAR16));
  AsciiStrToUnicodeStr (AsciiBootFilePath, BootFilePath);
  BootFilePathSize = StrSize(BootFilePath);

  // Allocate the memory for the IPv4 + File Path Device Path Nodes
  IPv4DevicePathNode = (IPv4_DEVICE_PATH*)AllocatePool(sizeof(IPv4_DEVICE_PATH) + SIZE_OF_FILEPATH_DEVICE_PATH + BootFilePathSize);

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
  FreePool (BootFilePath);

  Status = BootDeviceGetType (NULL, BootType, Attributes);
  if (EFI_ERROR(Status)) {
    FreePool (IPv4DevicePathNode);
  } else {
    *DevicePathNode = (EFI_DEVICE_PATH_PROTOCOL*)IPv4DevicePathNode;
  }

  return Status;
}

EFI_STATUS
BdsLoadOptionTftpUpdateDevicePath (
  IN EFI_DEVICE_PATH *OldDevicePath,
  OUT EFI_DEVICE_PATH_PROTOCOL** NewDevicePath,
  OUT BDS_LOADER_TYPE *BootType,
  OUT UINT32 *Attributes
  )
{
  ASSERT (0);
  return EFI_SUCCESS;
}

BOOLEAN
BdsLoadOptionTftpIsSupported (
  IN BDS_LOAD_OPTION* BdsLoadOption
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE Handle;
  EFI_DEVICE_PATH  *RemainingDevicePath;
  EFI_DEVICE_PATH  *NextDevicePath;
  EFI_PXE_BASE_CODE_PROTOCOL  *PxeBcProtocol;

  Status = BdsConnectDevicePath (BdsLoadOption->FilePathList, &Handle, &RemainingDevicePath);
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
