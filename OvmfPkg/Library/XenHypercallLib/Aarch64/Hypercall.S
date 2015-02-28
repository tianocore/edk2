
/** @file
  AArch64 implementation of XenHypercall2

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
  mov     x16, x0
  mov     x0, x1
  mov     x1, x2
  hvc     #XEN_HYPERCALL_TAG
  ret
