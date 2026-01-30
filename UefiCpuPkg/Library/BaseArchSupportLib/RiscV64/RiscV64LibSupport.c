/** @file
  UefiCpu RISC-V architectures support library for RISCV64.

  Copyright (c) Xiang W <wangxiang@iscas.ac.cn>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>
#include <Library/BaseLib.h>

UINT8
ArchGetPhysicalAddressBits (
  VOID
  )
{
  return 56;
}
