/* $NetBSD: db_machdep.h,v 1.2 2006/08/30 11:12:04 cherry Exp $ */

/*
 * Copyright (c) 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#ifndef	_IA64_DB_MACHDEP_H_
#define	_IA64_DB_MACHDEP_H_

/*
 * Machine-dependent defines for new kernel debugger.
 */

#include <sys/lock.h>
#include <sys/param.h>
#include <uvm/uvm_extern.h>
#include <machine/frame.h>
#include <machine/ia64_cpu.h>

typedef	vaddr_t		db_addr_t;	/* address - unsigned */
typedef	long		db_expr_t;	/* expression - signed */

typedef struct trapframe db_regs_t;
extern db_regs_t	*ddb_regp;	/* pointer to current register state */
#define	DDB_REGS	(ddb_regp)

#if 0	/* XXX: disabling this until we switch on makectx()and have a proper \
	   curlwp(). TODO: please switch this back on ASAP */

#define	PC_REGS(regs)	((db_addr_t)(regs)->tf_special.__spare == 0) ?	\
			((db_addr_t)(regs)->tf_special.rp) :		\
			((db_addr_t)(regs)->tf_special.iip + (((regs)->tf_special.psr>>41) & 3))
#endif

#if 1
#define PC_REGS(regs)   ((db_addr_t)(regs)->tf_special.iip + (((regs)->tf_special.psr>>41) & 3))
#endif

#define db_set_single_step(regs)	((regs)->tf_special.psr |= IA64_PSR_SS)
#define db_clear_single_step(regs)	((regs)->tf_special.psr &= ~IA64_PSR_SS)



/* defines to help with manipulating ia64 VLIW instruction bundles and slots */

#define	TMPL_BITS	5
#define	TMPL_MASK	((1 << TMPL_BITS) - 1)
#define	SLOT_BITS	41
#define	SLOT_COUNT	3
#define	SLOT_MASK	((1ULL << SLOT_BITS) - 1ULL)
#define	SLOT_SHIFT(i)	(TMPL_BITS+((i)<<3)+(i))

#define ADDR_SLOT0(addr)	( (addr) & ~(0xFUL) )
#define SLOT_ADDR(addr)		( (addr) & (0xFUL) )
/* breakpoint address. 
 * Check for violations of pseudo offsets above 2. 
 * Adjust for 32 bit shift within Bundle.
 */

#define	BKPT_ADDR(addr)	( (SLOT_ADDR(addr) < SLOT_COUNT) ? \
			  (ADDR_SLOT0(addr) | (SLOT_ADDR(addr) << 2))	\
			  : ADDR_SLOT0(addr) )

#define	BKPT_SIZE	8

#define	BKPT_SET(inst, addr)	db_bkpt_set(inst, addr)
db_expr_t db_bkpt_set(db_expr_t inst, db_addr_t addr);


#define PC_ADVANCE(regs) db_pc_advance(regs)
void db_pc_advance(db_regs_t *);

#define	IS_BREAKPOINT_TRAP(type, code)	(type == IA64_VEC_BREAK)
#define	IS_WATCHPOINT_TRAP(type, code)	0


#define	inst_trap_return(ins)	(ins & 0)
#define	inst_return(ins)	(ins & 0)
#define	inst_call(ins)		(ins & 0)
#define	inst_branch(ins)	(ins & 0)
#define	inst_load(ins)		(ins & 0)
#define	inst_store(ins)		(ins & 0)
#define	inst_unconditional_flow_transfer(ins) (ins & 0)

#define	branch_taken(ins, pc, regs) pc

u_long	db_register_value(db_regs_t *, int);
int	ddb_trap(unsigned long, unsigned long, unsigned long,
	    unsigned long, struct trapframe *);

int ia64_trap(int, int, db_regs_t *); /* See: trap.c */

/*
 * We define some of our own commands.
 */
#define	DB_MACHINE_COMMANDS

/*
 * We use Elf64 symbols in DDB.
 */
#define	DB_ELF_SYMBOLS
#define	DB_ELFSIZE	64

/*
 * Stuff for KGDB.
 */
typedef long		kgdb_reg_t;
#define	KGDB_NUMREGS	66	/* from tm-alpha.h, NUM_REGS */
#define	KGDB_REG_V0	0
#define	KGDB_REG_T0	1
#define	KGDB_REG_T1	2
#define	KGDB_REG_T2	3
#define	KGDB_REG_T3	4
#define	KGDB_REG_T4	5
#define	KGDB_REG_T5	6
#define	KGDB_REG_T6	7
#define	KGDB_REG_T7	8
#define	KGDB_REG_S0	9
#define	KGDB_REG_S1	10
#define	KGDB_REG_S2	11
#define	KGDB_REG_S3	12
#define	KGDB_REG_S4	13
#define	KGDB_REG_S5	14
#define	KGDB_REG_S6	15	/* FP */
#define	KGDB_REG_A0	16
#define	KGDB_REG_A1	17
#define	KGDB_REG_A2	18
#define	KGDB_REG_A3	19
#define	KGDB_REG_A4	20
#define	KGDB_REG_A5	21
#define	KGDB_REG_T8	22
#define	KGDB_REG_T9	23
#define	KGDB_REG_T10	24
#define	KGDB_REG_T11	25
#define	KGDB_REG_RA	26
#define	KGDB_REG_T12	27
#define	KGDB_REG_AT	28
#define	KGDB_REG_GP	29
#define	KGDB_REG_SP	30
#define	KGDB_REG_ZERO	31
#define	KGDB_REG_F0	32
#define	KGDB_REG_F1	33
#define	KGDB_REG_F2	34
#define	KGDB_REG_F3	35
#define	KGDB_REG_F4	36
#define	KGDB_REG_F5	37
#define	KGDB_REG_F6	38
#define	KGDB_REG_F7	39
#define	KGDB_REG_F8	40
#define	KGDB_REG_F9	41
#define	KGDB_REG_F10	42
#define	KGDB_REG_F11	43
#define	KGDB_REG_F12	44
#define	KGDB_REG_F13	45
#define	KGDB_REG_F14	46
#define	KGDB_REG_F15	47
#define	KGDB_REG_F16	48
#define	KGDB_REG_F17	49
#define	KGDB_REG_F18	50
#define	KGDB_REG_F19	51
#define	KGDB_REG_F20	52
#define	KGDB_REG_F21	53
#define	KGDB_REG_F22	54
#define	KGDB_REG_F23	55
#define	KGDB_REG_F24	56
#define	KGDB_REG_F25	57
#define	KGDB_REG_F26	58
#define	KGDB_REG_F27	59
#define	KGDB_REG_F28	60
#define	KGDB_REG_F29	61
#define	KGDB_REG_F30	62
#define	KGDB_REG_F31	63
#define	KGDB_REG_PC	64
#define	KGDB_REG_VFP	65

/* Too much?  Must be large enough for register transfer. */
#define	KGDB_BUFLEN	1024

#endif	/* _IA64_DB_MACHDEP_H_ */
