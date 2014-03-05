/** @file

  Copyright (c) 2013-2014, ARM Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __ANDROID_FASTBOOT_APP_H__
#define __ANDROID_FASTBOOT_APP_H__

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

#define BOOTIMG_KERNEL_ARGS_SIZE 512

#define ANDROID_FASTBOOT_VERSION "0.4"

EFI_STATUS
BootAndroidBootImg (
  IN  UINTN    BufferSize,
  IN  VOID    *Buffer
  );

EFI_STATUS
ParseAndroidBootImg (
  IN  VOID    *BootImg,
  OUT VOID   **Kernel,
  OUT UINTN   *KernelSize,
  OUT VOID   **Ramdisk,
  OUT UINTN   *RamdiskSize,
  OUT CHAR8   *KernelArgs
  );

#endif //ifdef __ANDROID_FASTBOOT_APP_H__
