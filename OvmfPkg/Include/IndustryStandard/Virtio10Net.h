/** @file
  Definitions from the VirtIo 1.0 specification (csprd05), specifically for the
  network device.

  Copyright (C) 2016, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _VIRTIO_1_0_NET_H_
#define _VIRTIO_1_0_NET_H_

#include <IndustryStandard/Virtio10.h>
#include <IndustryStandard/Virtio095Net.h>

//
// VirtIo 1.0 packet header
//
#pragma pack (1)
typedef struct {
  VIRTIO_NET_REQ    V0_9_5;
  UINT16            NumBuffers;
} VIRTIO_1_0_NET_REQ;
#pragma pack ()

#endif // _VIRTIO_1_0_NET_H_
