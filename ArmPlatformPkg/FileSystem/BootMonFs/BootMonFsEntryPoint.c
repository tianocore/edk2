/** @file
*
*  Copyright (c) 2012-2014, ARM Limited. All rights reserved.
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

#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/DevicePathFromText.h>
#include <Protocol/DriverBinding.h>

#include "BootMonFsInternal.h"

EFI_DEVICE_PATH* mBootMonFsSupportedDevicePaths;
EFI_HANDLE       mImageHandle;
LIST_ENTRY       mInstances;

EFI_FILE_PROTOCOL mBootMonFsRootTemplate = {
  EFI_FILE_PROTOCOL_REVISION,
  BootMonFsOpenFile,
  BootMonFsCloseFile,
  BootMonFsDeleteFail,
  BootMonFsReadDirectory,
  BootMonFsWriteFile,
  BootMonFsGetPositionUnsupported,  // UEFI Spec: GetPosition not valid on dirs
  BootMonFsSetDirPosition,
  BootMonFsGetInfo,
  BootMonFsSetInfo,
  BootMonFsFlushDirectory
};

EFI_FILE_PROTOCOL mBootMonFsFileTemplate = {
  EFI_FILE_PROTOCOL_REVISION,
  BootMonFsOpenFile,
  BootMonFsCloseFile,
  BootMonFsDelete,
  BootMonFsReadFile,
  BootMonFsWriteFile,
  BootMonFsGetPosition,
  BootMonFsSetPosition,
  BootMonFsGetInfo,
  BootMonFsSetInfo,
  BootMonFsFlushFile
};

EFI_STATUS
BootMonGetFileFromAsciiFileName (
  IN  BOOTMON_FS_INSTANCE   *Instance,
  IN  CHAR8*                AsciiFileName,
  OUT BOOTMON_FS_FILE       **File
  )
{
  LIST_ENTRY        *Entry;
  BOOTMON_FS_FILE   *FileEntry;

  // Remove the leading '\\'
  if (*AsciiFileName == '\\') {
    AsciiFileName++;
  }

  // Go through all the files in the list and return the file handle
  for (Entry = GetFirstNode (&Instance->RootFile->Link);
         !IsNull (&Instance->RootFile->Link, Entry);
         Entry = GetNextNode (&Instance->RootFile->Link, Entry)
         )
  {
    FileEntry = BOOTMON_FS_FILE_FROM_LINK_THIS (Entry);
    if (AsciiStrCmp (FileEntry->HwDescription.Footer.Filename, AsciiFileName) == 0) {
      *File = FileEntry;
      return EFI_SUCCESS;
    }
  }
  return EFI_NOT_FOUND;
}

EFI_STATUS
BootMonGetFileFromPosition (
  IN  BOOTMON_FS_INSTANCE   *Instance,
  IN  UINTN                 Position,
  OUT BOOTMON_FS_FILE       **File
  )
{
  LIST_ENTRY        *Entry;
  BOOTMON_FS_FILE   *FileEntry;

  // Go through all the files in the list and return the file handle
  for (Entry = GetFirstNode (&Instance->RootFile->Link);
       !IsNull (&Instance->RootFile->Link, Entry) && (&Instance->RootFile->Link != Entry);
       Entry = GetNextNode (&Instance->RootFile->Link, Entry)
       )
  {
    if (Position == 0) {
      FileEntry = BOOTMON_FS_FILE_FROM_LINK_THIS (Entry);
      *File = FileEntry;
      return EFI_SUCCESS;
    }
    Position--;
  }
  return EFI_NOT_FOUND;
}

EFI_STATUS
BootMonFsCreateFile (
  IN  BOOTMON_FS_INSTANCE *Instance,
  OUT BOOTMON_FS_FILE     **File
  )
{
  BOOTMON_FS_FILE *NewFile;

  NewFile = (BOOTMON_FS_FILE*)AllocateZeroPool (sizeof (BOOTMON_FS_FILE));
  if (NewFile == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewFile->Signature = BOOTMON_FS_FILE_SIGNATURE;
  InitializeListHead (&NewFile->Link);
  InitializeListHead (&NewFile->RegionToFlushLink);
  NewFile->Instance = Instance;

  // If the created file is the root file then create a directory EFI_FILE_PROTOCOL
  if (Instance->RootFile == *File) {
    CopyMem (&NewFile->File, &mBootMonFsRootTemplate, sizeof (mBootMonFsRootTemplate));
  } else {
    CopyMem (&NewFile->File, &mBootMonFsFileTemplate, sizeof (mBootMonFsFileTemplate));
  }
  *File = NewFile;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
SupportedDevicePathsInit (
  VOID
  )
{
  EFI_STATUS                          Status;
  CHAR16*                             DevicePathListStr;
  CHAR16*                             DevicePathStr;
  CHAR16*                             NextDevicePathStr;
  EFI_DEVICE_PATH_FROM_TEXT_PROTOCOL *EfiDevicePathFromTextProtocol;
  EFI_DEVICE_PATH_PROTOCOL           *Instance;

  Status = gBS->LocateProtocol (&gEfiDevicePathFromTextProtocolGuid, NULL, (VOID **)&EfiDevicePathFromTextProtocol);
  ASSERT_EFI_ERROR (Status);

  // Initialize Variable
  DevicePathListStr = (CHAR16*)PcdGetPtr (PcdBootMonFsSupportedDevicePaths);
  mBootMonFsSupportedDevicePaths = NULL;

  // Extract the Device Path instances from the multi-device path string
  while ((DevicePathListStr != NULL) && (DevicePathListStr[0] != L'\0')) {
    NextDevicePathStr = StrStr (DevicePathListStr, L";");
    if (NextDevicePathStr == NULL) {
      DevicePathStr = DevicePathListStr;
      DevicePathListStr = NULL;
    } else {
      DevicePathStr = (CHAR16*)AllocateCopyPool ((NextDevicePathStr - DevicePathListStr + 1) * sizeof (CHAR16), DevicePathListStr);
      if (DevicePathStr == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
      *(DevicePathStr + (NextDevicePathStr - DevicePathListStr)) = L'\0';
      DevicePathListStr = NextDevicePathStr;
      if (DevicePathListStr[0] == L';') {
        DevicePathListStr++;
      }
    }

    Instance = EfiDevicePathFromTextProtocol->ConvertTextToDevicePath (DevicePathStr);
    ASSERT (Instance != NULL);
    mBootMonFsSupportedDevicePaths = AppendDevicePathInstance (mBootMonFsSupportedDevicePaths, Instance);

    if (NextDevicePathStr != NULL) {
      FreePool (DevicePathStr);
    }
    FreePool (Instance);
  }

  if (mBootMonFsSupportedDevicePaths == NULL) {
    return EFI_UNSUPPORTED;
  } else {
    return EFI_SUCCESS;
  }
}

EFI_STATUS
EFIAPI
BootMonFsDriverSupported (
  IN        EFI_DRIVER_BINDING_PROTOCOL *DriverBinding,
  IN        EFI_HANDLE                   ControllerHandle,
  IN        EFI_DEVICE_PATH_PROTOCOL    *DevicePath OPTIONAL
  )
{
  EFI_DISK_IO_PROTOCOL  *DiskIo;
  EFI_DEVICE_PATH_PROTOCOL *DevicePathProtocol;
  EFI_DEVICE_PATH_PROTOCOL *SupportedDevicePath;
  EFI_DEVICE_PATH_PROTOCOL *SupportedDevicePaths;
  EFI_STATUS Status;
  UINTN Size1;
  UINTN Size2;

  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDiskIoProtocolGuid,
                  (VOID **) &DiskIo,
                  mImageHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Close the I/O Abstraction(s) used to perform the supported test
  //
  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiDiskIoProtocolGuid,
         mImageHandle,
         ControllerHandle
         );

  // Check that BlockIo protocol instance exists
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiBlockIoProtocolGuid,
                  NULL,
                  mImageHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Check if a DevicePath is attached to the handle
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&DevicePathProtocol,
                  mImageHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Check if the Device Path is the one which contains the Boot Monitor File System
  Size1 = GetDevicePathSize (DevicePathProtocol);

  // Go through the list of Device Path Instances
  Status = EFI_UNSUPPORTED;
  SupportedDevicePaths = mBootMonFsSupportedDevicePaths;
  while (SupportedDevicePaths != NULL) {
    SupportedDevicePath = GetNextDevicePathInstance (&SupportedDevicePaths, &Size2);

    if ((Size1 == Size2) && (CompareMem (DevicePathProtocol, SupportedDevicePath, Size1) == 0)) {
      // The Device Path is supported
      Status = EFI_SUCCESS;
      break;
    }
  }

  gBS->CloseProtocol (ControllerHandle, &gEfiDevicePathProtocolGuid, mImageHandle, ControllerHandle);
  return Status;
}

EFI_STATUS
EFIAPI
BootMonFsDriverStart (
  IN        EFI_DRIVER_BINDING_PROTOCOL *DriverBinding,
  IN        EFI_HANDLE                   ControllerHandle,
  IN        EFI_DEVICE_PATH_PROTOCOL    *DevicePath OPTIONAL
  )
{
  BOOTMON_FS_INSTANCE *Instance;
  EFI_STATUS           Status;
  UINTN                VolumeNameSize;

  Instance = AllocateZeroPool (sizeof (BOOTMON_FS_INSTANCE));
  if (Instance == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Initialize the BlockIo of the Instance
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiBlockIoProtocolGuid,
                  (VOID **)&(Instance->BlockIo),
                  mImageHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    FreePool (Instance);
    return Status;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDiskIoProtocolGuid,
                  (VOID **)&(Instance->DiskIo),
                  mImageHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    FreePool (Instance);
    return Status;
  }

  //
  // Initialize the attributes of the Instance
  //
  Instance->Signature = BOOTMON_FS_SIGNATURE;
  Instance->ControllerHandle = ControllerHandle;
  Instance->Media = Instance->BlockIo->Media;
  Instance->Binding = DriverBinding;

    // Initialize the Simple File System Protocol
  Instance->Fs.Revision = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION;
  Instance->Fs.OpenVolume = OpenBootMonFsOpenVolume;

  // Volume name + L' ' + '2' digit number
  VolumeNameSize = StrSize (BOOTMON_FS_VOLUME_LABEL) + (3 * sizeof (CHAR16));

  // Initialize FileSystem Information
  Instance->FsInfo.Size = SIZE_OF_EFI_FILE_SYSTEM_INFO + VolumeNameSize;
  Instance->FsInfo.BlockSize = Instance->Media->BlockSize;
  Instance->FsInfo.ReadOnly = FALSE;
  Instance->FsInfo.VolumeSize =
    Instance->Media->BlockSize * (Instance->Media->LastBlock - Instance->Media->LowestAlignedLba);
  CopyMem (Instance->FsInfo.VolumeLabel, BOOTMON_FS_VOLUME_LABEL, StrSize (BOOTMON_FS_VOLUME_LABEL));

  // Initialize the root file
  Status = BootMonFsCreateFile (Instance, &Instance->RootFile);
  if (EFI_ERROR (Status)) {
    FreePool (Instance);
    return Status;
  }

  // Initialize the DevicePath of the Instance
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&(Instance->DevicePath),
                  mImageHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    FreePool (Instance);
    return Status;
  }

  //
  // Install the Simple File System Protocol
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                      &ControllerHandle,
                      &gEfiSimpleFileSystemProtocolGuid, &Instance->Fs,
                      NULL
                      );

  InsertTailList (&mInstances, &Instance->Link);

  return Status;
}


EFI_STATUS
EFIAPI
BootMonFsDriverStop (
  IN        EFI_DRIVER_BINDING_PROTOCOL *DriverBinding,
  IN        EFI_HANDLE                   ControllerHandle,
  IN        UINTN                        NumberOfChildren,
  IN        EFI_HANDLE                  *ChildHandleBuffer OPTIONAL
  )
{
  BOOTMON_FS_INSTANCE *Instance;
  LIST_ENTRY          *Link;
  EFI_STATUS           Status;
  BOOLEAN              InstanceFound;

  // Find instance from ControllerHandle.
  Instance = NULL;
  InstanceFound = FALSE;
  // For each instance in mInstances:
  for (Link = GetFirstNode (&mInstances); !IsNull (&mInstances, Link); Link = GetNextNode (&mInstances, Link)) {
    Instance = BOOTMON_FS_FROM_LINK (Link);

    if (Instance->ControllerHandle == ControllerHandle) {
      InstanceFound = TRUE;
      break;
    }
  }
  ASSERT (InstanceFound == TRUE);

  gBS->CloseProtocol (
      ControllerHandle,
      &gEfiDevicePathProtocolGuid,
      DriverBinding->ImageHandle,
      ControllerHandle);

  gBS->CloseProtocol (
      ControllerHandle,
      &gEfiDiskIoProtocolGuid,
      DriverBinding->ImageHandle,
      ControllerHandle);

  gBS->CloseProtocol (
      ControllerHandle,
      &gEfiBlockIoProtocolGuid,
      DriverBinding->ImageHandle,
      ControllerHandle);

  Status = gBS->UninstallMultipleProtocolInterfaces (
      &ControllerHandle,
      &gEfiSimpleFileSystemProtocolGuid, &Instance->Fs,
      NULL);

  return Status;
}

//
// Simple Network Protocol Driver Global Variables
//
EFI_DRIVER_BINDING_PROTOCOL mBootMonFsDriverBinding = {
  BootMonFsDriverSupported,
  BootMonFsDriverStart,
  BootMonFsDriverStop,
  0xa,
  NULL,
  NULL
};

EFI_STATUS
EFIAPI
BootMonFsEntryPoint (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
{
  EFI_STATUS  Status;

  mImageHandle = ImageHandle;
  InitializeListHead (&mInstances);

  // Initialize the list of Device Paths that could support BootMonFs
  Status = SupportedDevicePathsInit ();
  if (!EFI_ERROR (Status)) {
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &ImageHandle,
                    &gEfiDriverBindingProtocolGuid, &mBootMonFsDriverBinding,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
  } else {
    DEBUG((EFI_D_ERROR,"Warning: No Device Paths supporting BootMonFs have been defined in the PCD.\n"));
  }

  return Status;
}
