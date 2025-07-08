/** @file

  Copyright (c) 2025, The Android Open Source Project.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

/*
  GBL EFI Image Loading Protocol.
  Supplies callers with platform-reserved or size-hinted buffers for images.

  Related docs:
  https://cs.android.com/android/kernel/superproject/+/common-android-mainline:bootable/libbootloader/gbl/docs/GBL_EFI_IMAGE_LOADING_PROTOCOL.md
*/

#ifndef GBL_EFI_IMAGE_LOADING_PROTOCOL_H_
#define GBL_EFI_IMAGE_LOADING_PROTOCOL_H_

#include <Uefi/UefiBaseType.h>

//
// {db84b4fa-53bd-4436-98a7-4e0271428ba8}
//
#define GBL_EFI_IMAGE_LOADING_PROTOCOL_GUID \
  { 0xdb84b4fa, 0x53bd, 0x4436, { 0x98, 0xa7, 0x4e, 0x02, 0x71, 0x42, 0x8b, 0xa8 } }

#define GBL_EFI_IMAGE_LOADING_PROTOCOL_REVISION  0x00000000

typedef struct _GBL_EFI_IMAGE_LOADING_PROTOCOL  GBL_EFI_IMAGE_LOADING_PROTOCOL;
typedef struct _GBL_EFI_IMAGE_INFO              GBL_EFI_IMAGE_INFO;
typedef struct _GBL_EFI_IMAGE_BUFFER            GBL_EFI_IMAGE_BUFFER;

#define GBL_EFI_IMAGE_LOADING_PARTITION_NAME_LEN_U16  36

//
// GBL-reserved image-type strings
//
#define GBL_IMAGE_TYPE_OS_LOAD     L"os_load"    // OS images load/verify buffer
#define GBL_IMAGE_TYPE_FASTBOOT    L"fastboot"   // Fastboot download buffer
#define GBL_IMAGE_TYPE_PVMFW_DATA  L"pvmfw_data" // pvmfw blob & config (4 KiB-aligned)

/*
  Caller’s buffer request.
*/
struct _GBL_EFI_IMAGE_INFO {
  CHAR16    ImageType[GBL_EFI_IMAGE_LOADING_PARTITION_NAME_LEN_U16];
  UINTN     SizeBytes; // caller hint or 0
};

/*
  Buffer returned by firmware.
*/
struct _GBL_EFI_IMAGE_BUFFER {
  VOID     *Memory;    // reserved region or NULL if caller allocates
  UINTN    SizeBytes;  // size of reserved region or bytes to allocate
};

/// Query buffer for a given image type.
typedef
EFI_STATUS
(EFIAPI *GBL_EFI_GET_IMAGE_BUFFER)(
  IN  GBL_EFI_IMAGE_LOADING_PROTOCOL  *This,
  IN  CONST GBL_EFI_IMAGE_INFO        *ImageInfo,
  OUT GBL_EFI_IMAGE_BUFFER            *Buffer
  );

/*
  Firmware-published protocol instance.
*/
struct _GBL_EFI_IMAGE_LOADING_PROTOCOL {
  UINT64                      Revision;
  GBL_EFI_GET_IMAGE_BUFFER    GetBuffer;
};

#endif // GBL_EFI_IMAGE_LOADING_PROTOCOL_H_
