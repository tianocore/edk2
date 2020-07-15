/** @file
  Recommended GUID to be used in the Vendor Hardware device path nodes that
  identify virtio-mmio transports.

  Copyright (C) 2014, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __VIRTIO_MMIO_TRANSPORT_H__
#define __VIRTIO_MMIO_TRANSPORT_H__

#define VIRTIO_MMIO_TRANSPORT_GUID \
{0x837dca9e, 0xe874, 0x4d82, {0xb2, 0x9a, 0x23, 0xfe, 0x0e, 0x23, 0xd1, 0xe2}}

extern EFI_GUID gVirtioMmioTransportGuid;

#endif
