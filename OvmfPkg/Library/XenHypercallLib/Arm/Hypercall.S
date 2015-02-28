/** @file
  ARM (AArch32) implementation of XenHypercall2

  Copyright (C) 2014, Linaro Ltd.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <IndustryStandard/Xen/arch-arm/xen.h>

  .text
  .global   ASM_PFX(XenHypercall2)
ASM_PFX(XenHypercall2):
  mov     r12, r0
  mov     r0, r1
  mov     r1, r2
  hvc     #XEN_HYPERCALL_TAG
  bx      lr
