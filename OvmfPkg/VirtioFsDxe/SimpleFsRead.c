/** @file
  EFI_FILE_PROTOCOL.Read() member function for the Virtio Filesystem driver.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "VirtioFsDxe.h"

/**
  Read from a regular file.
**/
STATIC
EFI_STATUS
ReadRegularFile (
  IN OUT VIRTIO_FS_FILE *VirtioFsFile,
  IN OUT UINTN          *BufferSize,
     OUT VOID           *Buffer
  )
{
  VIRTIO_FS                          *VirtioFs;
  EFI_STATUS                         Status;
  VIRTIO_FS_FUSE_ATTRIBUTES_RESPONSE FuseAttr;
  UINTN                              Transferred;
  UINTN                              Left;

  VirtioFs = VirtioFsFile->OwnerFs;
  //
  // The UEFI spec forbids reads that start beyond the end of the file.
  //
  Status = VirtioFsFuseGetAttr (VirtioFs, VirtioFsFile->NodeId, &FuseAttr);
  if (EFI_ERROR (Status) || VirtioFsFile->FilePosition > FuseAttr.Size) {
    return EFI_DEVICE_ERROR;
  }

  Status      = EFI_SUCCESS;
  Transferred = 0;
  Left        = *BufferSize;
  while (Left > 0) {
    UINT32 ReadSize;

    //
    // FUSE_READ cannot express a >=4GB buffer size.
    //
    ReadSize = (UINT32)MIN ((UINTN)MAX_UINT32, Left);
    Status = VirtioFsFuseReadFileOrDir (
               VirtioFs,
               VirtioFsFile->NodeId,
               VirtioFsFile->FuseHandle,
               FALSE,                                    // IsDir
               VirtioFsFile->FilePosition + Transferred,
               &ReadSize,
               (UINT8 *)Buffer + Transferred
               );
    if (EFI_ERROR (Status) || ReadSize == 0) {
      break;
    }
    Transferred += ReadSize;
    Left        -= ReadSize;
  }

  *BufferSize = Transferred;
  VirtioFsFile->FilePosition += Transferred;
  //
  // If we managed to read some data, return success. If zero bytes were
  // transferred due to zero-sized buffer on input or due to EOF on first read,
  // return SUCCESS. Otherwise, return the error due to which zero bytes were
  // transferred.
  //
  return (Transferred > 0) ? EFI_SUCCESS : Status;
}

EFI_STATUS
EFIAPI
VirtioFsSimpleFileRead (
  IN     EFI_FILE_PROTOCOL *This,
  IN OUT UINTN             *BufferSize,
     OUT VOID              *Buffer
  )
{
  VIRTIO_FS_FILE *VirtioFsFile;
  EFI_STATUS     Status;

  VirtioFsFile = VIRTIO_FS_FILE_FROM_SIMPLE_FILE (This);

  if (VirtioFsFile->IsDirectory) {
    Status = EFI_NO_MEDIA;
  } else {
    Status = ReadRegularFile (VirtioFsFile, BufferSize, Buffer);
  }
  return Status;
}
