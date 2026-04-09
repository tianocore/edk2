/** @file
  Try to load an EFI-stubbed ARM Linux kernel from QEMU's fw_cfg.

  This implementation differs from OvmfPkg/Library/LoadLinuxLib. An EFI
  stub in the subject kernel is a hard requirement here.

  Copyright (C) 2014-2016, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/QemuLoadImageLib.h>
#include <Library/ReportStatusCodeLib.h>

#include "PlatformBm.h"

//
// The entry point of the feature.
//

/**
  Download the kernel, the initial ramdisk, and the kernel command line from
  QEMU's fw_cfg. Construct a minimal SimpleFileSystem that contains the two
  image files, and load and start the kernel from it.

  The kernel will be instructed via its command line to load the initrd from
  the same Simple FileSystem.

  @retval EFI_NOT_FOUND         Kernel image was not found.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.
  @retval EFI_PROTOCOL_ERROR    Unterminated kernel command line.

  @return                       Error codes from any of the underlying
                                functions. On success, the function doesn't
                                return.
**/
EFI_STATUS
EFIAPI
TryRunningQemuKernel (
  VOID
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  KernelImageHandle;

  Status = QemuLoadKernelImage (&KernelImageHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Signal the EFI_EVENT_GROUP_READY_TO_BOOT event.
  //
  EfiSignalEventReadyToBoot ();

  REPORT_STATUS_CODE (
    EFI_PROGRESS_CODE,
    (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_DXE_BS_PC_READY_TO_BOOT_EVENT)
    );

  //
  // Start the image.
  //
  Status = QemuStartKernelImage (&KernelImageHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: QemuStartKernelImage(): %r\n",
      __func__,
      Status
      ));
  }

  QemuUnloadKernelImage (KernelImageHandle);

  return Status;
}
