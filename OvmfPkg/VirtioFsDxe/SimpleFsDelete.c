/** @file
  EFI_FILE_PROTOCOL.Delete() member function for the Virtio Filesystem driver.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "VirtioFsDxe.h"

EFI_STATUS
EFIAPI
VirtioFsSimpleFileDelete (
  IN EFI_FILE_PROTOCOL *This
  )
{
  //
  // At this point, the implementation is only suitable for closing the
  // VIRTIO_FS_FILE that was created by VirtioFsOpenVolume().
  //
  // Actually deleting the root directory is not possible, so we're only going
  // to release resources, and return EFI_WARN_DELETE_FAILURE.
  //
  // In order to release resources, VirtioFsSimpleFileClose() is just right
  // here.
  //
  VirtioFsSimpleFileClose (This);
  return EFI_WARN_DELETE_FAILURE;
}
