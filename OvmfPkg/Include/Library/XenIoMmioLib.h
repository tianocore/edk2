/** @file
*  Manage XenBus device path and I/O handles
*
*  Copyright (c) 2015, Linaro Ltd. All rights reserved.<BR>
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#ifndef _XENIO_MMIO_DEVICE_LIB_H_
#define _XENIO_MMIO_DEVICE_LIB_H_

/**

  Install the XENBUS_ROOT_DEVICE_PATH and XENIO_PROTOCOL protocols on
  the handle pointed to by @Handle, or on a new handle if it points to
  NULL

  @param  Handle                Pointer to the handle to install the protocols
                                on, may point to a NULL handle.

  @param  GrantTableAddress     The address of the Xen grant table

  @retval EFI_SUCCESS           Protocols were installed successfully

  @retval EFI_OUT_OF_RESOURCES  The function failed to allocate memory required
                                by the XenIo MMIO and device path protocols

  @return                       Status code returned by the boot service
                                InstallMultipleProtocolInterfaces ()

**/
EFI_STATUS
XenIoMmioInstall (
  IN OUT   EFI_HANDLE              *Handle,
  IN       EFI_PHYSICAL_ADDRESS    GrantTableAddress
  );


/**

  Uninstall the XENBUS_ROOT_DEVICE_PATH and XENIO_PROTOCOL protocols

  @param  Handle          Handle onto which the protocols have been installed
                          earlier by XenIoMmioInstall ()

  @retval EFI_SUCCESS     Protocols were uninstalled successfully

  @return                 Status code returned by the boot service
                          UninstallMultipleProtocolInterfaces ()

**/
EFI_STATUS
XenIoMmioUninstall (
  IN       EFI_HANDLE              Handle
  );

#endif
