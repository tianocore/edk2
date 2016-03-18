/*	$NetBSD: acpi_func.h,v 1.2 2006/05/14 21:55:38 elad Exp $	*/

/*-
 * Copyright (c) 2002 Mitsuru IWASAKI
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/ia64/include/acpica_machdep.h,v 1.4 2004/10/11 05:39:15 njl Exp $
 */

/******************************************************************************
 *
 * Name: acpica_machdep.h - arch-specific defines, etc.
 *       $Revision: 1.2 $
 *
 *****************************************************************************/

#ifndef _IA64_ACPI_FUNC_H_
#define _IA64_ACPI_FUNC_H_

#include <machine/cpufunc.h>
#include <machine/atomic.h>

/* Asm macros */

#define ACPI_ASM_MACROS
#define BREAKPOINT3
#define ACPI_DISABLE_IRQS() disable_intr()
#define ACPI_ENABLE_IRQS()  enable_intr()

#define ACPI_FLUSH_CPU_CACHE()	/* XXX ia64_fc()? */


/* Section 5.2.9.1:  global lock acquire/release functions */
extern int	acpi_acquire_global_lock(uint32_t *lock);
extern int	acpi_release_global_lock(uint32_t *lock);
#define ACPI_ACQUIRE_GLOBAL_LOCK(GLptr, Acq) \
		((Acq) = acpi_acquire_global_lock(GLptr))
#define ACPI_RELEASE_GLOBAL_LOCK(GLptr, Acq) \
		((Acq) = acpi_release_global_lock(GLptr))


/* Section 5.2.9.1:  global lock acquire/release functions */
#define GL_ACQUIRED	(-1)
#define GL_BUSY		0
#define GL_BIT_PENDING	0x1
#define GL_BIT_OWNED	0x2
#define GL_BIT_MASK	(GL_BIT_PENDING | GL_BIT_OWNED)

/*
 * Acquire the global lock.  If busy, set the pending bit.  The caller
 * will wait for notification from the BIOS that the lock is available
 * and then attempt to acquire it again.
 */
int
acpi_acquire_global_lock(uint32_t *lock)
{
	uint32_t new, old;

	do {
		old = *lock;
		new = ((old & ~GL_BIT_MASK) | GL_BIT_OWNED) |
			((old >> 1) & GL_BIT_PENDING);
	} while (atomic_cmpset_acq_int(lock, old, new) == 0);

	return ((new < GL_BIT_MASK) ? GL_ACQUIRED : GL_BUSY);
}

/*
 * Release the global lock, returning whether there is a waiter pending.
 * If the BIOS set the pending bit, OSPM must notify the BIOS when it
 * releases the lock.
 */
int
acpi_release_global_lock(uint32_t *lock)
{
	uint32_t new, old;

	do {
		old = *lock;
		new = old & ~GL_BIT_MASK;
	} while (atomic_cmpset_rel_int(lock, old, new) == 0);

	return (old & GL_BIT_PENDING);
}

#endif /* _IA64_ACPI_FUNC_H_ */
