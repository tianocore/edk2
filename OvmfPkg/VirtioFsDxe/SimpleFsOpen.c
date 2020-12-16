/** @file
  EFI_FILE_PROTOCOL.Open() member function for the Virtio Filesystem driver.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "VirtioFsDxe.h"

EFI_STATUS
EFIAPI
VirtioFsSimpleFileOpen (
  IN     EFI_FILE_PROTOCOL *This,
     OUT EFI_FILE_PROTOCOL **NewHandle,
  IN     CHAR16            *FileName,
  IN     UINT64            OpenMode,
  IN     UINT64            Attributes
  )
{
  return EFI_NO_MEDIA;
}
