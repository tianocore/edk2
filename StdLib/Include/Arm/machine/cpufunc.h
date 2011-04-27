/*	$NetBSD: cpufunc.h,v 1.37.24.1 2007/02/21 18:36:02 snj Exp $	*/

/*
 * Copyright (c) 1997 Mark Brinicombe.
 * Copyright (c) 1997 Causality Limited
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Causality Limited.
 * 4. The name of Causality Limited may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY CAUSALITY LIMITED ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL CAUSALITY LIMITED BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * cpufunc.h
 *
 * Prototypes for cpu, mmu and tlb related functions.
 */

#ifndef _ARM32_CPUFUNC_H_
#define _ARM32_CPUFUNC_H_

#ifdef _KERNEL

#include <sys/types.h>
#include <arm/cpuconf.h>

struct cpu_functions {

	/* CPU functions */

	u_int	(*cf_id)		__P((void));
	void	(*cf_cpwait)		__P((void));

	/* MMU functions */

	u_int	(*cf_control)		__P((u_int, u_int));
	void	(*cf_domains)		__P((u_int));
	void	(*cf_setttb)		__P((u_int));
	u_int	(*cf_faultstatus)	__P((void));
	u_int	(*cf_faultaddress)	__P((void));

	/* TLB functions */

	void	(*cf_tlb_flushID)	__P((void));
	void	(*cf_tlb_flushID_SE)	__P((u_int));
	void	(*cf_tlb_flushI)	__P((void));
	void	(*cf_tlb_flushI_SE)	__P((u_int));
	void	(*cf_tlb_flushD)	__P((void));
	void	(*cf_tlb_flushD_SE)	__P((u_int));

	/*
	 * Cache operations:
	 *
	 * We define the following primitives:
	 *
	 *	icache_sync_all		Synchronize I-cache
	 *	icache_sync_range	Synchronize I-cache range
	 *
	 *	dcache_wbinv_all	Write-back and Invalidate D-cache
	 *	dcache_wbinv_range	Write-back and Invalidate D-cache range
	 *	dcache_inv_range	Invalidate D-cache range
	 *	dcache_wb_range		Write-back D-cache range
	 *
	 *	idcache_wbinv_all	Write-back and Invalidate D-cache,
	 *				Invalidate I-cache
	 *	idcache_wbinv_range	Write-back and Invalidate D-cache,
	 *				Invalidate I-cache range
	 *
	 * Note that the ARM term for "write-back" is "clean".  We use
	 * the term "write-back" since it's a more common way to describe
	 * the operation.
	 *
	 * There are some rules that must be followed:
	 *
	 *	I-cache Synch (all or range):
	 *		The goal is to synchronize the instruction stream,
	 *		so you may beed to write-back dirty D-cache blocks
	 *		first.  If a range is requested, and you can't
	 *		synchronize just a range, you have to hit the whole
	 *		thing.
	 *
	 *	D-cache Write-Back and Invalidate range:
	 *		If you can't WB-Inv a range, you must WB-Inv the
	 *		entire D-cache.
	 *
	 *	D-cache Invalidate:
	 *		If you can't Inv the D-cache, you must Write-Back
	 *		and Invalidate.  Code that uses this operation
	 *		MUST NOT assume that the D-cache will not be written
	 *		back to memory.
	 *
	 *	D-cache Write-Back:
	 *		If you can't Write-back without doing an Inv,
	 *		that's fine.  Then treat this as a WB-Inv.
	 *		Skipping the invalidate is merely an optimization.
	 *
	 *	All operations:
	 *		Valid virtual addresses must be passed to each
	 *		cache operation.
	 */
	void	(*cf_icache_sync_all)	__P((void));
	void	(*cf_icache_sync_range)	__P((vaddr_t, vsize_t));

	void	(*cf_dcache_wbinv_all)	__P((void));
	void	(*cf_dcache_wbinv_range) __P((vaddr_t, vsize_t));
	void	(*cf_dcache_inv_range)	__P((vaddr_t, vsize_t));
	void	(*cf_dcache_wb_range)	__P((vaddr_t, vsize_t));

	void	(*cf_idcache_wbinv_all)	__P((void));
	void	(*cf_idcache_wbinv_range) __P((vaddr_t, vsize_t));

	/* Other functions */

	void	(*cf_flush_prefetchbuf)	__P((void));
	void	(*cf_drain_writebuf)	__P((void));
	void	(*cf_flush_brnchtgt_C)	__P((void));
	void	(*cf_flush_brnchtgt_E)	__P((u_int));

	void	(*cf_sleep)		__P((int mode));

	/* Soft functions */

	int	(*cf_dataabt_fixup)	__P((void *));
	int	(*cf_prefetchabt_fixup)	__P((void *));

	void	(*cf_context_switch)	__P((void));

	void	(*cf_setup)		__P((char *));
};

extern struct cpu_functions cpufuncs;
extern u_int cputype;

#define cpu_id()		cpufuncs.cf_id()
#define	cpu_cpwait()		cpufuncs.cf_cpwait()

#define cpu_control(c, e)	cpufuncs.cf_control(c, e)
#define cpu_domains(d)		cpufuncs.cf_domains(d)
#define cpu_setttb(t)		cpufuncs.cf_setttb(t)
#define cpu_faultstatus()	cpufuncs.cf_faultstatus()
#define cpu_faultaddress()	cpufuncs.cf_faultaddress()

#define	cpu_tlb_flushID()	cpufuncs.cf_tlb_flushID()
#define	cpu_tlb_flushID_SE(e)	cpufuncs.cf_tlb_flushID_SE(e)
#define	cpu_tlb_flushI()	cpufuncs.cf_tlb_flushI()
#define	cpu_tlb_flushI_SE(e)	cpufuncs.cf_tlb_flushI_SE(e)
#define	cpu_tlb_flushD()	cpufuncs.cf_tlb_flushD()
#define	cpu_tlb_flushD_SE(e)	cpufuncs.cf_tlb_flushD_SE(e)

#define	cpu_icache_sync_all()	cpufuncs.cf_icache_sync_all()
#define	cpu_icache_sync_range(a, s) cpufuncs.cf_icache_sync_range((a), (s))

#define	cpu_dcache_wbinv_all()	cpufuncs.cf_dcache_wbinv_all()
#define	cpu_dcache_wbinv_range(a, s) cpufuncs.cf_dcache_wbinv_range((a), (s))
#define	cpu_dcache_inv_range(a, s) cpufuncs.cf_dcache_inv_range((a), (s))
#define	cpu_dcache_wb_range(a, s) cpufuncs.cf_dcache_wb_range((a), (s))

#define	cpu_idcache_wbinv_all()	cpufuncs.cf_idcache_wbinv_all()
#define	cpu_idcache_wbinv_range(a, s) cpufuncs.cf_idcache_wbinv_range((a), (s))

#define	cpu_flush_prefetchbuf()	cpufuncs.cf_flush_prefetchbuf()
#define	cpu_drain_writebuf()	cpufuncs.cf_drain_writebuf()
#define	cpu_flush_brnchtgt_C()	cpufuncs.cf_flush_brnchtgt_C()
#define	cpu_flush_brnchtgt_E(e)	cpufuncs.cf_flush_brnchtgt_E(e)

#define cpu_sleep(m)		cpufuncs.cf_sleep(m)

#define cpu_dataabt_fixup(a)		cpufuncs.cf_dataabt_fixup(a)
#define cpu_prefetchabt_fixup(a)	cpufuncs.cf_prefetchabt_fixup(a)
#define ABORT_FIXUP_OK		0	/* fixup succeeded */
#define ABORT_FIXUP_FAILED	1	/* fixup failed */
#define ABORT_FIXUP_RETURN	2	/* abort handler should return */

#define cpu_setup(a)			cpufuncs.cf_setup(a)

int	set_cpufuncs		__P((void));
#define ARCHITECTURE_NOT_PRESENT	1	/* known but not configured */
#define ARCHITECTURE_NOT_SUPPORTED	2	/* not known */

void	cpufunc_nullop		__P((void));
int	cpufunc_null_fixup	__P((void *));
int	early_abort_fixup	__P((void *));
int	late_abort_fixup	__P((void *));
u_int	cpufunc_id		__P((void));
u_int	cpufunc_control		__P((u_int, u_int));
void	cpufunc_domains		__P((u_int));
u_int	cpufunc_faultstatus	__P((void));
u_int	cpufunc_faultaddress	__P((void));

#ifdef CPU_ARM3
u_int	arm3_control		__P((u_int, u_int));
void	arm3_cache_flush	__P((void));
#endif	/* CPU_ARM3 */

#if defined(CPU_ARM6) || defined(CPU_ARM7)
void	arm67_setttb		__P((u_int));
void	arm67_tlb_flush		__P((void));
void	arm67_tlb_purge		__P((u_int));
void	arm67_cache_flush	__P((void));
void	arm67_context_switch	__P((void));
#endif	/* CPU_ARM6 || CPU_ARM7 */

#ifdef CPU_ARM6
void	arm6_setup		__P((char *));
#endif	/* CPU_ARM6 */

#ifdef CPU_ARM7
void	arm7_setup		__P((char *));
#endif	/* CPU_ARM7 */

#ifdef CPU_ARM7TDMI
int	arm7_dataabt_fixup	__P((void *));
void	arm7tdmi_setup		__P((char *));
void	arm7tdmi_setttb		__P((u_int));
void	arm7tdmi_tlb_flushID	__P((void));
void	arm7tdmi_tlb_flushID_SE	__P((u_int));
void	arm7tdmi_cache_flushID	__P((void));
void	arm7tdmi_context_switch	__P((void));
#endif /* CPU_ARM7TDMI */

#ifdef CPU_ARM8
void	arm8_setttb		__P((u_int));
void	arm8_tlb_flushID	__P((void));
void	arm8_tlb_flushID_SE	__P((u_int));
void	arm8_cache_flushID	__P((void));
void	arm8_cache_flushID_E	__P((u_int));
void	arm8_cache_cleanID	__P((void));
void	arm8_cache_cleanID_E	__P((u_int));
void	arm8_cache_purgeID	__P((void));
void	arm8_cache_purgeID_E	__P((u_int entry));

void	arm8_cache_syncI	__P((void));
void	arm8_cache_cleanID_rng	__P((vaddr_t, vsize_t));
void	arm8_cache_cleanD_rng	__P((vaddr_t, vsize_t));
void	arm8_cache_purgeID_rng	__P((vaddr_t, vsize_t));
void	arm8_cache_purgeD_rng	__P((vaddr_t, vsize_t));
void	arm8_cache_syncI_rng	__P((vaddr_t, vsize_t));

void	arm8_context_switch	__P((void));

void	arm8_setup		__P((char *));

u_int	arm8_clock_config	__P((u_int, u_int));
#endif

#ifdef CPU_SA110
void	sa110_setup		__P((char *));
void	sa110_context_switch	__P((void));
#endif	/* CPU_SA110 */

#if defined(CPU_SA1100) || defined(CPU_SA1110)
void	sa11x0_drain_readbuf	__P((void));

void	sa11x0_context_switch	__P((void));
void	sa11x0_cpu_sleep	__P((int));

void	sa11x0_setup		__P((char *));
#endif

#if defined(CPU_SA110) || defined(CPU_SA1100) || defined(CPU_SA1110)
void	sa1_setttb		__P((u_int));

void	sa1_tlb_flushID_SE	__P((u_int));

void	sa1_cache_flushID	__P((void));
void	sa1_cache_flushI	__P((void));
void	sa1_cache_flushD	__P((void));
void	sa1_cache_flushD_SE	__P((u_int));

void	sa1_cache_cleanID	__P((void));
void	sa1_cache_cleanD	__P((void));
void	sa1_cache_cleanD_E	__P((u_int));

void	sa1_cache_purgeID	__P((void));
void	sa1_cache_purgeID_E	__P((u_int));
void	sa1_cache_purgeD	__P((void));
void	sa1_cache_purgeD_E	__P((u_int));

void	sa1_cache_syncI		__P((void));
void	sa1_cache_cleanID_rng	__P((vaddr_t, vsize_t));
void	sa1_cache_cleanD_rng	__P((vaddr_t, vsize_t));
void	sa1_cache_purgeID_rng	__P((vaddr_t, vsize_t));
void	sa1_cache_purgeD_rng	__P((vaddr_t, vsize_t));
void	sa1_cache_syncI_rng	__P((vaddr_t, vsize_t));

#endif

#ifdef CPU_ARM9
void	arm9_setttb		__P((u_int));

void	arm9_tlb_flushID_SE	__P((u_int));

void	arm9_icache_sync_all	__P((void));
void	arm9_icache_sync_range	__P((vaddr_t, vsize_t));

void	arm9_dcache_wbinv_all	__P((void));
void	arm9_dcache_wbinv_range __P((vaddr_t, vsize_t));
void	arm9_dcache_inv_range	__P((vaddr_t, vsize_t));
void	arm9_dcache_wb_range	__P((vaddr_t, vsize_t));

void	arm9_idcache_wbinv_all	__P((void));
void	arm9_idcache_wbinv_range __P((vaddr_t, vsize_t));

void	arm9_context_switch	__P((void));

void	arm9_setup		__P((char *));

extern unsigned arm9_dcache_sets_max;
extern unsigned arm9_dcache_sets_inc;
extern unsigned arm9_dcache_index_max;
extern unsigned arm9_dcache_index_inc;
#endif

#if defined(CPU_ARM9E) || defined(CPU_ARM10)
void	arm10_tlb_flushID_SE	__P((u_int));
void	arm10_tlb_flushI_SE	__P((u_int));

void	arm10_context_switch	__P((void));

void	arm10_setup		__P((char *));
#endif

#ifdef CPU_ARM11
void	arm11_setttb		__P((u_int));

void	arm11_tlb_flushID_SE	__P((u_int));
void	arm11_tlb_flushI_SE	__P((u_int));

void	arm11_context_switch	__P((void));

void	arm11_setup		__P((char *string));
void	arm11_tlb_flushID	__P((void));
void	arm11_tlb_flushI	__P((void));
void	arm11_tlb_flushD	__P((void));
void	arm11_tlb_flushD_SE	__P((u_int va));

void	arm11_drain_writebuf	__P((void));
#endif

#if defined(CPU_ARM9E) || defined (CPU_ARM10)
void	armv5_ec_setttb			__P((u_int));

void	armv5_ec_icache_sync_all	__P((void));
void	armv5_ec_icache_sync_range	__P((vaddr_t, vsize_t));

void	armv5_ec_dcache_wbinv_all	__P((void));
void	armv5_ec_dcache_wbinv_range	__P((vaddr_t, vsize_t));
void	armv5_ec_dcache_inv_range	__P((vaddr_t, vsize_t));
void	armv5_ec_dcache_wb_range	__P((vaddr_t, vsize_t));

void	armv5_ec_idcache_wbinv_all	__P((void));
void	armv5_ec_idcache_wbinv_range	__P((vaddr_t, vsize_t));
#endif

#if defined (CPU_ARM10) || defined (CPU_ARM11)
void	armv5_setttb		__P((u_int));

void	armv5_icache_sync_all	__P((void));
void	armv5_icache_sync_range	__P((vaddr_t, vsize_t));

void	armv5_dcache_wbinv_all	__P((void));
void	armv5_dcache_wbinv_range __P((vaddr_t, vsize_t));
void	armv5_dcache_inv_range	__P((vaddr_t, vsize_t));
void	armv5_dcache_wb_range	__P((vaddr_t, vsize_t));

void	armv5_idcache_wbinv_all	__P((void));
void	armv5_idcache_wbinv_range __P((vaddr_t, vsize_t));

extern unsigned armv5_dcache_sets_max;
extern unsigned armv5_dcache_sets_inc;
extern unsigned armv5_dcache_index_max;
extern unsigned armv5_dcache_index_inc;
#endif

#if defined(CPU_ARM9) || defined(CPU_ARM9E) || defined(CPU_ARM10) || \
    defined(CPU_SA110) || defined(CPU_SA1100) || defined(CPU_SA1110) || \
    defined(CPU_XSCALE_80200) || defined(CPU_XSCALE_80321) || \
    defined(__CPU_XSCALE_PXA2XX) || defined(CPU_XSCALE_IXP425)

void	armv4_tlb_flushID	__P((void));
void	armv4_tlb_flushI	__P((void));
void	armv4_tlb_flushD	__P((void));
void	armv4_tlb_flushD_SE	__P((u_int));

void	armv4_drain_writebuf	__P((void));
#endif

#if defined(CPU_IXP12X0)
void	ixp12x0_drain_readbuf	__P((void));
void	ixp12x0_context_switch	__P((void));
void	ixp12x0_setup		__P((char *));
#endif

#if defined(CPU_XSCALE_80200) || defined(CPU_XSCALE_80321) || \
    defined(__CPU_XSCALE_PXA2XX) || defined(CPU_XSCALE_IXP425)
void	xscale_cpwait		__P((void));

void	xscale_cpu_sleep	__P((int));

u_int	xscale_control		__P((u_int, u_int));

void	xscale_setttb		__P((u_int));

void	xscale_tlb_flushID_SE	__P((u_int));

void	xscale_cache_flushID	__P((void));
void	xscale_cache_flushI	__P((void));
void	xscale_cache_flushD	__P((void));
void	xscale_cache_flushD_SE	__P((u_int));

void	xscale_cache_cleanID	__P((void));
void	xscale_cache_cleanD	__P((void));
void	xscale_cache_cleanD_E	__P((u_int));

void	xscale_cache_clean_minidata __P((void));

void	xscale_cache_purgeID	__P((void));
void	xscale_cache_purgeID_E	__P((u_int));
void	xscale_cache_purgeD	__P((void));
void	xscale_cache_purgeD_E	__P((u_int));

void	xscale_cache_syncI	__P((void));
void	xscale_cache_cleanID_rng __P((vaddr_t, vsize_t));
void	xscale_cache_cleanD_rng	__P((vaddr_t, vsize_t));
void	xscale_cache_purgeID_rng __P((vaddr_t, vsize_t));
void	xscale_cache_purgeD_rng	__P((vaddr_t, vsize_t));
void	xscale_cache_syncI_rng	__P((vaddr_t, vsize_t));
void	xscale_cache_flushD_rng	__P((vaddr_t, vsize_t));

void	xscale_context_switch	__P((void));

void	xscale_setup		__P((char *));
#endif	/* CPU_XSCALE_80200 || CPU_XSCALE_80321 || __CPU_XSCALE_PXA2XX || CPU_XSCALE_IXP425 */

#define tlb_flush	cpu_tlb_flushID
#define setttb		cpu_setttb
#define drain_writebuf	cpu_drain_writebuf

/*
 * Macros for manipulating CPU interrupts
 */
#ifdef __PROG32
static __inline u_int32_t __set_cpsr_c(u_int bic, u_int eor) __attribute__((__unused__));

static __inline u_int32_t
__set_cpsr_c(u_int bic, u_int eor)
{
	u_int32_t	tmp, ret;

	__asm volatile(
		"mrs     %0, cpsr\n"	/* Get the CPSR */
		"bic	 %1, %0, %2\n"	/* Clear bits */
		"eor	 %1, %1, %3\n"	/* XOR bits */
		"msr     cpsr_c, %1\n"	/* Set the control field of CPSR */
	: "=&r" (ret), "=&r" (tmp)
	: "r" (bic), "r" (eor) : "memory");

	return ret;
}

#define disable_interrupts(mask)					\
	(__set_cpsr_c((mask) & (I32_bit | F32_bit), \
		      (mask) & (I32_bit | F32_bit)))

#define enable_interrupts(mask)						\
	(__set_cpsr_c((mask) & (I32_bit | F32_bit), 0))

#define restore_interrupts(old_cpsr)					\
	(__set_cpsr_c((I32_bit | F32_bit), (old_cpsr) & (I32_bit | F32_bit)))
#else /* ! __PROG32 */
#define	disable_interrupts(mask)					\
	(set_r15((mask) & (R15_IRQ_DISABLE | R15_FIQ_DISABLE),		\
		 (mask) & (R15_IRQ_DISABLE | R15_FIQ_DISABLE)))

#define	enable_interrupts(mask)						\
	(set_r15((mask) & (R15_IRQ_DISABLE | R15_FIQ_DISABLE), 0))

#define	restore_interrupts(old_r15)					\
	(set_r15((R15_IRQ_DISABLE | R15_FIQ_DISABLE),			\
		 (old_r15) & (R15_IRQ_DISABLE | R15_FIQ_DISABLE)))
#endif /* __PROG32 */

#ifdef __PROG32
/* Functions to manipulate the CPSR. */
u_int	SetCPSR(u_int, u_int);
u_int	GetCPSR(void);
#else
/* Functions to manipulate the processor control bits in r15. */
u_int	set_r15(u_int, u_int);
u_int	get_r15(void);
#endif /* __PROG32 */

/*
 * Functions to manipulate cpu r13
 * (in arm/arm32/setstack.S)
 */

void set_stackptr	__P((u_int, u_int));
u_int get_stackptr	__P((u_int));

/*
 * Miscellany
 */

int get_pc_str_offset	__P((void));

/*
 * CPU functions from locore.S
 */

void cpu_reset		__P((void)) __attribute__((__noreturn__));

/*
 * Cache info variables.
 */

/* PRIMARY CACHE VARIABLES */
extern int	arm_picache_size;
extern int	arm_picache_line_size;
extern int	arm_picache_ways;

extern int	arm_pdcache_size;	/* and unified */
extern int	arm_pdcache_line_size;
extern int	arm_pdcache_ways;

extern int	arm_pcache_type;
extern int	arm_pcache_unified;

extern int	arm_dcache_align;
extern int	arm_dcache_align_mask;

#endif	/* _KERNEL */
#endif	/* _ARM32_CPUFUNC_H_ */

/* End of cpufunc.h */
