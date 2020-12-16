/** @file
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL.OpenVolume() member function for the Virtio
  Filesystem driver.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>             // InsertTailList()
#include <Library/MemoryAllocationLib.h> // AllocatePool()

#include "VirtioFsDxe.h"

/**
  Open the root directory on the Virtio Filesystem.

  Refer to EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_OPEN_VOLUME for the interface
  contract.
**/
EFI_STATUS
EFIAPI
VirtioFsOpenVolume (
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *This,
  OUT EFI_FILE_PROTOCOL               **Root
  )
{
  VIRTIO_FS      *VirtioFs;
  VIRTIO_FS_FILE *VirtioFsFile;
  EFI_STATUS     Status;
  UINT64         RootDirHandle;

  VirtioFs = VIRTIO_FS_FROM_SIMPLE_FS (This);

  VirtioFsFile = AllocatePool (sizeof *VirtioFsFile);
  if (VirtioFsFile == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Open the root directory.
  //
  Status = VirtioFsFuseOpenDir (VirtioFs, VIRTIO_FS_FUSE_ROOT_DIR_NODE_ID,
             &RootDirHandle);
  if (EFI_ERROR (Status)) {
    goto FreeVirtioFsFile;
  }

  //
  // Populate the new VIRTIO_FS_FILE object.
  //
  VirtioFsFile->Signature              = VIRTIO_FS_FILE_SIG;
  VirtioFsFile->SimpleFile.Revision    = EFI_FILE_PROTOCOL_REVISION;
  VirtioFsFile->SimpleFile.Open        = VirtioFsSimpleFileOpen;
  VirtioFsFile->SimpleFile.Close       = VirtioFsSimpleFileClose;
  VirtioFsFile->SimpleFile.Delete      = VirtioFsSimpleFileDelete;
  VirtioFsFile->SimpleFile.Read        = VirtioFsSimpleFileRead;
  VirtioFsFile->SimpleFile.Write       = VirtioFsSimpleFileWrite;
  VirtioFsFile->SimpleFile.GetPosition = VirtioFsSimpleFileGetPosition;
  VirtioFsFile->SimpleFile.SetPosition = VirtioFsSimpleFileSetPosition;
  VirtioFsFile->SimpleFile.GetInfo     = VirtioFsSimpleFileGetInfo;
  VirtioFsFile->SimpleFile.SetInfo     = VirtioFsSimpleFileSetInfo;
  VirtioFsFile->SimpleFile.Flush       = VirtioFsSimpleFileFlush;
  VirtioFsFile->IsDirectory            = TRUE;
  VirtioFsFile->OwnerFs                = VirtioFs;
  VirtioFsFile->NodeId                 = VIRTIO_FS_FUSE_ROOT_DIR_NODE_ID;
  VirtioFsFile->FuseHandle             = RootDirHandle;

  //
  // One more file open for the filesystem.
  //
  InsertTailList (&VirtioFs->OpenFiles, &VirtioFsFile->OpenFilesEntry);

  *Root = &VirtioFsFile->SimpleFile;
  return EFI_SUCCESS;

FreeVirtioFsFile:
  FreePool (VirtioFsFile);

  return Status;
}
