/** @file
  OVMF support for QEMU system firmware flash device: functions specific to the
  runtime DXE driver build.

  Copyright (C) 2015, Red Hat, Inc.
  Copyright (c) 2009 - 2013, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/UefiRuntimeLib.h>

#include "QemuFlash.h"

VOID
QemuFlashConvertPointers (
  VOID
  )
{
  EfiConvertPointer (0x0, (VOID **) &mFlashBase);
}
