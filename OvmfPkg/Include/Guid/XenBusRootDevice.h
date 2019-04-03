/** @file
  GUID to be used to identify the XenBus root node on non-PCI Xen guests

  Copyright (C) 2015, Linaro Ltd.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __XENBUS_ROOT_DEVICE_H__
#define __XENBUS_ROOT_DEVICE_H__

#define XENBUS_ROOT_DEVICE_GUID \
{0xa732241f, 0x383d, 0x4d9c, {0x8a, 0xe1, 0x8e, 0x09, 0x83, 0x75, 0x89, 0xd7}}

extern EFI_GUID gXenBusRootDeviceGuid;

#endif
