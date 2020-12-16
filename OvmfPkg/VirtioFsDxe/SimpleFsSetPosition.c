/** @file
  EFI_FILE_PROTOCOL.SetPosition() member function for the Virtio Filesystem
  driver.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "VirtioFsDxe.h"

EFI_STATUS
EFIAPI
VirtioFsSimpleFileSetPosition (
  IN EFI_FILE_PROTOCOL *This,
  IN UINT64            Position
  )
{
  return EFI_DEVICE_ERROR;
}
