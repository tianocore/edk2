/** @file
  Rewrite the BootOrder NvVar based on QEMU's "bootorder" fw_cfg file --
  include file.

  Copyright (C) 2012-2014, Red Hat, Inc.

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef __QEMU_BOOT_ORDER_LIB_H__
#define __QEMU_BOOT_ORDER_LIB_H__

#include <Uefi/UefiBaseType.h>
#include <Base.h>


/**

  Set the boot order based on configuration retrieved from QEMU.

  Attempt to retrieve the "bootorder" fw_cfg file from QEMU. Translate the
  OpenFirmware device paths therein to UEFI device path fragments. Match the
  translated fragments against BootOptionList, and rewrite the BootOrder NvVar
  so that it corresponds to the order described in fw_cfg.

  @param[in] BootOptionList  A boot option list, created with
                             BdsLibEnumerateAllBootOption ().


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
SetBootOrderFromQemu (
  IN  CONST LIST_ENTRY *BootOptionList
  );


/**
  Calculate the number of seconds we should be showing the FrontPage progress
  bar for.

  @return  The TimeoutDefault argument for PlatformBdsEnterFrontPage().
**/
UINT16
GetFrontPageTimeoutFromQemu (
  VOID
  );

#endif
