/*-
 * Copyright (c) 1998, 1999, 2000, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center and by Chris G. Demetriou.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1991 Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and William Jolitz of UUNET Technologies Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
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
 * Derived from hp300 version by Mike Hibler, this version by William
 * Jolitz uses a recursive map [a pde points to the page directory] to
 * map the page tables using the pagetables themselves. This is done to
 * reduce the impact on kernel virtual memory for lots of sparse address
 * space, and to reduce the cost of memory to each process.
 *
 *	from: hp300: @(#)pmap.h	7.2 (Berkeley) 12/16/90
 *	from: @(#)pmap.h	7.4 (Berkeley) 5/12/91
 *	from: i386 pmap.h,v 1.54 1997/11/20 19:30:35 bde Exp
 * $FreeBSD: src/sys/ia64/include/pmap.h,v 1.25 2005/09/03 23:53:50 marcel Exp $
 */

#ifndef _PMAP_MACHINE_
#define _PMAP_MACHINE_

#include <sys/lock.h>

paddr_t vtophys(vaddr_t);

struct pv_entry;	/* Forward declaration. */

struct pmap {
	TAILQ_ENTRY(pmap)	pm_list;	/* list of all pmaps */
	TAILQ_HEAD(,pv_entry)	pm_pvlist;	/* list of mappings in pmap */
	int			pm_count;	/* pmap reference count */
	struct simplelock	pm_slock;	/* lock on pmap */
	u_int32_t		pm_rid[5];	/* base RID for pmap */
	int			pm_active;	/* active flag */
	struct pmap_statistics	pm_stats;	/* pmap statistics */
	unsigned long		pm_cpus;	/* mask of CPUs using pmap */

};

typedef struct pmap	*pmap_t;

/*
 * For each vm_page_t, there is a list of all currently valid virtual
 * mappings of that page.  An entry is a pv_entry_t, the list is pv_pvlist.
 */
typedef struct pv_entry {
	pmap_t		pv_pmap;	/* pmap where mapping lies */
	vaddr_t		pv_va;		/* virtual address for mapping */
	TAILQ_ENTRY(pv_entry)	pv_list;
	TAILQ_ENTRY(pv_entry)	pv_plist;
} *pv_entry_t;

/* pvh_attrs */
#define	PGA_MODIFIED		0x01		/* modified */
#define	PGA_REFERENCED		0x02		/* referenced */


extern struct pmap	kernel_pmap_store;

#define pmap_kernel()			(&kernel_pmap_store)

#define	pmap_resident_count(pmap)	((pmap)->pm_stats.resident_count)
#define	pmap_wired_count(pmap)		((pmap)->pm_stats.wired_count)

#define	pmap_copy(dp, sp, da, l, sa)	/* nothing */
#define	pmap_update(pmap)		/* nothing (yet) */

void pmap_bootstrap(void);

#define	pmap_is_referenced(pg)						\
	(((pg)->mdpage.pvh_attrs & PGA_REFERENCED) != 0)
#define	pmap_is_modified(pg)						\
	(((pg)->mdpage.pvh_attrs & PGA_MODIFIED) != 0)


#define PMAP_STEAL_MEMORY		/* enable pmap_steal_memory() */

/*
 * Alternate mapping hooks for pool pages.  Avoids thrashing the TLB.
 */
#define	PMAP_MAP_POOLPAGE(pa)		IA64_PHYS_TO_RR7((pa))
#define	PMAP_UNMAP_POOLPAGE(va)		IA64_RR_MASK((va))

/*
 * Macros for locking pmap structures.
 *
 * Note that we if we access the kernel pmap in interrupt context, it
 * is only to update statistics.  Since stats are updated using atomic
 * operations, locking the kernel pmap is not necessary.  Therefore,
 * it is not necessary to block interrupts when locking pmap strucutres.
 */
#define	PMAP_LOCK(pmap)		simple_lock(&(pmap)->pm_slock)
#define	PMAP_UNLOCK(pmap)	simple_unlock(&(pmap)->pm_slock)


#define PMAP_VHPT_LOG2SIZE 16 


#endif /* _PMAP_MACHINE_ */
