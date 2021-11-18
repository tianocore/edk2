/** @file
  GUID definition for the QEMU LoaderFs media device path, containing the
  kernel, initrd and command line as file objects

  Copyright (c) 2020, Arm, Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef QEMU_KERNEL_LOADER_FS_MEDIA_GUID_H__
#define QEMU_KERNEL_LOADER_FS_MEDIA_GUID_H__

#define QEMU_KERNEL_LOADER_FS_MEDIA_GUID \
  {0x1428f772, 0xb64a, 0x441e, {0xb8, 0xc3, 0x9e, 0xbd, 0xd7, 0xf8, 0x93, 0xc7}}

extern EFI_GUID  gQemuKernelLoaderFsMediaGuid;

#endif
