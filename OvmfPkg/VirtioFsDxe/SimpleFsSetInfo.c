/** @file
  EFI_FILE_PROTOCOL.SetInfo() member function for the Virtio Filesystem driver.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "VirtioFsDxe.h"

EFI_STATUS
EFIAPI
VirtioFsSimpleFileSetInfo (
  IN EFI_FILE_PROTOCOL *This,
  IN EFI_GUID          *InformationType,
  IN UINTN             BufferSize,
  IN VOID              *Buffer
  )
{
  return EFI_NO_MEDIA;
}
