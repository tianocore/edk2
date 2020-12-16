/** @file
  Type and macro definitions specific to the Virtio Filesystem device.

  At the time of this writing, the latest released Virtio specification (v1.1)
  does not include the virtio-fs device. The development version of the
  specification defines it however; see the latest version at
  <https://github.com/oasis-tcs/virtio-spec/blob/87fa6b5d8155/virtio-fs.tex>.

  This header file is minimal, and only defines the types and macros that are
  necessary for the OvmfPkg implementation.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef VIRTIO_FS_H_
#define VIRTIO_FS_H_

#include <IndustryStandard/Virtio.h>

//
// Lowest numbered queue for sending normal priority requests.
//
#define VIRTIO_FS_REQUEST_QUEUE 1

//
// Number of bytes in the "VIRTIO_FS_CONFIG.Tag" field.
//
#define VIRTIO_FS_TAG_BYTES 36

//
// Device configuration layout.
//
#pragma pack (1)
typedef struct {
  //
  // The Tag field can be considered the filesystem label, or a mount point
  // hint. It is UTF-8 encoded, and padded to full size with NUL bytes. If the
  // encoded bytes take up the entire Tag field, then there is no NUL
  // terminator.
  //
  UINT8 Tag[VIRTIO_FS_TAG_BYTES];
  //
  // The total number of request virtqueues exposed by the device (i.e.,
  // excluding the "hiprio" queue).
  //
  UINT32 NumReqQueues;
} VIRTIO_FS_CONFIG;
#pragma pack ()

#endif // VIRTIO_FS_H_
