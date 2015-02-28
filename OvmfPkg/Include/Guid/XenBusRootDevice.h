/** @file
  GUID to be used to identify the XenBus root node on non-PCI Xen guests

  Copyright (C) 2015, Linaro Ltd.

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License that accompanies this
  distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __XENBUS_ROOT_DEVICE_H__
#define __XENBUS_ROOT_DEVICE_H__

#define XENBUS_ROOT_DEVICE_GUID \
{0xa732241f, 0x383d, 0x4d9c, {0x8a, 0xe1, 0x8e, 0x09, 0x83, 0x75, 0x89, 0xd7}}

extern EFI_GUID gXenBusRootDeviceGuid;

#endif
