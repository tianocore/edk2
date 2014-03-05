/** @file

  Copyright (c) 2013-2014, ARM Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "AndroidFastbootApp.h"

#define BOOT_MAGIC        "ANDROID!"
#define BOOT_MAGIC_LENGTH sizeof (BOOT_MAGIC) - 1

// Check Val (unsigned) is a power of 2 (has only one bit set)
#define IS_POWER_OF_2(Val) (Val != 0 && ((Val & (Val - 1)) == 0))

// No documentation for this really - sizes of fields has been determined
// empirically.
#pragma pack(1)
typedef struct {
  CHAR8   BootMagic[BOOT_MAGIC_LENGTH];
  UINT32  KernelSize;
  UINT32  KernelAddress;
  UINT32  RamdiskSize;
  UINT32  RamdiskAddress;
  UINT32  SecondStageBootloaderSize;
  UINT32  SecondStageBootloaderAddress;
  UINT32  KernelTaggsAddress;
  UINT32  PageSize;
  UINT32  Reserved[2];
  CHAR8   ProductName[16];
  CHAR8   KernelArgs[BOOTIMG_KERNEL_ARGS_SIZE];
  UINT32  Id[32];
} ANDROID_BOOTIMG_HEADER;
#pragma pack()

// Find the kernel and ramdisk in an Android boot.img.
// return EFI_INVALID_PARAMTER if the boot.img is invalid (i.e. doesn't have the
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

  if (AsciiStrnCmp (Header->BootMagic, BOOT_MAGIC, BOOT_MAGIC_LENGTH) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  if (Header->KernelSize == 0) {
    return EFI_NOT_FOUND;
  }

  ASSERT (IS_POWER_OF_2 (Header->PageSize));

  *KernelSize = Header->KernelSize;
  *Kernel = BootImgBytePtr + Header->PageSize;
  *RamdiskSize = Header->RamdiskSize;

  if (Header->RamdiskSize != 0) {
    *Ramdisk = (VOID *) (BootImgBytePtr
                 + Header->PageSize
                 + ALIGN_VALUE (Header->KernelSize, Header->PageSize));
  }

  AsciiStrnCpy (KernelArgs, Header->KernelArgs, BOOTIMG_KERNEL_ARGS_SIZE);

  return EFI_SUCCESS;
}
