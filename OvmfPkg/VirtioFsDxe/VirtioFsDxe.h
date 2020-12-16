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
// Structure for describing a contiguous buffer, potentially mapped for Virtio
// transfer.
//
typedef struct {
  //
  // The following fields originate from the owner of the buffer.
  //
  VOID  *Buffer;
  UINTN Size;
  //
  // All of the fields below, until the end of the structure, are
  // zero-initialized when the structure is initially validated.
  //
  // Mapped, MappedAddress and Mapping are updated when the buffer is mapped
  // for VirtioOperationBusMasterRead or VirtioOperationBusMasterWrite. They
  // are again updated when the buffer is unmapped.
  //
  BOOLEAN              Mapped;
  EFI_PHYSICAL_ADDRESS MappedAddress;
  VOID                 *Mapping;
  //
  // Transferred is updated after VirtioFlush() returns successfully:
  // - for VirtioOperationBusMasterRead, Transferred is set to Size;
  // - for VirtioOperationBusMasterWrite, Transferred is calculated from the
  //   UsedLen output parameter of VirtioFlush().
  //
  UINTN Transferred;
} VIRTIO_FS_IO_VECTOR;

//
// Structure for describing a list of IO Vectors.
//
typedef struct {
  //
  // The following fields originate from the owner of the buffers.
  //
  VIRTIO_FS_IO_VECTOR *IoVec;
  UINTN               NumVec;
  //
  // TotalSize is calculated when the scatter-gather list is initially
  // validated.
  //
  UINT32 TotalSize;
} VIRTIO_FS_SCATTER_GATHER_LIST;

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

EFI_STATUS
VirtioFsSgListsValidate (
  IN     VIRTIO_FS                     *VirtioFs,
  IN OUT VIRTIO_FS_SCATTER_GATHER_LIST *RequestSgList,
  IN OUT VIRTIO_FS_SCATTER_GATHER_LIST *ResponseSgList OPTIONAL
  );

EFI_STATUS
VirtioFsSgListsSubmit (
  IN OUT VIRTIO_FS                     *VirtioFs,
  IN OUT VIRTIO_FS_SCATTER_GATHER_LIST *RequestSgList,
  IN OUT VIRTIO_FS_SCATTER_GATHER_LIST *ResponseSgList OPTIONAL
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
