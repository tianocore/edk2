/**@file
Copyright (c) 2007 - 2009, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

    FileIo.c

Abstract:

  File operation for Firmware volume block driver

**/
#include "FileIo.h"

//
// Variable storage hot plug is supported but there are still some restrictions:
// After plugging the storage back,
// 1. Still use memory as NV if newly plugged storage is not same as the original one
// 2. Still use memory as NV if there are some update operation during storage is unplugged.
//


EFI_STATUS
FileWrite (
  IN EFI_FILE_PROTOCOL  *File,
  IN UINTN              Offset,
  IN UINTN              Buffer,
  IN UINTN              Size
  )
{
  EFI_STATUS Status;

  Status = File->SetPosition (File, Offset);
  ASSERT_EFI_ERROR (Status);
  if (!EFI_ERROR (Status)) {
    Status = File->Write (File, &Size, (VOID *) Buffer);
    ASSERT_EFI_ERROR (Status);
  }
  return Status;
}

EFI_STATUS
CheckStore (
  IN  EFI_HANDLE                 SimpleFileSystemHandle,
  IN  UINT32                     VolumeId,
  OUT EFI_DEVICE_PATH_PROTOCOL   **Device
  )
{
#define BLOCK_SIZE              0x200
#define FAT16_VOLUME_ID_OFFSET  39
#define FAT32_VOLUME_ID_OFFSET  67
  EFI_STATUS                      Status;
  EFI_BLOCK_IO_PROTOCOL           *BlkIo;
  UINT8                           BootSector[BLOCK_SIZE];

  *Device = NULL;
  Status  = gBS->HandleProtocol (
                   SimpleFileSystemHandle,
                   &gEfiBlockIoProtocolGuid, // BlockIo should be supported if it supports SimpleFileSystem
                   (VOID*)&BlkIo
                   );

  if (EFI_ERROR (Status)) {
    goto ErrHandle;
  }
  if (!BlkIo->Media->MediaPresent) {
    DEBUG ((EFI_D_ERROR, "FwhMappedFile: Media not present!\n"));
    Status = EFI_NO_MEDIA;
    goto ErrHandle;
  }
  if (BlkIo->Media->ReadOnly) {
    DEBUG ((EFI_D_ERROR, "FwhMappedFile: Media is read-only!\n"));
    Status = EFI_ACCESS_DENIED;
    goto ErrHandle;
  }

  Status = BlkIo->ReadBlocks(
                    BlkIo,
                    BlkIo->Media->MediaId,
                    0,
                    BLOCK_SIZE,
                    BootSector
                    );
  ASSERT_EFI_ERROR (Status);
  if ((*(UINT32 *) &BootSector[FAT16_VOLUME_ID_OFFSET] != VolumeId) &&
      (*(UINT32 *) &BootSector[FAT32_VOLUME_ID_OFFSET] != VolumeId)
      ) {
    Status = EFI_NOT_FOUND;
    goto ErrHandle;
  }

  *Device = DuplicateDevicePath (DevicePathFromHandle (SimpleFileSystemHandle));
  ASSERT (*Device != NULL);

ErrHandle:
  return Status;
}

EFI_STATUS
CheckStoreExists (
  IN  EFI_DEVICE_PATH_PROTOCOL   *Device
  )
{
  EFI_HANDLE                        Handle;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL   *Volume;
  EFI_STATUS                        Status;

  Status = gBS->LocateDevicePath (
                  &gEfiSimpleFileSystemProtocolGuid,
                  &Device, 
                  &Handle
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  &Volume
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

VOID
FileClose (
  IN  EFI_FILE_PROTOCOL          *File
  )
{
  File->Flush (File);
  File->Close (File);
}
EFI_STATUS
FileOpen (
  IN  EFI_DEVICE_PATH_PROTOCOL   *Device,
  IN  CHAR16                     *MappedFile,
  OUT EFI_FILE_PROTOCOL          **File,
  IN  UINT64                     OpenMode
  )
{  
  EFI_HANDLE                        Handle;
  EFI_FILE_HANDLE                   Root;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL   *Volume;
  EFI_STATUS                        Status;

  *File = NULL;

  Status = gBS->LocateDevicePath (
                  &gEfiSimpleFileSystemProtocolGuid,
                  &Device, 
                  &Handle
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  &Volume
                  );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  
  //
  // Open the root directory of the volume
  //
  Root = NULL;
  Status = Volume->OpenVolume (
                     Volume,
                     &Root
                     );
  ASSERT_EFI_ERROR (Status);
  ASSERT (Root != NULL);

  //
  // Open file
  //
  Status = Root->Open (
                   Root,
                   File,
                   MappedFile,
                   OpenMode,
                   0
                   );
  if (EFI_ERROR (Status)) {
    *File = NULL;
  }

  //
  // Close the Root directory
  //
  Root->Close (Root);
  return Status;
}
