/** @file

  Copyright (c) 2013-2014, ARM Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "AndroidFastbootApp.h"

// Find the kernel and ramdisk in an Android boot.img.
// return EFI_INVALID_PARAMETER if the boot.img is invalid (i.e. doesn't have the
//  right magic value),
// return EFI_NOT_FOUND if there was no kernel in the boot.img.
// Note that the Ramdisk is optional - *Ramdisk won't be touched if it isn't
// present, but RamdiskSize will be set to 0.
EFI_STATUS
ParseAndroidBootImg (
  IN  VOID    *BootImg,
  OUT VOID   **Kernel,
  OUT UINTN   *KernelSize,
  OUT VOID   **Ramdisk,
  OUT UINTN   *RamdiskSize,
  OUT CHAR8   *KernelArgs
  )
{
  ANDROID_BOOTIMG_HEADER   *Header;
  UINT8                    *BootImgBytePtr;

  // Cast to UINT8 so we can do pointer arithmetic
  BootImgBytePtr = (UINT8 *) BootImg;

  Header = (ANDROID_BOOTIMG_HEADER *) BootImg;

  if (AsciiStrnCmp ((CONST CHAR8 *)Header->BootMagic, ANDROID_BOOT_MAGIC,
                    ANDROID_BOOT_MAGIC_LENGTH) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  if (Header->KernelSize == 0) {
    return EFI_NOT_FOUND;
  }

  ASSERT (IS_VALID_ANDROID_PAGE_SIZE (Header->PageSize));

  *KernelSize = Header->KernelSize;
  *Kernel = BootImgBytePtr + Header->PageSize;
  *RamdiskSize = Header->RamdiskSize;

  if (Header->RamdiskSize != 0) {
    *Ramdisk = (VOID *) (BootImgBytePtr
                 + Header->PageSize
                 + ALIGN_VALUE (Header->KernelSize, Header->PageSize));
  }

  AsciiStrnCpyS (KernelArgs, ANDROID_BOOTIMG_KERNEL_ARGS_SIZE, Header->KernelArgs,
    ANDROID_BOOTIMG_KERNEL_ARGS_SIZE);

  return EFI_SUCCESS;
}
