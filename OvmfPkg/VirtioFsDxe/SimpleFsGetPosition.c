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
  IN     EFI_FILE_PROTOCOL *This,
     OUT UINT64            *Position
  )
{
  return EFI_DEVICE_ERROR;
}
