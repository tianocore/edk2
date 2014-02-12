/*
 * boot.S - simple register setup code for junping to a second stage bootloader
 *
 * Copyright (C) 2011-2013 ARM Limited.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of ARM nor the names of its contributors may be
 *       used to endorse or promote products derived from this software
 *       without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This license can also be found in the LICENSE.TXT file.
 */

	.text

	.globl	_start

.set LED_ADDR, 0x1c010008

_start:
	/*
	 * EL3 initialisation
	 */

	// Set LED to show progress.
	ldr	x1, =LED_ADDR
	mov	w0, #0x1
	str	w0, [x1]
        dsb	sy

#if defined START_EL2

	mrs	x0, CurrentEL
	cmp	x0, #0xc			// EL3?
	b.ne	start_ns			// skip EL3 initialisation

	mov	x0, #0x30			// RES1
	orr	x0, x0, #(1 << 0)		// Non-secure bit
	orr	x0, x0, #(1 << 8)		// HVC enable
	orr	x0, x0, #(1 << 10)		// 64-bit EL2
	msr	scr_el3, x0

	msr	cptr_el3, xzr			// Disable copro. traps to EL3

	ldr	x0, =CNTFRQ
	msr	cntfrq_el0, x0

	/*
	 * Check for the primary CPU to avoid a race on the distributor
	 * registers.
	 */
	mrs	x0, mpidr_el1
	tst	x0, #15
	b.ne	1f				// secondary CPU

	ldr	x1, =GIC_DIST_BASE		// GICD_CTLR
	mov	w0, #3				// EnableGrp0 | EnableGrp1
	str	w0, [x1]

1:	ldr	x1, =GIC_DIST_BASE + 0x80	// GICD_IGROUPR
	mov	w0, #~0				// Grp1 interrupts
	str	w0, [x1], #4
	b.ne	2f				// Only local interrupts for secondary CPUs
	str	w0, [x1], #4
	str	w0, [x1], #4

2:	ldr	x1, =GIC_CPU_BASE		// GICC_CTLR
	ldr	w0, [x1]
	orr	w0, w0, #3			// EnableGrp0 | EnableGrp1
	str	w0, [x1]

	mov	w0, #1 << 7			// allow NS access to GICC_PMR
	str	w0, [x1, #4]			// GICC_PMR

	msr	sctlr_el2, xzr

#if defined START_EL1

	/* Now setup our EL1. Controlled by EL2 config on Model */
	mrs	x0, hcr_el2			// Read EL2 Hypervisor configuration Register
	orr	x0, x0, #(1 << 31)		// Set EL1 to be 64bit

	// Send all interrupts to their respective Exception levels for EL2
	bic	x0, x0, #(1 << 3)		// Disable virtual FIQ
	bic	x0, x0, #(1 << 4)		// Disable virtual IRQ
	bic	x0, x0, #(1 << 5)		// Disable virtual SError and Abort
	msr	hcr_el2, x0			// Write back our settings

	/*
	 * Enable architected timer access
	 */
	mrs	x0, cnthctl_el2
	orr	x0, x0, #3			// Enable EL1 access to timers
	msr	cnthctl_el2, x0

	mrs	x0, cntkctl_el1
	orr	x0, x0, #3			// EL0 access to counters
	msr	cntkctl_el1, x0

	/* Set ID regs */
	mrs	x0, midr_el1
	mrs	x1, mpidr_el1
	msr	vpidr_el2, x0
	msr	vmpidr_el2, x1

	/* Coprocessor traps. */
	mov	x0, #0x33ff
	msr	cptr_el2, x0			// Disable copro. traps to EL2

	msr	hstr_el2, xzr			// Disable CP15 traps to EL2

#endif // START_EL1

        /* Configure UART. Primary CPU only */
	mrs     x4, mpidr_el1
	tst     x4, #15
	b.ne    1f

	/*
	 * UART initialisation (38400 8N1)
	 */
	ldr     x4, =UART_BASE                  // UART base
	mov     w5, #0x10                       // ibrd
	str     w5, [x4, #0x24]
	mov     w5, #0xc300
	orr     w5, w5, #0x0001                 // cr
	str     w5, [x4, #0x30]

	/*
	 * Prepare the switch to the EL2_SP2 mode from EL3
	 */
1:	ldr	x0, =start_ns			// Return after mode switch
#if defined START_EL1
	mov	x1, #0x3c5			// EL1_SP1 | D | A | I | F
#else
	mov	x1, #0x3c9			// EL2_SP2 | D | A | I | F
#endif

	msr	elr_el3, x0
	msr	spsr_el3, x1
	eret

#endif // START_EL2


start_ns:

/*
 * We do not have NOR flash on the Foundation model. So run UEFI from RAM.
 * On the full model we use the NOR FLASH to store UEFI, so start there.
 */
#if defined FOUNDATION_MODEL
	mov	x0, #0xa0000000
#else
	mov	x0, #0x0
#endif
	br	x0

	.ltorg

	.org	0x200
