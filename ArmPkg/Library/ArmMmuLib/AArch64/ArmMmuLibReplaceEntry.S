#------------------------------------------------------------------------------
#
# Copyright (c) 2016, Linaro Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#------------------------------------------------------------------------------

#include <AsmMacroIoLibV8.h>

  .set CTRL_M_BIT,      (1 << 0)

  .macro __replace_entry, el

  // check whether we should disable the MMU
  cbz   x3, .L1_\@

  // clean and invalidate first so that we don't clobber
  // adjacent entries that are dirty in the caches
  dc    civac, x0
  dsb   nsh

  // disable the MMU
  mrs   x8, sctlr_el\el
  bic   x9, x8, #CTRL_M_BIT
  msr   sctlr_el\el, x9
  isb

  // write updated entry
  str   x1, [x0]

  // invalidate again to get rid of stale clean cachelines that may
  // have been filled speculatively since the last invalidate
  dmb   sy
  dc    ivac, x0

  // flush translations for the target address from the TLBs
  lsr   x2, x2, #12
  .if   \el == 1
  tlbi  vaae1, x2
  .else
  tlbi  vae\el, x2
  .endif
  dsb   nsh

  // re-enable the MMU
  msr   sctlr_el\el, x8
  isb
  b     .L2_\@

.L1_\@:
  // write invalid entry
  str   xzr, [x0]
  dsb   nshst

  // flush translations for the target address from the TLBs
  lsr   x2, x2, #12
  .if   \el == 1
  tlbi  vaae1, x2
  .else
  tlbi  vae\el, x2
  .endif
  dsb   nsh

  // write updated entry
  str   x1, [x0]
  dsb   nshst

.L2_\@:
  .endm

  // Align this routine to a log2 upper bound of its size, so that it is
  // guaranteed not to cross a page or block boundary.
  .balign 0x200

//VOID
//ArmReplaceLiveTranslationEntry (
//  IN  UINT64  *Entry,
//  IN  UINT64  Value,
//  IN  UINT64  Address
//  )
ASM_FUNC(ArmReplaceLiveTranslationEntry)

  // disable interrupts
  mrs   x4, daif
  msr   daifset, #0xf
  isb

  EL1_OR_EL2_OR_EL3(x5)
1:__replace_entry 1
  b     4f
2:__replace_entry 2
  b     4f
3:__replace_entry 3

4:msr   daif, x4
  ret

ASM_GLOBAL ASM_PFX(ArmReplaceLiveTranslationEntrySize)

ASM_PFX(ArmReplaceLiveTranslationEntrySize):
  .long   . - ArmReplaceLiveTranslationEntry

  // Double check that we did not overrun the assumed maximum size
  .org    ArmReplaceLiveTranslationEntry + 0x200
