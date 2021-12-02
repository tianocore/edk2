/** @file

  Copyright (c) 2017, Linaro. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __ANDROID_BOOTIMG_PROTOCOL_H__
#define __ANDROID_BOOTIMG_PROTOCOL_H__

//
// Protocol interface structure
//
typedef struct _ANDROID_BOOTIMG_PROTOCOL ANDROID_BOOTIMG_PROTOCOL;

//
// Function Prototypes
//
typedef
EFI_STATUS
(EFIAPI *ANDROID_BOOTIMG_APPEND_KERNEL_ARGS)(
  IN CHAR16            *Args,
  IN UINTN              Size
  );

typedef
EFI_STATUS
(EFIAPI *ANDROID_BOOTIMG_UPDATE_DTB)(
  IN  EFI_PHYSICAL_ADDRESS    OrigDtbBase,
  OUT EFI_PHYSICAL_ADDRESS   *NewDtbBase
  );

struct _ANDROID_BOOTIMG_PROTOCOL {
  ANDROID_BOOTIMG_APPEND_KERNEL_ARGS    AppendArgs;
  ANDROID_BOOTIMG_UPDATE_DTB            UpdateDtb;
};

extern EFI_GUID  gAndroidBootImgProtocolGuid;

#endif /* __ANDROID_BOOTIMG_PROTOCOL_H__ */
