/** @file
  ARM (AArch32) implementation of XenHypercall2

  Copyright (C) 2014, Linaro Ltd.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/Xen/arch-arm/xen.h>

  .text
  .arch_extension virt

GCC_ASM_EXPORT(XenHypercall2)

ASM_PFX(XenHypercall2):
  mov     r12, r0
  mov     r0, r1
  mov     r1, r2
  hvc     #XEN_HYPERCALL_TAG
  bx      lr
