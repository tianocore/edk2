/** @file
  Protocol/GUID definition to describe a kernel image loaded by the legacy X86
  loader from the file specified on the QEMU command line via the -kernel
  option.

  Copyright (c) 2020, Arm, Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef X86_QEMU_KERNEL_LOADED_IMAGE_GUID_H__
#define X86_QEMU_KERNEL_LOADED_IMAGE_GUID_H__

#define X86_QEMU_KERNEL_LOADED_IMAGE_GUID \
  {0xa3edc05d, 0xb618, 0x4ff6, {0x95, 0x52, 0x76, 0xd7, 0x88, 0x63, 0x43, 0xc8}}

extern EFI_GUID gX86QemuKernelLoadedImageGuid;

#endif
