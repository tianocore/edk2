/** @file
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL.OpenVolume() member function for the Virtio
  Filesystem driver.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

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
  return EFI_NO_MEDIA;
}
