/** @file
  An event group GUID with which BDS indicates that PCI root bridges have been
  connected, and PciIo protocol instances have become available.

  Note that this differs from the PCI Enumeration Complete Protocol as defined
  in the PI 1.1 specification. That protocol is installed by the PCI bus driver
  after enumeration and resource allocation have been completed, but before
  PciIo protocol instances are created.

  Copyright (C) 2016, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _ROOT_BRIDGES_CONNECTED_EVENT_GROUP_H_
#define _ROOT_BRIDGES_CONNECTED_EVENT_GROUP_H_

#define ROOT_BRIDGES_CONNECTED_EVENT_GROUP_GUID         \
  { 0x24a2d66f,                                         \
    0xeedd,                                             \
    0x4086,                                             \
    { 0x90, 0x42, 0xf2, 0x6e, 0x47, 0x97, 0xee, 0x69 }, \
  }

extern EFI_GUID  gRootBridgesConnectedEventGroupGuid;

#endif
