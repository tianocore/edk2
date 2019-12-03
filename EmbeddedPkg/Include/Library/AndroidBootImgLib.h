/** @file

  Copyright (c) 2013-2014, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2017, Linaro.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __ABOOTIMG_H__
#define __ABOOTIMG_H__

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiSpec.h>

#define ANDROID_BOOTIMG_KERNEL_ARGS_SIZE  512

#define ANDROID_BOOT_MAGIC                "ANDROID!"
#define ANDROID_BOOT_MAGIC_LENGTH         (sizeof (ANDROID_BOOT_MAGIC) - 1)

// No documentation for this really - sizes of fields has been determined
// empirically.
#pragma pack(1)
/* https://android.googlesource.com/platform/system/core/+/master/mkbootimg/bootimg.h */
typedef struct {
  UINT8   BootMagic[ANDROID_BOOT_MAGIC_LENGTH];
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
  CHAR8   KernelArgs[ANDROID_BOOTIMG_KERNEL_ARGS_SIZE];
  UINT32  Id[32];
} ANDROID_BOOTIMG_HEADER;
#pragma pack ()

/* Check Val (unsigned) is a power of 2 (has only one bit set) */
#define IS_POWER_OF_2(Val)       ((Val) != 0 && (((Val) & ((Val) - 1)) == 0))
/* Android boot image page size is not specified, but it should be power of 2
 * and larger than boot header */
#define IS_VALID_ANDROID_PAGE_SIZE(Val)   \
             (IS_POWER_OF_2(Val) && (Val > sizeof(ANDROID_BOOTIMG_HEADER)))

EFI_STATUS
AndroidBootImgGetImgSize (
  IN  VOID    *BootImg,
  OUT UINTN   *ImgSize
  );

EFI_STATUS
AndroidBootImgBoot (
  IN VOID                   *Buffer,
  IN UINTN                   BufferSize
  );

#endif /* __ABOOTIMG_H__ */
