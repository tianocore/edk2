/** @file
  OVMF support for QEMU system firmware flash device: functions specific to the
  runtime DXE driver build.

  Copyright (C) 2015, Red Hat, Inc.
  Copyright (c) 2009 - 2013, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/UefiRuntimeLib.h>
#include <Library/MemEncryptSevLib.h>
#include <Library/VmgExitLib.h>

#include "QemuFlash.h"

VOID
QemuFlashConvertPointers (
  VOID
  )
{
  EfiConvertPointer (0x0, (VOID **) &mFlashBase);
}

VOID
QemuFlashBeforeProbe (
  IN  EFI_PHYSICAL_ADDRESS    BaseAddress,
  IN  UINTN                   FdBlockSize,
  IN  UINTN                   FdBlockCount
  )
{
  //
  // Do nothing
  //
}

/**
  Write to QEMU Flash

  @param[in] Ptr    Pointer to the location to write.
  @param[in] Value  The value to write.

**/
VOID
QemuFlashPtrWrite (
  IN        volatile UINT8    *Ptr,
  IN        UINT8             Value
  )
{
  if (MemEncryptSevEsIsEnabled ()) {
    VmgMmioWrite ((UINT8 *) Ptr, &Value, 1);
  } else {
    *Ptr = Value;
  }
}
