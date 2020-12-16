/** @file
  Internal macro definitions, type definitions, and function declarations for
  the Virtio Filesystem device driver.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef VIRTIO_FS_DXE_H_
#define VIRTIO_FS_DXE_H_

#include <Base.h>                      // SIGNATURE_64()
#include <Library/DebugLib.h>          // CR()
#include <Protocol/SimpleFileSystem.h> // EFI_SIMPLE_FILE_SYSTEM_PROTOCOL
#include <Protocol/VirtioDevice.h>     // VIRTIO_DEVICE_PROTOCOL

#define VIRTIO_FS_SIG SIGNATURE_64 ('V', 'I', 'R', 'T', 'I', 'O', 'F', 'S')

//
// Main context structure, expressing an EFI_SIMPLE_FILE_SYSTEM_PROTOCOL
// interface on top of the Virtio Filesystem device.
//
typedef struct {
  //
  // Parts of this structure are initialized / torn down in various functions
  // at various call depths. The table to the right should make it easier to
  // track them.
  //
  //                              field         init function       init depth
  //                              -----------   ------------------  ----------
  UINT64                          Signature; // DriverBindingStart  0
  VIRTIO_DEVICE_PROTOCOL          *Virtio;   // DriverBindingStart  0
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL SimpleFs;  // DriverBindingStart  0
} VIRTIO_FS;

#define VIRTIO_FS_FROM_SIMPLE_FS(SimpleFsReference) \
  CR (SimpleFsReference, VIRTIO_FS, SimpleFs, VIRTIO_FS_SIG);

//
// EFI_SIMPLE_FILE_SYSTEM_PROTOCOL member functions for the Virtio Filesystem
// driver.
//

EFI_STATUS
EFIAPI
VirtioFsOpenVolume (
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *This,
  OUT EFI_FILE_PROTOCOL               **Root
  );

#endif // VIRTIO_FS_DXE_H_
