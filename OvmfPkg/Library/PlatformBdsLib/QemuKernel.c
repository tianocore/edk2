/** @file

  Copyright (c) 2006 - 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/LoadLinuxLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/QemuFwCfgLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>


EFI_STATUS
TryRunningQemuKernel (
  VOID
  )
{
  EFI_STATUS                Status;
  UINTN                     KernelSize;
  UINTN                     KernelInitialSize;
  VOID                      *KernelBuf;
  UINTN                     SetupSize;
  VOID                      *SetupBuf;
  UINTN                     CommandLineSize;
  CHAR8                     *CommandLine;
  UINTN                     InitrdSize;
  VOID*                     InitrdData;

  SetupBuf = NULL;
  SetupSize = 0;
  KernelBuf = NULL;
  KernelInitialSize = 0;
  CommandLine = NULL;
  CommandLineSize = 0;
  InitrdData = NULL;
  InitrdSize = 0;

  if (!QemuFwCfgIsAvailable ()) {
    return EFI_NOT_FOUND;
  }

  QemuFwCfgSelectItem (QemuFwCfgItemKernelSize);
  KernelSize = (UINTN) QemuFwCfgRead64 ();

  QemuFwCfgSelectItem (QemuFwCfgItemKernelSetupSize);
  SetupSize = (UINTN) QemuFwCfgRead64 ();

  if (KernelSize == 0 || SetupSize == 0) {
    DEBUG ((EFI_D_INFO, "qemu -kernel was not used.\n"));
    return EFI_NOT_FOUND;
  }

  SetupBuf = LoadLinuxAllocateKernelSetupPages (EFI_SIZE_TO_PAGES (SetupSize));
  if (SetupBuf == NULL) {
    DEBUG ((EFI_D_ERROR, "Unable to allocate memory for kernel setup!\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  DEBUG ((EFI_D_INFO, "Setup size: 0x%x\n", (UINT32) SetupSize));
  DEBUG ((EFI_D_INFO, "Reading kernel setup image ..."));
  QemuFwCfgSelectItem (QemuFwCfgItemKernelSetupData);
  QemuFwCfgReadBytes (SetupSize, SetupBuf);
  DEBUG ((EFI_D_INFO, " [done]\n"));

  Status = LoadLinuxCheckKernelSetup (SetupBuf, SetupSize);
  if (EFI_ERROR (Status)) {
    goto FreeAndReturn;
  }

  Status = LoadLinuxInitializeKernelSetup (SetupBuf);
  if (EFI_ERROR (Status)) {
    goto FreeAndReturn;
  }

  KernelInitialSize = LoadLinuxGetKernelSize (SetupBuf, KernelSize);
  if (KernelInitialSize == 0) {
    Status = EFI_UNSUPPORTED;
    goto FreeAndReturn;
  }

  KernelBuf = LoadLinuxAllocateKernelPages (
                SetupBuf,
                EFI_SIZE_TO_PAGES (KernelInitialSize));
  if (KernelBuf == NULL) {
    DEBUG ((EFI_D_ERROR, "Unable to allocate memory for kernel!\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto FreeAndReturn;
  }

  DEBUG ((EFI_D_INFO, "Kernel size: 0x%x\n", (UINT32) KernelSize));
  DEBUG ((EFI_D_INFO, "Reading kernel image ..."));
  QemuFwCfgSelectItem (QemuFwCfgItemKernelData);
  QemuFwCfgReadBytes (KernelSize, KernelBuf);
  DEBUG ((EFI_D_INFO, " [done]\n"));

  QemuFwCfgSelectItem (QemuFwCfgItemCommandLineSize);
  CommandLineSize = (UINTN) QemuFwCfgRead64 ();

  if (CommandLineSize > 0) {
    CommandLine = LoadLinuxAllocateCommandLinePages (
                    EFI_SIZE_TO_PAGES (CommandLineSize));
    QemuFwCfgSelectItem (QemuFwCfgItemCommandLineData);
    QemuFwCfgReadBytes (CommandLineSize, CommandLine);
  } else {
    CommandLine = NULL;
  }

  Status = LoadLinuxSetCommandLine (SetupBuf, CommandLine);
  if (EFI_ERROR (Status)) {
    goto FreeAndReturn;
  }

  QemuFwCfgSelectItem (QemuFwCfgItemInitrdSize);
  InitrdSize = (UINTN) QemuFwCfgRead64 ();

  if (InitrdSize > 0) {
    InitrdData = LoadLinuxAllocateInitrdPages (
                   SetupBuf,
                   EFI_SIZE_TO_PAGES (InitrdSize)
                   );
    DEBUG ((EFI_D_INFO, "Initrd size: 0x%x\n", (UINT32) InitrdSize));
    DEBUG ((EFI_D_INFO, "Reading initrd image ..."));
    QemuFwCfgSelectItem (QemuFwCfgItemInitrdData);
    QemuFwCfgReadBytes (InitrdSize, InitrdData);
    DEBUG ((EFI_D_INFO, " [done]\n"));
  } else {
    InitrdData = NULL;
  }

  Status = LoadLinuxSetInitrd (SetupBuf, InitrdData, InitrdSize);
  if (EFI_ERROR (Status)) {
    goto FreeAndReturn;
  }

  //
  // Signal the EVT_SIGNAL_READY_TO_BOOT event
  //
  EfiSignalEventReadyToBoot();

  Status = LoadLinux (KernelBuf, SetupBuf);

FreeAndReturn:
  if (SetupBuf != NULL) {
    FreePages (SetupBuf, EFI_SIZE_TO_PAGES (SetupSize));
  }
  if (KernelBuf != NULL) {
    FreePages (KernelBuf, EFI_SIZE_TO_PAGES (KernelInitialSize));
  }
  if (CommandLine != NULL) {
    FreePages (CommandLine, EFI_SIZE_TO_PAGES (CommandLineSize));
  }
  if (InitrdData != NULL) {
    FreePages (InitrdData, EFI_SIZE_TO_PAGES (InitrdSize));
  }

  return Status;
}

