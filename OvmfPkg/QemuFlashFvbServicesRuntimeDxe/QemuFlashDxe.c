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
#include <Register/Amd/Msr.h>

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
    MSR_SEV_ES_GHCB_REGISTER  Msr;
    GHCB                      *Ghcb;

    Msr.GhcbPhysicalAddress = AsmReadMsr64 (MSR_SEV_ES_GHCB);
    Ghcb = Msr.Ghcb;

    //
    // Writing to flash is emulated by the hypervisor through the use of write
    // protection. This won't work for an SEV-ES guest because the write won't
    // be recognized as a true MMIO write, which would result in the required
    // #VC exception. Instead, use the the VMGEXIT MMIO write support directly
    // to perform the update.
    //
    VmgInit (Ghcb);
    Ghcb->SharedBuffer[0] = Value;
    Ghcb->SaveArea.SwScratch = (UINT64) (UINTN) Ghcb->SharedBuffer;
    VmgExit (Ghcb, SVM_EXIT_MMIO_WRITE, (UINT64) (UINTN) Ptr, 1);
    VmgDone (Ghcb);
  } else {
    *Ptr = Value;
  }
}
