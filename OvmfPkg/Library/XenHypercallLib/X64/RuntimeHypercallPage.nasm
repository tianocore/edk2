;; @file
;  Provide one page of code space to be overwritten at boot and to be used by
;  runtime drivers to make Xen hypercall on x86.
;
;  Copyright (c) 2022, Citrix Systems, Inc.
;
;  SPDX-License-Identifier: BSD-2-Clause-Patent
;;

DEFAULT REL
SECTION .text

;
; Align at page boundary as we need a pointer on a page without offset.
;
ALIGN EFI_PAGE_SIZE

;
; reserve some .text space to put-in Xen's hypercall instructions in at runtime.
; Poisoned with `ret`
;
global ASM_PFX(XenHypercallPage)
ASM_PFX(XenHypercallPage):
  times EFI_PAGE_SIZE ret
