/** @file
  EFI_FILE_PROTOCOL.Flush() member function for the Virtio Filesystem driver.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "VirtioFsDxe.h"

EFI_STATUS
EFIAPI
VirtioFsSimpleFileFlush (
  IN EFI_FILE_PROTOCOL  *This
  )
{
  VIRTIO_FS_FILE  *VirtioFsFile;
  VIRTIO_FS       *VirtioFs;
  EFI_STATUS      Status;

  VirtioFsFile = VIRTIO_FS_FILE_FROM_SIMPLE_FILE (This);
  VirtioFs     = VirtioFsFile->OwnerFs;

  if (!VirtioFsFile->IsOpenForWriting) {
    return EFI_ACCESS_DENIED;
  }

  //
  // FUSE_FLUSH is for regular files only.
  //
  if (!VirtioFsFile->IsDirectory) {
    Status = VirtioFsFuseFlush (
               VirtioFs,
               VirtioFsFile->NodeId,
               VirtioFsFile->FuseHandle
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  Status = VirtioFsFuseFsyncFileOrDir (
             VirtioFs,
             VirtioFsFile->NodeId,
             VirtioFsFile->FuseHandle,
             VirtioFsFile->IsDirectory
             );
  return Status;
}
