/*	$NetBSD: cpu.h,v 1.1 2006/04/07 14:21:18 cherry Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center, and by Charles M. Hannum.
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
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 * from: Utah $Hdr: cpu.h 1.16 91/03/25$
 *
 *	@(#)cpu.h	8.4 (Berkeley) 1/5/94
 */


#ifndef _IA64_CPU_H_
#define _IA64_CPU_H_

#ifdef _KERNEL
#include <sys/cpu_data.h>
#include <sys/cc_microtime.h>
#include <machine/frame.h>
#include <machine/ia64_cpu.h>


struct cpu_info {
	struct device *ci_dev;		/* pointer to our device */
	struct cpu_info *ci_self;	/* self-pointer */
	/*
	 * Public members.
	 */
	struct lwp *ci_curlwp;		/* current owner of the processor */
	struct cpu_data ci_data;	/* MI per-cpu data */
	struct cc_microtime_state ci_cc;/* cc_microtime state */
	struct cpu_info *ci_next;	/* next cpu_info structure */

	/* XXX: Todo */
	/*
	 * Private members.
	 */
	cpuid_t ci_cpuid;		/* our CPU ID */
	struct pmap *ci_pmap;		/* current pmap */
	struct lwp *ci_fpcurlwp;	/* current owner of the FPU */
	paddr_t ci_curpcb;		/* PA of current HW PCB */
	struct pcb *ci_idle_pcb;	/* our idle PCB */
	struct cpu_softc *ci_softc;	/* pointer to our device */
	u_long ci_want_resched;		/* preempt current process */
	u_long ci_intrdepth;		/* interrupt trap depth */
	struct trapframe *ci_db_regs;	/* registers for debuggers */
};


extern struct cpu_info cpu_info_primary;

#ifdef MULTIPROCESSOR
/* XXX: TODO */
#else
#define	curcpu() (&cpu_info_primary)
#endif /* MULTIPROCESSOR */

#define cpu_number() 0              /*XXX: FIXME */

#define aston(p)		((p)->p_md.md_astpending = 1)

#define	need_resched(ci)            /*XXX: FIXME */

struct clockframe {
	struct trapframe cf_tf;
};

#define	CLKF_PC(cf)		((cf)->cf_tf.tf_special.iip)
#define	CLKF_CPL(cf)		((cf)->cf_tf.tf_special.psr & IA64_PSR_CPL)
#define	CLKF_USERMODE(cf)	(CLKF_CPL(cf) != IA64_PSR_CPL_KERN)
#define	CLKF_BASEPRI(frame)	(0) /*XXX: CHECKME */
#define	CLKF_INTR(frame)	(curcpu()->ci_intrdepth)

#define	TRAPF_PC(tf)		((tf)->tf_special.iip)
#define	TRAPF_CPL(tf)		((tf)->tf_special.psr & IA64_PSR_CPL)
#define	TRAPF_USERMODE(tf)	(TRAPF_CPL(tf) != IA64_PSR_CPL_KERN)







/*
 * Give a profiling tick to the current process when the user profiling
 * buffer pages are invalid. XXX:Fixme.... On the ia64 I haven't yet figured 
 * out what to do about this.. XXX.
 */


#define	need_proftick(p)

/*
 * Notify the current process (p) that it has a signal pending,
 * process as soon as possible.
 */
#define	signotify(p)		aston(p)

#define setsoftclock()              /*XXX: FIXME */

/* machdep.c */
int	cpu_maxproc(void); /*XXX: Fill in machdep.c */

#define	cpu_proc_fork(p1, p2) /* XXX: Look into this. */


/* XXX: TODO: generic microtime support kern/kern_microtime.c 
 * #define microtime(tv)	cc_microtime(tv) 
 */


#endif /* _KERNEL_ */
#endif /* _IA64_CPU_H */
