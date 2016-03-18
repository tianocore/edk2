/*	$NetBSD: asm.h,v 1.6 2006/09/05 19:00:42 ad Exp $	*/

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
 *	@(#)asm.h	5.5 (Berkeley) 5/7/91
 */

#ifndef _AMD64_ASM_H_
#define _AMD64_ASM_H_

#ifdef PIC
#define PIC_PLT(x)	x@PLT
#define PIC_GOT(x)	x@GOTPCREL(%rip)
#else
#define PIC_PLT(x)	x
#define PIC_GOT(x)	x
#endif

# define _C_LABEL(x)	x
#define	_ASM_LABEL(x)	x

#define CVAROFF(x,y)		(_C_LABEL(x)+y)(%rip)

#ifdef __STDC__
# define __CONCAT(x,y)	x ## y
# define __STRING(x)	#x
#else
# define __CONCAT(x,y)	x/**/y
# define __STRING(x)	"x"
#endif

/* let kernels and others override entrypoint alignment */
#ifndef _ALIGN_TEXT
#define _ALIGN_TEXT .align 4
#endif

#define _ENTRY(x) \
	.text; _ALIGN_TEXT; .globl x; .type x,@function; x:

#ifdef _KERNEL
/* XXX Can't use __CONCAT() here, as it would be evaluated incorrectly. */
#ifdef __STDC__
#define	IDTVEC(name) \
	ALIGN_TEXT; .globl X ## name; .type X ## name,@function; X ## name:
#else 
#define	IDTVEC(name) \
	ALIGN_TEXT; .globl X/**/name; .type X/**/name,@function; X/**/name:
#endif /* __STDC__ */ 
#endif /* _KERNEL */

#ifdef __STDC__
#define CPUVAR(off)	%gs:CPU_INFO_ ## off
#else
#define CPUVAR(off)     %gs:CPU_INFO_/**/off
#endif


#ifdef GPROF
# define _PROF_PROLOGUE	\
	pushq %rbp; leaq (%rsp),%rbp; call PIC_PLT(__mcount); popq %rbp
#else
# define _PROF_PROLOGUE
#endif

#define	ENTRY(y)	_ENTRY(_C_LABEL(y)); _PROF_PROLOGUE
#define	NENTRY(y)	_ENTRY(_C_LABEL(y))
#define	ASENTRY(y)	_ENTRY(_ASM_LABEL(y)); _PROF_PROLOGUE

#define	ASMSTR		.asciz

#define RCSID(x)	.text; .asciz x

#define	WEAK_ALIAS(alias,sym)						\
	.weak alias;							\
	alias = sym

/*
 * STRONG_ALIAS: create a strong alias.
 */
#define STRONG_ALIAS(alias,sym)						\
	.globl alias;							\
	alias = sym

/* XXXfvdl do not use stabs here */
#ifdef __STDC__
#define	WARN_REFERENCES(sym,msg)					\
	.stabs msg ## ,30,0,0,0 ;					\
	.stabs __STRING(_C_LABEL(sym)) ## ,1,0,0,0
#else
#define	WARN_REFERENCES(sym,msg)					\
	.stabs msg,30,0,0,0 ;						\
	.stabs __STRING(sym),1,0,0,0
#endif /* __STDC__ */

/*
 * Assembley equivalent of spllower().  Label contains the label to jump to
 * if we need to fire off pending interrupts (e.g. _C_LABEL(Xspllower)).
 *
 * On entry %rcx = new SPL.
 */
#define	SPLLOWER(label)						\
	movq		CPUVAR(SELF), %r9 ;			\
	cmpl		CPU_INFO_ILEVEL(%r9), %ecx ;		\
	jae		99f ;					\
	movl		CPU_INFO_IUNMASK(%r9,%rcx,4), %edi ;	\
	pushfq		;					\
	popq		%rax ;					\
	cli		;					\
	testl		CPU_INFO_IPENDING(%r9), %edi ;		\
	movq		%rcx, %rdi ;				\
	jnz		label ;					\
	movl		%ecx, CPU_INFO_ILEVEL(%r9) ;		\
	pushq		%rax ;					\
	popfq		;					\
99:

#endif /* !_AMD64_ASM_H_ */
