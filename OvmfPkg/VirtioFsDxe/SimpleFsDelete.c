/** @file
  EFI_FILE_PROTOCOL.Delete() member function for the Virtio Filesystem driver.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>             // RemoveEntryList()
#include <Library/MemoryAllocationLib.h> // FreePool()

#include "VirtioFsDxe.h"

EFI_STATUS
EFIAPI
VirtioFsSimpleFileDelete (
  IN EFI_FILE_PROTOCOL *This
  )
{
  VIRTIO_FS_FILE *VirtioFsFile;
  VIRTIO_FS      *VirtioFs;
  EFI_STATUS     Status;

  VirtioFsFile = VIRTIO_FS_FILE_FROM_SIMPLE_FILE (This);
  VirtioFs     = VirtioFsFile->OwnerFs;

  //
  // All actions in this function are "best effort"; the UEFI spec requires
  // EFI_FILE_PROTOCOL.Delete() to release resources unconditionally. If a step
  // related to removing the file fails, it's only reflected in the return
  // status (EFI_WARN_DELETE_FAILURE rather than EFI_SUCCESS).
  //
  // Release, remove, and (if needed) forget. We don't waste time flushing and
  // syncing; if the EFI_FILE_PROTOCOL user cares enough, they should keep the
  // parent directory open until after this function call returns, and then
  // force a sync on *that* EFI_FILE_PROTOCOL instance, using either the
  // Flush() member function, or the Close() member function.
  //
  // If any action fails below, we still try the others.
  //
  VirtioFsFuseReleaseFileOrDir (VirtioFs, VirtioFsFile->NodeId,
    VirtioFsFile->FuseHandle, VirtioFsFile->IsDirectory);

  //
  // VirtioFsFile->FuseHandle is gone at this point, but VirtioFsFile->NodeId
  // is still valid. Continue with removing the file or directory. The result
  // of this operation determines the return status of the function.
  //
  // TODO
  //
  Status = EFI_WARN_DELETE_FAILURE;

  //
  // Finally, if we've known VirtioFsFile->NodeId from a lookup, then we should
  // also ask the server to forget it *once*.
  //
  if (VirtioFsFile->NodeId != VIRTIO_FS_FUSE_ROOT_DIR_NODE_ID) {
    VirtioFsFuseForget (VirtioFs, VirtioFsFile->NodeId);
  }

  //
  // One fewer file left open for the owner filesystem.
  //
  RemoveEntryList (&VirtioFsFile->OpenFilesEntry);

  FreePool (VirtioFsFile->CanonicalPathname);
  FreePool (VirtioFsFile);
  return Status;
}
