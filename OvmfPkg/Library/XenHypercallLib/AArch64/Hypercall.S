
/** @file
  AArch64 implementation of XenHypercall2

  Copyright (C) 2014, Linaro Ltd.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/Xen/arch-arm/xen.h>

  .text
  .global   ASM_PFX(XenHypercall2)
ASM_PFX(XenHypercall2):
  mov     x16, x0
  mov     x0, x1
  mov     x1, x2
  hvc     #XEN_HYPERCALL_TAG
  ret
