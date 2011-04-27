/*	$NetBSD: vmparam.h,v 1.2 2006/07/03 17:01:45 cherry Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)vmparam.h	5.9 (Berkeley) 5/12/91
 */

#ifndef _VMPARAM_H_
#define _VMPARAM_H_

#include <sys/tree.h>

#define	USRSTACK	VM_MAX_ADDRESS /* XXX: Revisit vm address space. */

/*
 * Virtual memory related constants, all in bytes
 */
#ifndef MAXTSIZ
#define	MAXTSIZ		(1<<30)			/* max text size (1G) */
#endif
#ifndef DFLDSIZ
#define	DFLDSIZ		(1<<27)			/* initial data size (128M) */
#endif
#ifndef MAXDSIZ
#define	MAXDSIZ		(1<<30)			/* max data size (1G) */
#endif
#ifndef	DFLSSIZ
#define	DFLSSIZ		(1<<21)			/* initial stack size (2M) */
#endif
#ifndef	MAXSSIZ
#define	MAXSSIZ		(1<<28)			/* max stack size (256M) */
#endif
#ifndef SGROWSIZ
#define SGROWSIZ	(128UL*1024)		/* amount to grow stack */
#endif



/*
 * PTEs for mapping user space into the kernel for phyio operations.
 * 64 pte's are enough to cover 8 disks * MAXBSIZE.
 */
#ifndef USRIOSIZE
#define USRIOSIZE	64
#endif

/*
 * Manipulating region bits of an address.
 */
#define IA64_RR_BASE(n)         (((u_int64_t) (n)) << 61)
#define IA64_RR_MASK(x)         ((x) & ((1L << 61) - 1))

#define IA64_PHYS_TO_RR6(x)     ((x) | IA64_RR_BASE(6))
#define IA64_PHYS_TO_RR7(x)     ((x) | IA64_RR_BASE(7))

#define	IA64_ID_PAGE_SHIFT	28		/* 256M */
#define	IA64_ID_PAGE_SIZE	(1 << IA64_ID_PAGE_SHIFT)
#define	IA64_ID_PAGE_MASK	(IA64_ID_PAGE_SIZE-1)

#define	IA64_BACKINGSTORE	IA64_RR_BASE(4)

#define	PAGE_SHIFT	14		/* 16K pages by default. */
#define	PAGE_SIZE	(1 << PAGE_SHIFT)
#define	PAGE_MASK	(PAGE_SIZE - 1)

/* user/kernel map constants */
#define VM_MIN_ADDRESS		((vaddr_t)0)
#define	VM_MAX_ADDRESS		((vaddr_t) IA64_RR_BASE(5))
#define	VM_GATEWAY_SIZE		PAGE_SIZE
#define	VM_MAXUSER_ADDRESS	(VM_MAX_ADDRESS + VM_GATEWAY_SIZE)
#define	VM_MIN_KERNEL_ADDRESS	VM_MAXUSER_ADDRESS
#define VM_MAX_KERNEL_ADDRESS	((vaddr_t) (IA64_RR_BASE(6) - 1))

#define VM_PHYSSEG_MAX		16		/* XXX: */
#define VM_PHYSSEG_STRAT	VM_PSTRAT_BSEARCH
#define	VM_PHYSSEG_NOADD			/* no more after vm_mem_init */

#define	VM_NFREELIST		1 /* XXX: */
#define	VM_FREELIST_DEFAULT	0 /* XXX: */

/* virtual sizes (bytes) for various kernel submaps */
#define VM_PHYS_SIZE		(USRIOSIZE*PAGE_SIZE)

#ifndef _LOCORE
/*
 * pmap-specific data store in the vm_page structure.
 */
#define	__HAVE_VM_PAGE_MD
struct vm_page_md {
	TAILQ_HEAD(,pv_entry) pv_list;		/* pv_entry list */
	int pv_list_count;
	struct simplelock pv_slock;		/* lock on this head */
	int pvh_attrs;				/* page attributes */
};

#define	VM_MDPAGE_INIT(pg)						\
do {									\
	TAILQ_INIT(&(pg)->mdpage.pv_list);				\
	simple_lock_init(&(pg)->mdpage.pv_slock);			\
} while (/*CONSTCOND*/0)
#endif /*_LOCORE*/

#endif /* _VMPARAM_H_ */
