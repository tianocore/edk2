#------------------------------------------------------------------------------ 
#
# CopyMem() worker for ARM
#
# This file started out as C code that did 64 bit moves if the buffer was
# 32-bit aligned, else it does a byte copy. It also does a byte copy for
# any trailing bytes. It was updated to do 32-byte copies using stm/ldm. 
#
# Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#------------------------------------------------------------------------------

/**
  Copy Length bytes from Source to Destination. Overlap is OK.

  This implementation 

  @param  Destination Target of copy
  @param  Source      Place to copy from
  @param  Length      Number of bytes to copy

  @return Destination


VOID *
EFIAPI
InternalMemCopyMem (
  OUT     VOID                      *DestinationBuffer,
  IN      CONST VOID                *SourceBuffer,
  IN      UINTN                     Length
  )
**/
.text
.align 2
.globl ASM_PFX(InternalMemCopyMem)
INTERWORK_FUNC(InternalMemCopyMem)

ASM_PFX(InternalMemCopyMem):
	stmfd	sp!, {r4-r11, lr}
	tst	r0, #3
	mov	r11, r0
	mov	r10, r0
	mov	ip, r2
	mov	lr, r1
	movne	r0, #0
	bne	L4
	tst	r1, #3
	movne	r3, #0
	moveq	r3, #1
	cmp	r2, #31
	movls	r0, #0
	andhi	r0, r3, #1
L4:
	cmp	r11, r1
	bcc	L26
	bls	L7
	rsb	r3, r1, r11
	cmp	ip, r3
	bcc	L26
	cmp	ip, #0
	beq	L7
	add	r10, r11, ip
	add	lr, ip, r1
	b	L16
L29:
	sub	ip, ip, #8
	cmp	ip, #7
	ldrd	r2, [lr, #-8]!
	movls	r0, #0
	cmp	ip, #0
	strd	r2, [r10, #-8]!
	beq	L7
L16:
	cmp	r0, #0
	bne	L29
	sub	r3, lr, #1
	sub	ip, ip, #1
	ldrb	r3, [r3, #0]	
	sub	r2, r10, #1
	cmp	ip, #0
	sub	r10, r10, #1
	sub	lr, lr, #1
	strb	r3, [r2, #0]
	bne	L16
	b   L7
L11:
	ldrb	r3, [lr], #1	
	sub	ip, ip, #1
	strb	r3, [r10], #1
L26:
	cmp	ip, #0
	beq	L7
L30:
	cmp	r0, #0
	beq	L11
	sub	ip, ip, #32
	cmp	ip, #31
	ldmia	lr!, {r2-r9}
	movls	r0, #0
	cmp	ip, #0
	stmia	r10!, {r2-r9}
	bne	L30
L7:
  mov	r0, r11
	ldmfd	sp!, {r4-r11, pc}
  
