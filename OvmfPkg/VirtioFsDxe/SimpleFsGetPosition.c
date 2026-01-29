/** @file
  EFI_FILE_PROTOCOL.GetPosition() member function for the Virtio Filesystem
  driver.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "VirtioFsDxe.h"

EFI_STATUS
EFIAPI
VirtioFsSimpleFileGetPosition (
  IN     EFI_FILE_PROTOCOL  *This,
  OUT UINT64                *Position
  )
{
  VIRTIO_FS_FILE  *VirtioFsFile;

  VirtioFsFile = VIRTIO_FS_FILE_FROM_SIMPLE_FILE (This);
  if (VirtioFsFile->IsDirectory) {
    return EFI_UNSUPPORTED;
  }

  *Position = VirtioFsFile->FilePosition;
  return EFI_SUCCESS;
}
