/*        $NetBSD */

/*-
 * Copyright (c) 1996-1997 John D. Polstra.
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
 * $FreeBSD: src/sys/ia64/include/elf.h,v 1.11 2003/09/25 01:10:24 peter Exp $
 */


#define	ELF32_MACHDEP_ENDIANNESS	XXX	/* break compilation */
#define	ELF32_MACHDEP_ID_CASES						\
		/* no 32-bit ELF machine types supported until 32bit emu */

#define	ELF64_MACHDEP_ENDIANNESS	ELFDATA2LSB
#define	ELF64_MACHDEP_ID_CASES						\
		case EM_IA_64:						\
			break;

#define	ELF64_MACHDEP_ID	EM_IA_64	/* XXX */

#define ARCH_ELFSIZE		64	/* MD native binary size */

/*
 * Relocation types.
 */

/*	Name			Value	   Field	Calculation */
#define	R_IA64_NONE		0	/* None */
#define	R_IA64_IMM14		0x21	/* immediate14	S + A */
#define	R_IA64_IMM22		0x22	/* immediate22	S + A */
#define	R_IA64_IMM64		0x23	/* immediate64	S + A */
#define	R_IA64_DIR32MSB		0x24	/* word32 MSB	S + A */
#define	R_IA64_DIR32LSB		0x25	/* word32 LSB	S + A */
#define	R_IA64_DIR64MSB		0x26	/* word64 MSB	S + A */
#define	R_IA64_DIR64LSB		0x27	/* word64 LSB	S + A */
#define	R_IA64_GPREL22		0x2a	/* immediate22	@gprel(S + A) */
#define	R_IA64_GPREL64I		0x2b	/* immediate64	@gprel(S + A) */
#define	R_IA64_GPREL64MSB	0x2e	/* word64 MSB	@gprel(S + A) */
#define	R_IA64_GPREL64LSB	0x2f	/* word64 LSB	@gprel(S + A) */
#define	R_IA64_LTOFF22		0x32	/* immediate22	@ltoff(S + A) */
#define	R_IA64_LTOFF64I		0x33	/* immediate64	@ltoff(S + A) */
#define	R_IA64_PLTOFF22		0x3a	/* immediate22	@pltoff(S + A) */
#define	R_IA64_PLTOFF64I	0x3b	/* immediate64	@pltoff(S + A) */
#define	R_IA64_PLTOFF64MSB	0x3e	/* word64 MSB	@pltoff(S + A) */
#define	R_IA64_PLTOFF64LSB	0x3f	/* word64 LSB	@pltoff(S + A) */
#define	R_IA64_FPTR64I		0x43	/* immediate64	@fptr(S + A) */
#define	R_IA64_FPTR32MSB	0x44	/* word32 MSB	@fptr(S + A) */
#define	R_IA64_FPTR32LSB	0x45	/* word32 LSB	@fptr(S + A) */
#define	R_IA64_FPTR64MSB	0x46	/* word64 MSB	@fptr(S + A) */
#define	R_IA64_FPTR64LSB	0x47	/* word64 LSB	@fptr(S + A) */
#define	R_IA64_PCREL21B		0x49	/* immediate21 form1 S + A - P */
#define	R_IA64_PCREL21M		0x4a	/* immediate21 form2 S + A - P */
#define	R_IA64_PCREL21F		0x4b	/* immediate21 form3 S + A - P */
#define	R_IA64_PCREL32MSB	0x4c	/* word32 MSB	S + A - P */
#define	R_IA64_PCREL32LSB	0x4d	/* word32 LSB	S + A - P */
#define	R_IA64_PCREL64MSB	0x4e	/* word64 MSB	S + A - P */
#define	R_IA64_PCREL64LSB	0x4f	/* word64 LSB	S + A - P */
#define	R_IA64_LTOFF_FPTR22	0x52	/* immediate22	@ltoff(@fptr(S + A)) */
#define	R_IA64_LTOFF_FPTR64I	0x53	/* immediate64	@ltoff(@fptr(S + A)) */
#define	R_IA64_LTOFF_FPTR32MSB	0x54	/* word32 MSB	@ltoff(@fptr(S + A)) */
#define	R_IA64_LTOFF_FPTR32LSB	0x55	/* word32 LSB	@ltoff(@fptr(S + A)) */
#define	R_IA64_LTOFF_FPTR64MSB	0x56	/* word64 MSB	@ltoff(@fptr(S + A)) */
#define	R_IA64_LTOFF_FPTR64LSB	0x57	/* word64 LSB	@ltoff(@fptr(S + A)) */
#define	R_IA64_SEGREL32MSB	0x5c	/* word32 MSB	@segrel(S + A) */
#define	R_IA64_SEGREL32LSB	0x5d	/* word32 LSB	@segrel(S + A) */
#define	R_IA64_SEGREL64MSB	0x5e	/* word64 MSB	@segrel(S + A) */
#define	R_IA64_SEGREL64LSB	0x5f	/* word64 LSB	@segrel(S + A) */
#define	R_IA64_SECREL32MSB	0x64	/* word32 MSB	@secrel(S + A) */
#define	R_IA64_SECREL32LSB	0x65	/* word32 LSB	@secrel(S + A) */
#define	R_IA64_SECREL64MSB	0x66	/* word64 MSB	@secrel(S + A) */
#define	R_IA64_SECREL64LSB	0x67	/* word64 LSB	@secrel(S + A) */
#define	R_IA64_REL32MSB		0x6c	/* word32 MSB	BD + A */
#define	R_IA64_REL32LSB		0x6d	/* word32 LSB	BD + A */
#define	R_IA64_REL64MSB		0x6e	/* word64 MSB	BD + A */
#define	R_IA64_REL64LSB		0x6f	/* word64 LSB	BD + A */
#define	R_IA64_LTV32MSB		0x74	/* word32 MSB	S + A */
#define	R_IA64_LTV32LSB		0x75	/* word32 LSB	S + A */
#define	R_IA64_LTV64MSB		0x76	/* word64 MSB	S + A */
#define	R_IA64_LTV64LSB		0x77	/* word64 LSB	S + A */
#define	R_IA64_IPLTMSB		0x80	/* function descriptor MSB special */
#define	R_IA64_IPLTLSB		0x81	/* function descriptor LSB speciaal */
#define	R_IA64_SUB		0x85	/* immediate64	A - S */
#define	R_IA64_LTOFF22X		0x86	/* immediate22	special */
#define	R_IA64_LDXMOV		0x87	/* immediate22	special */
#define	R_IA64_TPREL14		0x91	/* imm14	@tprel(S + A) */
#define	R_IA64_TPREL22		0x92	/* imm22	@tprel(S + A) */
#define	R_IA64_TPREL64I		0x93	/* imm64	@tprel(S + A) */
#define	R_IA64_TPREL64MSB	0x96	/* word64 MSB	@tprel(S + A) */
#define	R_IA64_TPREL64LSB	0x97	/* word64 LSB	@tprel(S + A) */
#define	R_IA64_LTOFF_TPREL22	0x9a	/* imm22	@ltoff(@tprel(S+A)) */
#define	R_IA64_DTPMOD64MSB	0xa6	/* word64 MSB	@dtpmod(S + A) */
#define	R_IA64_DTPMOD64LSB	0xa7	/* word64 LSB	@dtpmod(S + A) */
#define	R_IA64_LTOFF_DTPMOD22	0xaa	/* imm22	@ltoff(@dtpmod(S+A)) */
#define	R_IA64_DTPREL14		0xb1	/* imm14	@dtprel(S + A) */
#define	R_IA64_DTPREL22		0xb2	/* imm22	@dtprel(S + A) */
#define	R_IA64_DTPREL64I	0xb3	/* imm64	@dtprel(S + A) */
#define	R_IA64_DTPREL32MSB	0xb4	/* word32 MSB	@dtprel(S + A) */
#define	R_IA64_DTPREL32LSB	0xb5	/* word32 LSB	@dtprel(S + A) */
#define	R_IA64_DTPREL64MSB	0xb6	/* word64 MSB	@dtprel(S + A) */
#define	R_IA64_DTPREL64LSB	0xb7	/* word64 LSB	@dtprel(S + A) */
#define	R_IA64_LTOFF_DTPREL22	0xba	/* imm22	@ltoff(@dtprel(S+A)) */

/* p_type */

#define PT_IA_64_ARCHEXT 0x70000000	/* segment contains a section of type SHT_IA_64_EXT */
#define PT_IA_64_UNWIND  0x70000001	/* segment contains the stack unwind tables */

/* p_flags */

#define PF_IA_64_NORECOV 0x80000000	/* segment contains the stack unwind tables */

/* sh_type */

#define SHT_IA_64_EXT           0x70000000	/* section contains product specific extension bits */
#define SHT_IA_64_UNWIND        0x70000001	/* section contains unwind function table entries for stack unwinding */
#define SHT_IA_64_LOPSREG       0x78000000	/* reserved for implementation-specific section types */
#define SHT_IA_64_HIPSREG       0x7fffffff	/* Ditto */
#define SHT_IA_64_PRIORITY_INIT 0x79000000	/* section contains priority initialization record */

/* sh_flags */

#define SHF_IA_64_SHORT   0x10000000	/* section must be placed near gp. */
#define SHF_IA_64_NORECOV 0x20000000	/* section contains code that uses speculative instructions without
					 * recovery code
					 */



