/** @file

  Copyright (c) 2013-2014, ARM Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Library/AndroidBootImgLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

#define BOOTIMG_KERNEL_ARGS_SIZE  512

#define ANDROID_FASTBOOT_VERSION  "0.4"

EFI_STATUS
BootAndroidBootImg (
  IN  UINTN  BufferSize,
  IN  VOID   *Buffer
  );

EFI_STATUS
ParseAndroidBootImg (
  IN  VOID   *BootImg,
  OUT VOID   **Kernel,
  OUT UINTN  *KernelSize,
  OUT VOID   **Ramdisk,
  OUT UINTN  *RamdiskSize,
  OUT CHAR8  *KernelArgs
  );
