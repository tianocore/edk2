/** @file
  EFI_FILE_PROTOCOL.Flush() member function for the Virtio Filesystem driver.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "VirtioFsDxe.h"

EFI_STATUS
EFIAPI
VirtioFsSimpleFileFlush (
  IN EFI_FILE_PROTOCOL *This
  )
{
  return EFI_NO_MEDIA;
}
