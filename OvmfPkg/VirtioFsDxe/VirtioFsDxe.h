/** @file
  Internal macro definitions, type definitions, and function declarations for
  the Virtio Filesystem device driver.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef VIRTIO_FS_DXE_H_
#define VIRTIO_FS_DXE_H_

#include <Base.h>                      // SIGNATURE_64()
#include <IndustryStandard/VirtioFs.h> // VIRTIO_FS_TAG_BYTES
#include <Library/DebugLib.h>          // CR()
#include <Protocol/SimpleFileSystem.h> // EFI_SIMPLE_FILE_SYSTEM_PROTOCOL
#include <Protocol/VirtioDevice.h>     // VIRTIO_DEVICE_PROTOCOL
#include <Uefi/UefiBaseType.h>         // EFI_EVENT

#define VIRTIO_FS_SIG SIGNATURE_64 ('V', 'I', 'R', 'T', 'I', 'O', 'F', 'S')

//
// Filesystem label encoded in UCS-2, transformed from the UTF-8 representation
// in "VIRTIO_FS_CONFIG.Tag", and NUL-terminated. Only the printable ASCII code
// points (U+0020 through U+007E) are supported.
//
typedef CHAR16 VIRTIO_FS_LABEL[VIRTIO_FS_TAG_BYTES + 1];

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
  VIRTIO_FS_LABEL                 Label;     // VirtioFsInit        1
  UINT16                          QueueSize; // VirtioFsInit        1
  VRING                           Ring;      // VirtioRingInit      2
  VOID                            *RingMap;  // VirtioRingMap       2
  EFI_EVENT                       ExitBoot;  // DriverBindingStart  0
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL SimpleFs;  // DriverBindingStart  0
} VIRTIO_FS;

#define VIRTIO_FS_FROM_SIMPLE_FS(SimpleFsReference) \
  CR (SimpleFsReference, VIRTIO_FS, SimpleFs, VIRTIO_FS_SIG);

//
// Initialization and helper routines for the Virtio Filesystem device.
//

EFI_STATUS
VirtioFsInit (
  IN OUT VIRTIO_FS *VirtioFs
  );

VOID
VirtioFsUninit (
  IN OUT VIRTIO_FS *VirtioFs
  );

VOID
EFIAPI
VirtioFsExitBoot (
  IN EFI_EVENT ExitBootEvent,
  IN VOID      *VirtioFsAsVoid
  );

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
