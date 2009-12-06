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
	.globl ___umodsi3
___umodsi3:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0
	stmfd	sp!, {r4, r5, r7, lr}
	add	r7, sp, #8
	mov	r5, r0
	mov	r4, r1
	bl	L___udivsi3$stub
	mul	r0, r4, r0
	rsb	r0, r0, r5
	ldmfd	sp!, {r4, r5, r7, pc}
	.section __TEXT,__symbol_stub4,symbol_stubs,none,12
	.align 2
L___udivsi3$stub:
	.indirect_symbol ___udivsi3
	ldr	ip, L___udivsi3$slp
	ldr	pc, [ip, #0]
L___udivsi3$slp:
	.long	L___udivsi3$lazy_ptr
	.lazy_symbol_pointer
L___udivsi3$lazy_ptr:
	.indirect_symbol	___udivsi3
	.long	dyld_stub_binding_helper
