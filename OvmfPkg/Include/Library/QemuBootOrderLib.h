/** @file
  Rewrite the BootOrder NvVar based on QEMU's "bootorder" fw_cfg file --
  include file.

  Copyright (C) 2012-2014, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __QEMU_BOOT_ORDER_LIB_H__
#define __QEMU_BOOT_ORDER_LIB_H__

#include <Uefi/UefiBaseType.h>
#include <Base.h>

/**
  Connect devices based on the boot order retrieved from QEMU.

  Attempt to retrieve the "bootorder" fw_cfg file from QEMU. Translate the
  OpenFirmware device paths therein to UEFI device path fragments. Connect the
  devices identified by the UEFI devpath prefixes as narrowly as possible, then
  connect all their child devices, recursively.

  If this function fails, then platform BDS should fall back to
  EfiBootManagerConnectAll(), or some other method for connecting any expected
  boot devices.

  @retval RETURN_SUCCESS            The "bootorder" fw_cfg file has been
                                    parsed, and the referenced device-subtrees
                                    have been connected.

  @retval RETURN_UNSUPPORTED        QEMU's fw_cfg is not supported.

  @retval RETURN_NOT_FOUND          Empty or nonexistent "bootorder" fw_cfg
                                    file.

  @retval RETURN_INVALID_PARAMETER  Parse error in the "bootorder" fw_cfg file.

  @retval RETURN_OUT_OF_RESOURCES   Memory allocation failed.

  @return                           Error statuses propagated from underlying
                                    functions.
**/
RETURN_STATUS
EFIAPI
ConnectDevicesFromQemu (
  VOID
  );

/**
  Write qemu boot order to uefi variables.

  Attempt to retrieve the "bootorder" fw_cfg file from QEMU. Translate
  the OpenFirmware device paths therein to UEFI device path fragments.

  On Success store the device path in QemuBootOrderNNNN variables.
**/
VOID
EFIAPI
StoreQemuBootOrder (
  VOID
  );

/**

  Set the boot order based on configuration retrieved from QEMU.

  Attempt to retrieve the "bootorder" fw_cfg file from QEMU. Translate the
  OpenFirmware device paths therein to UEFI device path fragments. Match the
  translated fragments against the current list of boot options, and rewrite
  the BootOrder NvVar so that it corresponds to the order described in fw_cfg.

  Platform BDS should call this function after connecting any expected boot
  devices and calling EfiBootManagerRefreshAllBootOption ().

  @retval RETURN_SUCCESS            BootOrder NvVar rewritten.

  @retval RETURN_UNSUPPORTED        QEMU's fw_cfg is not supported.

  @retval RETURN_NOT_FOUND          Empty or nonexistent "bootorder" fw_cfg
                                    file, or no match found between the
                                    "bootorder" fw_cfg file and BootOptionList.

  @retval RETURN_INVALID_PARAMETER  Parse error in the "bootorder" fw_cfg file.

  @retval RETURN_OUT_OF_RESOURCES   Memory allocation failed.

  @return                           Values returned by gBS->LocateProtocol ()
                                    or gRT->SetVariable ().

**/
RETURN_STATUS
EFIAPI
SetBootOrderFromQemu (
  VOID
  );

/**
  Calculate the number of seconds we should be showing the FrontPage progress
  bar for.

  @return  The TimeoutDefault argument for PlatformBdsEnterFrontPage().
**/
UINT16
EFIAPI
GetFrontPageTimeoutFromQemu (
  VOID
  );

#endif
