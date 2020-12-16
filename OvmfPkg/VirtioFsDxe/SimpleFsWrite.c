/** @file
  EFI_FILE_PROTOCOL.Write() member function for the Virtio Filesystem driver.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "VirtioFsDxe.h"

EFI_STATUS
EFIAPI
VirtioFsSimpleFileWrite (
  IN     EFI_FILE_PROTOCOL *This,
  IN OUT UINTN             *BufferSize,
  IN     VOID              *Buffer
  )
{
  return EFI_NO_MEDIA;
}
