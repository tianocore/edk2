/** @file
  XenIo protocol to abstract arch specific details

  The Xen implementations for the Intel and ARM architectures differ in the way
  the base address of the grant table is communicated to the guest. The former
  uses a virtual PCI device, while the latter uses a device tree node.
  In order to allow the XenBusDxe UEFI driver to be reused for the non-PCI
  Xen implementation, this abstract protocol can be installed on a handle
  with the appropriate base address.

  Copyright (C) 2014, Linaro Ltd.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PROTOCOL_XENIO_H__
#define __PROTOCOL_XENIO_H__

#include <IndustryStandard/Xen/xen.h>

#define XENIO_PROTOCOL_GUID \
  {0x6efac84f, 0x0ab0, 0x4747, {0x81, 0xbe, 0x85, 0x55, 0x62, 0x59, 0x04, 0x49}}

///
/// Forward declaration
///
typedef struct _XENIO_PROTOCOL XENIO_PROTOCOL;

///
/// Protocol structure
///
struct _XENIO_PROTOCOL {
  //
  // Protocol data fields
  //
  EFI_PHYSICAL_ADDRESS    GrantTableAddress;
};

extern EFI_GUID  gXenIoProtocolGuid;

#endif
