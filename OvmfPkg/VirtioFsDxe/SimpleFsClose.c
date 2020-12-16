/** @file
  EFI_FILE_PROTOCOL.Close() member function for the Virtio Filesystem driver.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>             // RemoveEntryList()
#include <Library/MemoryAllocationLib.h> // FreePool()

#include "VirtioFsDxe.h"

EFI_STATUS
EFIAPI
VirtioFsSimpleFileClose (
  IN EFI_FILE_PROTOCOL *This
  )
{
  VIRTIO_FS_FILE *VirtioFsFile;
  VIRTIO_FS      *VirtioFs;

  VirtioFsFile = VIRTIO_FS_FILE_FROM_SIMPLE_FILE (This);
  VirtioFs     = VirtioFsFile->OwnerFs;

  //
  // At this point, the implementation is only suitable for closing the
  // VIRTIO_FS_FILE that was created by VirtioFsOpenVolume().
  //
  ASSERT (VirtioFsFile->IsDirectory);
  ASSERT (VirtioFsFile->NodeId == VIRTIO_FS_FUSE_ROOT_DIR_NODE_ID);
  //
  // Close the root directory.
  //
  // Ignore any errors, because EFI_FILE_PROTOCOL.Close() is required to
  // release the EFI_FILE_PROTOCOL object unconditionally.
  //
  VirtioFsFuseReleaseFileOrDir (VirtioFs, VirtioFsFile->NodeId,
    VirtioFsFile->FuseHandle, VirtioFsFile->IsDirectory);

  //
  // One fewer file left open for the owner filesystem.
  //
  RemoveEntryList (&VirtioFsFile->OpenFilesEntry);

  FreePool (VirtioFsFile);
  return EFI_SUCCESS;
}
