/** @file
  XenBus Bus driver declarations.

  Copyright (C) 2014, Citrix Ltd.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _XEN_XENBUS_XENBUSB_H
#define _XEN_XENBUS_XENBUSB_H

#include "XenBusDxe.h"

#define XENBUS_DEVICE_PATH_TYPE_VBD  0x1
struct _XENBUS_DEVICE_PATH {
  VENDOR_DEVICE_PATH    Vendor;
  UINT8                 Type;
  UINT16                DeviceId;
};

/**
  Perform XenBus bus enumeration and install protocol for children.

  Caller should ensure that it is the only one to call this function. This
  function cannot be called concurrently.

  @param Dev   A XENBUS_DEVICE instance.

  @return      On success, XENSTORE_STATUS_SUCCESS. Otherwise an errno value
               indicating the type of failure.
**/
XENSTORE_STATUS
XenBusEnumerateBus (
  XENBUS_DEVICE  *Dev
  );

#endif /* _XEN_XENBUS_XENBUSB_H */
