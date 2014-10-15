/*++

Caution: This file is used for Duet platform only, do not use them in real platform.
All variable code, variable metadata, and variable data used by Duet platform are on 
disk. They can be changed by user. BIOS is not able to protoect those.
Duet trusts all meta data from disk. If variable code, variable metadata and variable
data is modified in inproper way, the behavior is undefined.

Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

    FileStorage.c

Abstract:

    handles variable store/reads on file

Revision History

--*/
#include "FSVariable.h"

VOID             *mSFSRegistration;

//
// Prototypes
//

VOID
EFIAPI
OnVirtualAddressChangeFs (
  IN EFI_EVENT            Event,
  IN VOID                 *Context
  );

EFI_STATUS
EFIAPI
FileEraseStore(
  IN VARIABLE_STORAGE     *This
  );

EFI_STATUS
EFIAPI
FileWriteStore (
  IN VARIABLE_STORAGE     *This,
  IN UINTN                Offset,
  IN UINTN                BufferSize,
  IN VOID                 *Buffer
  );

EFI_STATUS
OpenStore (
  IN  EFI_DEVICE_PATH_PROTOCOL  *Device,
  IN  CHAR16                    *FilePathName,
  IN  UINT64                    OpenMode,
  OUT EFI_FILE_PROTOCOL         **File
  );

//
// Implementation below:
//
VOID
FileClose (
  IN  EFI_FILE_PROTOCOL          *File
  )
{
  EFI_STATUS Status;

  Status = File->Flush (File);
  ASSERT_EFI_ERROR (Status);

  Status = File->Close (File);
  ASSERT_EFI_ERROR (Status);
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
    DEBUG ((EFI_D_ERROR, "FileStorage: Media not present!\n"));
    Status = EFI_NO_MEDIA;
    goto ErrHandle;
  }
  if (BlkIo->Media->ReadOnly) {
    DEBUG ((EFI_D_ERROR, "FileStorage: Media is read-only!\n"));
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
                  (VOID **) &Volume
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

// this routine is still running in BS period, no limitation
// call FileInitStorage(), which load variable content file to memory
// read the store_header, init store_header if it has not been inited (read sth. about format/heathy)
// reclaim space using scratch memory

VOID
EFIAPI
OnSimpleFileSystemInstall (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  EFI_STATUS                Status;
  UINTN                     HandleSize;
  EFI_HANDLE                Handle;
  EFI_DEVICE_PATH_PROTOCOL  *Device;
  VS_DEV                    *Dev;
  EFI_FILE_PROTOCOL         *File;
  UINTN                     NumBytes;

  Dev = (VS_DEV *) Context;
  
  if (VAR_FILE_DEVICEPATH (Dev) != NULL &&
      !EFI_ERROR (CheckStoreExists (VAR_FILE_DEVICEPATH (Dev)))
     ) {
    DEBUG ((EFI_D_ERROR, "FileStorage: Already mapped!\n"));
    return ;
  }

  while (TRUE) {
    HandleSize = sizeof (EFI_HANDLE);
    Status = gBS->LocateHandle (
                    ByRegisterNotify,
                    NULL,
                    mSFSRegistration,
                    &HandleSize,
                    &Handle
                    );
    if (EFI_ERROR (Status)) {
      return ;
    }
    
    Status = CheckStore (Handle, VAR_FILE_VOLUMEID (Dev), &Device);
    if (!EFI_ERROR (Status)) {
      break;
    }
  }

  VAR_FILE_DEVICEPATH (Dev) = Device;
  Status = OpenStore (
             VAR_FILE_DEVICEPATH (Dev), 
             VAR_FILE_FILEPATH (Dev), 
             EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ | EFI_FILE_MODE_CREATE,
             &File
             );
  ASSERT_EFI_ERROR (Status);
  
  NumBytes = Dev->Size;
  Status = File->Write (File, &NumBytes, VAR_DATA_PTR (Dev));
  ASSERT_EFI_ERROR (Status);
  FileClose (File);
  DEBUG ((EFI_D_ERROR, "FileStorage: Mapped to file!\n"));
}

EFI_STATUS
FileStorageConstructor (
  OUT VARIABLE_STORAGE      **VarStore,
  OUT EFI_EVENT_NOTIFY      *GoVirtualEvent,
  IN  EFI_PHYSICAL_ADDRESS  NvStorageBase,
  IN  UINTN                 Size,
  IN  UINT32                VolumeId,
  IN  CHAR16                *FilePath
  )
{
  VS_DEV                    *Dev;
  EFI_STATUS                Status;
  EFI_EVENT                 Event;

  Status = gBS->AllocatePool (EfiRuntimeServicesData, sizeof(VS_DEV), (VOID **) &Dev);
  ASSERT_EFI_ERROR (Status);
  ZeroMem (Dev, sizeof(VS_DEV));

  Dev->Signature          = VS_DEV_SIGNATURE;
  Dev->Size               = Size;
  VAR_DATA_PTR (Dev)      = (UINT8 *) (UINTN) NvStorageBase;
  VAR_FILE_VOLUMEID (Dev) = VolumeId;
  StrCpy (VAR_FILE_FILEPATH (Dev), FilePath);
  Dev->VarStore.Erase     = FileEraseStore;
  Dev->VarStore.Write     = FileWriteStore;

  DEBUG ((EFI_D_ERROR, "FileStorageConstructor(0x%0x:0x%0x): added!\n", NvStorageBase, Size));

  // add notify on SFS's installation.

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  OnSimpleFileSystemInstall,
                  Dev,
                  &Event
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->RegisterProtocolNotify (
                  &gEfiSimpleFileSystemProtocolGuid,
                  Event,
                  &mSFSRegistration
                  );
  ASSERT_EFI_ERROR (Status);

  *VarStore       = &Dev->VarStore;
  *GoVirtualEvent = OnVirtualAddressChangeFs;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
FileEraseStore(
  IN VARIABLE_STORAGE   *This
  )
{
  EFI_STATUS              Status;
  VS_DEV                  *Dev;
  EFI_FILE_PROTOCOL       *File;
  UINTN                   NumBytes;

  Status = EFI_SUCCESS;
  Dev    = DEV_FROM_THIS(This);

  SetMem (VAR_DATA_PTR (Dev), Dev->Size, VAR_DEFAULT_VALUE);

  if (!EfiAtRuntime () && VAR_FILE_DEVICEPATH (Dev) != NULL) {
    Status = OpenStore (
               VAR_FILE_DEVICEPATH (Dev), 
               VAR_FILE_FILEPATH (Dev), 
               EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ,
               &File
               );
    ASSERT_EFI_ERROR (Status);
    NumBytes = Dev->Size;
    Status = File->Write (File, &NumBytes, VAR_DATA_PTR (Dev));
    ASSERT_EFI_ERROR (Status);
    FileClose (File);
  }
  
  return Status;
}

EFI_STATUS
EFIAPI
FileWriteStore (
  IN VARIABLE_STORAGE     *This,
  IN UINTN                Offset,
  IN UINTN                BufferSize,
  IN VOID                 *Buffer
  )
{
  EFI_STATUS              Status;
  VS_DEV                  *Dev;
  EFI_FILE_PROTOCOL       *File;

  Status = EFI_SUCCESS;
  Dev    = DEV_FROM_THIS(This);

  ASSERT (Buffer != NULL);
  ASSERT (Offset + BufferSize <= Dev->Size);

  CopyMem (VAR_DATA_PTR (Dev) + Offset, Buffer, BufferSize);
  
  if (!EfiAtRuntime () && VAR_FILE_DEVICEPATH (Dev) != NULL) {
    Status = OpenStore (
               VAR_FILE_DEVICEPATH (Dev), 
               VAR_FILE_FILEPATH (Dev), 
               EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ,
               &File
               );
    Status = File->SetPosition (File, Offset);
    ASSERT_EFI_ERROR (Status);
    Status = File->Write (File, &BufferSize, Buffer);
    ASSERT_EFI_ERROR (Status);
    FileClose (File);
  }
  return Status;
}

VOID
EFIAPI
OnVirtualAddressChangeFs (
  IN EFI_EVENT            Event,
  IN VOID                 *Context
  )
{
  VS_DEV  *Dev;

  Dev = DEV_FROM_THIS (Context);

  EfiConvertPointer (0, (VOID **) &VAR_DATA_PTR (Dev));
  EfiConvertPointer (0, (VOID **) &Dev->VarStore.Erase);
  EfiConvertPointer (0, (VOID **) &Dev->VarStore.Write);
}

EFI_STATUS
OpenStore (
  IN  EFI_DEVICE_PATH_PROTOCOL  *Device,
  IN  CHAR16                    *FilePathName,
  IN  UINT64                    OpenMode,
  OUT EFI_FILE_PROTOCOL         **File
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
                  (VOID **) &Volume
                  );
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
                   FilePathName,
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
