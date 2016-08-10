#------------------------------------------------------------------------------
#
# Copyright (c) 2016, Linaro Limited. All rights reserved.
#
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#------------------------------------------------------------------------------

#include <AsmMacroIoLibV8.h>

  .set CTRL_M_BIT,      (1 << 0)

  .macro __replace_entry, el

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

  // flush the TLBs
  .if   \el == 1
  tlbi  vmalle1
  .else
  tlbi  alle\el
  .endif
  dsb   sy

  // re-enable the MMU
  msr   sctlr_el\el, x8
  isb
  .endm

//VOID
//ArmReplaceLiveTranslationEntry (
//  IN  UINT64  *Entry,
//  IN  UINT64  Value
//  )
ASM_FUNC(ArmReplaceLiveTranslationEntry)

  // disable interrupts
  mrs   x2, daif
  msr   daifset, #0xf
  isb

  // clean and invalidate first so that we don't clobber
  // adjacent entries that are dirty in the caches
  dc    civac, x0
  dsb   ish

  EL1_OR_EL2_OR_EL3(x3)
1:__replace_entry 1
  b     4f
2:__replace_entry 2
  b     4f
3:__replace_entry 3

4:msr   daif, x2
  ret

ASM_GLOBAL ASM_PFX(ArmReplaceLiveTranslationEntrySize)

ASM_PFX(ArmReplaceLiveTranslationEntrySize):
  .long   . - ArmReplaceLiveTranslationEntry
