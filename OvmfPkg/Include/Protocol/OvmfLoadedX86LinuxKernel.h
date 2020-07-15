/** @file
  Protocol/GUID definition to describe a x86 Linux kernel image loaded
  into memory.

  Note that this protocol is considered internal ABI, and may be change
  structure at any time without regard for backward compatibility.

  Copyright (c) 2020, Arm, Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef OVMF_LOADED_X86_LINUX_KERNEL_H__
#define OVMF_LOADED_X86_LINUX_KERNEL_H__

#define OVMF_LOADED_X86_LINUX_KERNEL_PROTOCOL_GUID \
  {0xa3edc05d, 0xb618, 0x4ff6, {0x95, 0x52, 0x76, 0xd7, 0x88, 0x63, 0x43, 0xc8}}

typedef struct {
  VOID    *SetupBuf;
  VOID    *KernelBuf;
  CHAR8   *CommandLine;
  VOID    *InitrdData;
  UINTN   SetupSize;
  UINTN   KernelInitialSize;
  UINTN   InitrdSize;
  UINTN   CommandLineSize;
} OVMF_LOADED_X86_LINUX_KERNEL;

extern EFI_GUID gOvmfLoadedX86LinuxKernelProtocolGuid;

#endif
