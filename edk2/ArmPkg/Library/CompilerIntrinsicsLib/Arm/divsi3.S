#------------------------------------------------------------------------------ 
#
# Copyright (c) 2008-2009 Apple Inc. All rights reserved.
#
# All rights reserved. This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#------------------------------------------------------------------------------

	.text
	.align 2
	.globl ___divsi3
___divsi3:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0
	eor	r3, r0, r0, asr #31
	eor	r2, r1, r1, asr #31
	stmfd	sp!, {r4, r5, r7, lr}
	mov	r5, r0, asr #31
	add	r7, sp, #8
	mov	r4, r1, asr #31
	sub	r0, r3, r0, asr #31
	sub	r1, r2, r1, asr #31
	bl	___udivsi3
	eor	r1, r5, r4
	eor	r0, r0, r1
	rsb	r0, r1, r0
	ldmfd	sp!, {r4, r5, r7, pc}
