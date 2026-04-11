/** @file
*
*  Copyright (c) 2023, Ventana Micro Systems Inc. All Rights Reserved.<BR>
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Base.h>
#include <Register/RiscV64/RiscVImpl.h>

.text
  .align 3

//
// Local tlb flush all.
//
//
ASM_FUNC (RiscVLocalTlbFlushAll)
sfence.vma
ret

//
// Local tlb flush at a virtual address
// @retval a0 : virtual address.
//
ASM_FUNC (
  RiscVLocalTlbFlush
  )
sfence.vma a0
ret
