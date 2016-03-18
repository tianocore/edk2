/** @file

  Definitions for the VirtIo MMIO Device Library

  Copyright (C) 2013, ARM Ltd

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _VIRTIO_MMIO_DEVICE_LIB_H_
#define _VIRTIO_MMIO_DEVICE_LIB_H_

/**

  Initialize VirtIo Device and Install VIRTIO_DEVICE_PROTOCOL protocol

  @param[in] BaseAddress  Base Address of the VirtIo MMIO Device

  @param[in] Handle       Handle of the device the driver should be attached
                          to.

  @retval EFI_SUCCESS           The VirtIo Device has been installed
                                successfully.

  @retval EFI_OUT_OF_RESOURCES  The function failed to allocate memory required
                                by the Virtio MMIO device initialization.

  @retval EFI_UNSUPPORTED       BaseAddress does not point to a VirtIo MMIO
                                device.

  @return                       Status code returned by InstallProtocolInterface
                                Boot Service function.

**/
EFI_STATUS
VirtioMmioInstallDevice (
  IN PHYSICAL_ADDRESS       BaseAddress,
  IN EFI_HANDLE             Handle
  );

/**

  Uninstall the VirtIo Device

  @param[in] Handle       Handle of the device where the VirtIo Device protocol
                          should have been installed.

  @retval EFI_SUCCESS     The device has been un-initialized successfully.

  @return                 Status code returned by UninstallProtocolInterface
                          Boot Service function.

**/
EFI_STATUS
VirtioMmioUninstallDevice (
  IN EFI_HANDLE             Handle
  );

#endif // _VIRTIO_MMIO_DEVICE_LIB_H_
