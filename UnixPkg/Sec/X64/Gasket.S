#------------------------------------------------------------------------------
#
# This template was generated from GasketEfiTemplate.c Unix x86_64 ABI
#
# Efi Prefix means it has been ported to gasket EFIABI to Unix x86_64 ABI
#
# The EFI_UNIX_THUNK_PROTOCOL member functions call these these generic assembly 
# routines.  
#
# Some OS X POSIX calls get name mangled in C code and we need to fill in a C global
# to get the correct binding (does not work from assembly). So we have 4 functions
# that do an indirect call, while the others call POSIX APIs directly
#
# movq	_gUnixRmDir@GOTPCREL(%rip), %rax 
#
#
# UNIX Arg passing: RCX, RDX,  R8,  R9
# EFI  Arg passing: RDI, RDI, RDX, RCX, R8, R9
#
# RSI, RDI calle-save on EFI, scatch on UNIX callign
#
#
# Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#------------------------------------------------------------------------------
#ifdef __APPLE__
	.section __DWARF,__debug_frame,regular,debug
Lsection__debug_frame:
	.section __DWARF,__debug_info,regular,debug
Lsection__debug_info:
	.section __DWARF,__debug_abbrev,regular,debug
Lsection__debug_abbrev:
	.section __DWARF,__debug_aranges,regular,debug
Lsection__debug_aranges:
	.section __DWARF,__debug_macinfo,regular,debug
Lsection__debug_macinfo:
	.section __DWARF,__debug_line,regular,debug
Lsection__debug_line:
	.section __DWARF,__debug_loc,regular,debug
Lsection__debug_loc:
	.section __DWARF,__debug_pubnames,regular,debug
Lsection__debug_pubnames:
	.section __DWARF,__debug_pubtypes,regular,debug
Lsection__debug_pubtypes:
	.section __DWARF,__debug_inlined,regular,debug
Lsection__debug_inlined:
	.section __DWARF,__debug_str,regular,debug
Lsection__debug_str:
	.section __DWARF,__debug_ranges,regular,debug
Lsection__debug_ranges:
	.section __DWARF,__debug_abbrev,regular,debug
Ldebug_abbrev0:
	.section __DWARF,__debug_info,regular,debug
Ldebug_info0:
	.section __DWARF,__debug_line,regular,debug
Ldebug_line0:
#endif

	.text
Ltext0:
.globl _Gasketrmdir
_Gasketrmdir:
LFB16:
	.file 1 "/Users/fish/work/edk2/UnixPkg/Sec/X64/GasketEfiTemplate.c"
	.loc 1 35 0
	pushq	%rbp
LCFI0:
	movq	%rsp, %rbp
LCFI1:
	subq	$16, %rsp
LCFI2:
	movq	%rdi, -8(%rbp)
	.loc 1 36 0
	movq	_gUnixRmDir@GOTPCREL(%rip), %rax
	movq	(%rax), %rax
	movq	-8(%rbp), %rdi
	call	*%rax
	.loc 1 37 0
	leave
	ret
	
LFE16:
.globl _Gasketopendir
_Gasketopendir:
LFB17:
	.loc 1 42 0
	pushq	%rbp
LCFI3:
	movq	%rsp, %rbp
LCFI4:
	subq	$16, %rsp
LCFI5:
	movq	%rdi, -8(%rbp)
	.loc 1 43 0
	movq	_gUnixOpenDir@GOTPCREL(%rip), %rax
	movq	(%rax), %rax
	movq	-8(%rbp), %rdi
	call	*%rax
	.loc 1 44 0
	leave
	ret
LFE17:
.globl _Gasketstat
_Gasketstat:
LFB18:
	.loc 1 49 0
	pushq	%rbp
LCFI6:
	movq	%rsp, %rbp
LCFI7:
	subq	$16, %rsp
LCFI8:
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	.loc 1 50 0
	movq	_gUnixStat@GOTPCREL(%rip), %rax
	movq	(%rax), %rax
	movq	-16(%rbp), %rsi
	movq	-8(%rbp), %rdi
	call	*%rax
	.loc 1 51 0
	leave
	ret
LFE18:
.globl _Gasketstatfs
_Gasketstatfs:
LFB19:
	.loc 1 56 0
	pushq	%rbp
LCFI9:
	movq	%rsp, %rbp
LCFI10:
	subq	$16, %rsp
LCFI11:
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	.loc 1 57 0
	movq	_gUnixStatFs@GOTPCREL(%rip), %rax
	movq	(%rax), %rax
	movq	-16(%rbp), %rsi
	movq	-8(%rbp), %rdi
	call	*%rax
	.loc 1 58 0
	leave
	ret
LFE19:
.globl _GasketmsSleep
_GasketmsSleep:
LFB20:
	.loc 1 65 0
	pushq	%rbp
LCFI12:
	movq	%rsp, %rbp
LCFI13:
	subq	$16, %rsp
LCFI14:
	movq	%rdi, -8(%rbp)
	.loc 1 66 0
	movq	-8(%rbp), %rdi
	call	_msSleep
	.loc 1 68 0
	leave
	ret
LFE20:
.globl _Gasketexit
_Gasketexit:
LFB21:
	.loc 1 72 0
	pushq	%rbp
LCFI15:
	movq	%rsp, %rbp
LCFI16:
	subq	$16, %rsp
LCFI17:
	movl	%edi, -4(%rbp)
	.loc 1 73 0
	movl	-4(%rbp), %edi
	call	_exit
LFE21:
.globl _GasketSetTimer
_GasketSetTimer:
LFB22:
	.loc 1 80 0
	pushq	%rbp
LCFI18:
	movq	%rsp, %rbp
LCFI19:
	subq	$16, %rsp
LCFI20:
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	.loc 1 81 0
	movq	-16(%rbp), %rsi
	movq	-8(%rbp), %rdi
	call	_SetTimer
	.loc 1 83 0
	leave
	ret
LFE22:
.globl _GasketGetLocalTime
_GasketGetLocalTime:
LFB23:
	.loc 1 88 0
	pushq	%rbp
LCFI21:
	movq	%rsp, %rbp
LCFI22:
	subq	$16, %rsp
LCFI23:
	movq	%rdi, -8(%rbp)
	.loc 1 89 0
	movq	-8(%rbp), %rdi
	call	_GetLocalTime
	.loc 1 91 0
	leave
	ret
LFE23:
.globl _Gasketgmtime
_Gasketgmtime:
LFB24:
	.loc 1 96 0
	pushq	%rbp
LCFI24:
	movq	%rsp, %rbp
LCFI25:
	subq	$16, %rsp
LCFI26:
	movq	%rdi, -8(%rbp)
	.loc 1 97 0
	movq	-8(%rbp), %rdi
	call	_localtime
	.loc 1 98 0
	leave
	ret
LFE24:
.globl _GasketGetTimeZone
_GasketGetTimeZone:
LFB25:
	.loc 1 103 0
	pushq	%rbp
LCFI27:
	movq	%rsp, %rbp
LCFI28:
	.loc 1 104 0
	call	_GetTimeZone
	.loc 1 105 0
	leave
	ret
LFE25:
.globl _GasketGetDayLight
_GasketGetDayLight:
LFB26:
	.loc 1 110 0
	pushq	%rbp
LCFI29:
	movq	%rsp, %rbp
LCFI30:
	.loc 1 111 0
	call	_GetDayLight
	.loc 1 112 0
	leave
	ret
LFE26:
.globl _Gasketpoll
_Gasketpoll:
LFB27:
	.loc 1 117 0
	pushq	%rbp
LCFI31:
	movq	%rsp, %rbp
LCFI32:
	subq	$16, %rsp
LCFI33:
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movl	%edx, -16(%rbp)
	.loc 1 118 0
	movl	-16(%rbp), %edx
	movl	-12(%rbp), %esi
	movq	-8(%rbp), %rdi
	call	_poll
	.loc 1 119 0
	leave
	ret
LFE27:
.globl _Gasketread
_Gasketread:
LFB28:
	.loc 1 124 0
	pushq	%rbp
LCFI34:
	movq	%rsp, %rbp
LCFI35:
	subq	$32, %rsp
LCFI36:
	movl	%edi, -4(%rbp)
	movq	%rsi, -16(%rbp)
	movl	%edx, -20(%rbp)
	.loc 1 125 0
	movl	-20(%rbp), %eax
	movslq	%eax,%rdx
	movq	-16(%rbp), %rsi
	movl	-4(%rbp), %edi
	call	_read
	.loc 1 126 0
	leave
	ret
LFE28:
.globl _Gasketwrite
_Gasketwrite:
LFB29:
	.loc 1 131 0
	pushq	%rbp
LCFI37:
	movq	%rsp, %rbp
LCFI38:
	subq	$32, %rsp
LCFI39:
	movl	%edi, -4(%rbp)
	movq	%rsi, -16(%rbp)
	movl	%edx, -20(%rbp)
	.loc 1 132 0
	movl	-20(%rbp), %eax
	movslq	%eax,%rdx
	movq	-16(%rbp), %rsi
	movl	-4(%rbp), %edi
	call	_write
	.loc 1 133 0
	leave
	ret
LFE29:
.globl _Gasketgetenv
_Gasketgetenv:
LFB30:
	.loc 1 138 0
	pushq	%rbp
LCFI40:
	movq	%rsp, %rbp
LCFI41:
	subq	$16, %rsp
LCFI42:
	movq	%rdi, -8(%rbp)
	.loc 1 139 0
	movq	-8(%rbp), %rdi
	call	_getenv
	.loc 1 140 0
	leave
	ret
LFE30:
.globl _Gasketopen
_Gasketopen:
LFB31:
	.loc 1 145 0
	pushq	%rbp
LCFI43:
	movq	%rsp, %rbp
LCFI44:
	subq	$16, %rsp
LCFI45:
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movl	%edx, -16(%rbp)
	.loc 1 146 0
	movl	-16(%rbp), %edx
	movl	-12(%rbp), %esi
	movq	-8(%rbp), %rdi
	movl	$0, %eax
	call	_open
	.loc 1 147 0
	leave
	ret
LFE31:
.globl _Gasketlseek
_Gasketlseek:
LFB32:
	.loc 1 152 0
	pushq	%rbp
LCFI46:
	movq	%rsp, %rbp
LCFI47:
	subq	$32, %rsp
LCFI48:
	movl	%edi, -4(%rbp)
	movq	%rsi, -16(%rbp)
	movl	%edx, -20(%rbp)
	.loc 1 153 0
	movl	-20(%rbp), %edx
	movq	-16(%rbp), %rsi
	movl	-4(%rbp), %edi
	call	_lseek
	.loc 1 154 0
	leave
	ret
LFE32:
.globl _Gasketftruncate
_Gasketftruncate:
LFB33:
	.loc 1 159 0
	pushq	%rbp
LCFI49:
	movq	%rsp, %rbp
LCFI50:
	subq	$16, %rsp
LCFI51:
	movl	%edi, -4(%rbp)
	movq	%rsi, -16(%rbp)
	.loc 1 160 0
	movq	-16(%rbp), %rsi
	movl	-4(%rbp), %edi
	call	_ftruncate
	.loc 1 161 0
	leave
	ret
LFE33:
.globl _Gasketclose
_Gasketclose:
LFB34:
	.loc 1 166 0
	pushq	%rbp
LCFI52:
	movq	%rsp, %rbp
LCFI53:
	subq	$16, %rsp
LCFI54:
	movl	%edi, -4(%rbp)
	.loc 1 167 0
	movl	-4(%rbp), %edi
	call	_close
	.loc 1 168 0
	leave
	ret
LFE34:
.globl _Gasketmkdir
_Gasketmkdir:
LFB35:
	.loc 1 173 0
	pushq	%rbp
LCFI55:
	movq	%rsp, %rbp
LCFI56:
	subq	$16, %rsp
LCFI57:
	movq	%rdi, -8(%rbp)
	movw	%si, -12(%rbp)
	.loc 1 174 0
	movzwl	-12(%rbp), %esi
	movq	-8(%rbp), %rdi
	call	_mkdir
	.loc 1 175 0
	leave
	ret
LFE35:
.globl _Gasketunlink
_Gasketunlink:
LFB36:
	.loc 1 180 0
	pushq	%rbp
LCFI58:
	movq	%rsp, %rbp
LCFI59:
	subq	$16, %rsp
LCFI60:
	movq	%rdi, -8(%rbp)
	.loc 1 181 0
	movq	-8(%rbp), %rdi
	call	_unlink
	.loc 1 182 0
	leave
	ret
LFE36:
.globl _GasketGetErrno
_GasketGetErrno:
LFB37:
	.loc 1 187 0
	pushq	%rbp
LCFI61:
	movq	%rsp, %rbp
LCFI62:
	.loc 1 188 0
	call	_GetErrno
	.loc 1 189 0
	leave
	ret
LFE37:
.globl _Gasketrewinddir
_Gasketrewinddir:
LFB38:
	.loc 1 194 0
	pushq	%rbp
LCFI63:
	movq	%rsp, %rbp
LCFI64:
	subq	$16, %rsp
LCFI65:
	movq	%rdi, -8(%rbp)
	.loc 1 195 0
	movq	-8(%rbp), %rdi
	call	_rewinddir$INODE64
	.loc 1 197 0
	leave
	ret
LFE38:
.globl _Gasketreaddir
_Gasketreaddir:
LFB39:
	.loc 1 202 0
	pushq	%rbp
LCFI66:
	movq	%rsp, %rbp
LCFI67:
	subq	$16, %rsp
LCFI68:
	movq	%rdi, -8(%rbp)
	.loc 1 203 0
	movq	-8(%rbp), %rdi
	call	_readdir$INODE64
	.loc 1 204 0
	leave
	ret
LFE39:
.globl _Gasketclosedir
_Gasketclosedir:
LFB40:
	.loc 1 209 0
	pushq	%rbp
LCFI69:
	movq	%rsp, %rbp
LCFI70:
	subq	$16, %rsp
LCFI71:
	movq	%rdi, -8(%rbp)
	.loc 1 210 0
	movq	-8(%rbp), %rdi
	call	_closedir
	.loc 1 211 0
	leave
	ret
LFE40:
.globl _Gasketrename
_Gasketrename:
LFB41:
	.loc 1 216 0
	pushq	%rbp
LCFI72:
	movq	%rsp, %rbp
LCFI73:
	subq	$16, %rsp
LCFI74:
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	.loc 1 217 0
	movq	-16(%rbp), %rsi
	movq	-8(%rbp), %rdi
	call	_rename
	.loc 1 218 0
	leave
	ret
LFE41:
.globl _Gasketmktime
_Gasketmktime:
LFB42:
	.loc 1 223 0
	pushq	%rbp
LCFI75:
	movq	%rsp, %rbp
LCFI76:
	subq	$16, %rsp
LCFI77:
	movq	%rdi, -8(%rbp)
	.loc 1 224 0
	movq	-8(%rbp), %rdi
	call	_mktime
	.loc 1 225 0
	leave
	ret
LFE42:
.globl _Gasketfsync
_Gasketfsync:
LFB43:
	.loc 1 230 0
	pushq	%rbp
LCFI78:
	movq	%rsp, %rbp
LCFI79:
	subq	$16, %rsp
LCFI80:
	movl	%edi, -4(%rbp)
	.loc 1 231 0
	movl	-4(%rbp), %edi
	call	_fsync
	.loc 1 232 0
	leave
	ret
LFE43:
.globl _Gasketchmod
_Gasketchmod:
LFB44:
	.loc 1 237 0
	pushq	%rbp
LCFI81:
	movq	%rsp, %rbp
LCFI82:
	subq	$16, %rsp
LCFI83:
	movq	%rdi, -8(%rbp)
	movw	%si, -12(%rbp)
	.loc 1 238 0
	movzwl	-12(%rbp), %esi
	movq	-8(%rbp), %rdi
	call	_chmod
	.loc 1 239 0
	leave
	ret
LFE44:
.globl _Gasketutime
_Gasketutime:
LFB45:
	.loc 1 244 0
	pushq	%rbp
LCFI84:
	movq	%rsp, %rbp
LCFI85:
	subq	$16, %rsp
LCFI86:
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	.loc 1 245 0
	movq	-16(%rbp), %rsi
	movq	-8(%rbp), %rdi
	call	_utime
	.loc 1 246 0
	leave
	ret
LFE45:
.globl _Gaskettcflush
_Gaskettcflush:
LFB46:
	.loc 1 251 0
	pushq	%rbp
LCFI87:
	movq	%rsp, %rbp
LCFI88:
	subq	$16, %rsp
LCFI89:
	movl	%edi, -4(%rbp)
	movl	%esi, -8(%rbp)
	.loc 1 252 0
	movl	-8(%rbp), %esi
	movl	-4(%rbp), %edi
	call	_tcflush
	.loc 1 253 0
	leave
	ret
LFE46:
.globl _GasketUgaCreate
_GasketUgaCreate:
LFB47:
	.loc 1 258 0
	pushq	%rbp
LCFI90:
	movq	%rsp, %rbp
LCFI91:
	subq	$16, %rsp
LCFI92:
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	.loc 1 259 0
	movq	-16(%rbp), %rsi
	movq	-8(%rbp), %rdi
	call	_UgaCreate
	.loc 1 260 0
	leave
	ret
LFE47:
.globl _Gasketperror
_Gasketperror:
LFB48:
	.loc 1 265 0
	pushq	%rbp
LCFI93:
	movq	%rsp, %rbp
LCFI94:
	subq	$16, %rsp
LCFI95:
	movq	%rdi, -8(%rbp)
	.loc 1 266 0
	movq	-8(%rbp), %rdi
	call	_perror
	.loc 1 268 0
	leave
	ret
LFE48:
.globl _Gasketioctl
_Gasketioctl:
LFB49:
	.loc 1 277 0
	pushq	%rbp
LCFI96:
	movq	%rsp, %rbp
LCFI97:
	subq	$240, %rsp
LCFI98:
	movl	%edi, -212(%rbp)
	movq	%rdx, -160(%rbp)
	movq	%rcx, -152(%rbp)
	movq	%r8, -144(%rbp)
	movq	%r9, -136(%rbp)
	movzbl	%al, %eax
	movq	%rax, -240(%rbp)
	movq	-240(%rbp), %rdx
	leaq	0(,%rdx,4), %rax
	leaq	L68(%rip), %rdx
	movq	%rdx, -240(%rbp)
	subq	%rax, -240(%rbp)
	leaq	-1(%rbp), %rax
	movq	-240(%rbp), %rdx
	jmp	*%rdx
	movaps	%xmm7, -15(%rax)
	movaps	%xmm6, -31(%rax)
	movaps	%xmm5, -47(%rax)
	movaps	%xmm4, -63(%rax)
	movaps	%xmm3, -79(%rax)
	movaps	%xmm2, -95(%rax)
	movaps	%xmm1, -111(%rax)
	movaps	%xmm0, -127(%rax)
L68:
	movq	%rsi, -224(%rbp)
	.loc 1 280 0
	leaq	-208(%rbp), %rax
	movl	$16, (%rax)
	leaq	-208(%rbp), %rax
	movl	$48, 4(%rax)
	leaq	-208(%rbp), %rax
	leaq	16(%rbp), %rdx
	movq	%rdx, 8(%rax)
	leaq	-208(%rbp), %rax
	leaq	-176(%rbp), %rdx
	movq	%rdx, 16(%rax)
	.loc 1 281 0
	movl	-208(%rbp), %eax
	cmpl	$48, %eax
	jae	L69
	movq	-192(%rbp), %rdx
	movl	-208(%rbp), %eax
	mov	%eax, %eax
	addq	%rax, %rdx
	movq	%rdx, -232(%rbp)
	movl	-208(%rbp), %eax
	addl	$8, %eax
	movl	%eax, -208(%rbp)
	jmp	L71
L69:
	movq	-200(%rbp), %rax
	movq	%rax, -232(%rbp)
	addq	$8, %rax
	movq	%rax, -200(%rbp)
L71:
	movq	-232(%rbp), %rax
	movq	(%rax), %rdx
	movq	-224(%rbp), %rsi
	movl	-212(%rbp), %edi
	movl	$0, %eax
	call	_ioctl
	.loc 1 282 0
	leave
	ret
LFE49:
.globl _Gasketfcntl
_Gasketfcntl:
LFB50:
	.loc 1 287 0
	pushq	%rbp
LCFI99:
	movq	%rsp, %rbp
LCFI100:
	subq	$240, %rsp
LCFI101:
	movl	%edi, -212(%rbp)
	movq	%rdx, -160(%rbp)
	movq	%rcx, -152(%rbp)
	movq	%r8, -144(%rbp)
	movq	%r9, -136(%rbp)
	movzbl	%al, %eax
	movq	%rax, -232(%rbp)
	movq	-232(%rbp), %rdx
	leaq	0(,%rdx,4), %rax
	leaq	L74(%rip), %rdx
	movq	%rdx, -232(%rbp)
	subq	%rax, -232(%rbp)
	leaq	-1(%rbp), %rax
	movq	-232(%rbp), %rdx
	jmp	*%rdx
	movaps	%xmm7, -15(%rax)
	movaps	%xmm6, -31(%rax)
	movaps	%xmm5, -47(%rax)
	movaps	%xmm4, -63(%rax)
	movaps	%xmm3, -79(%rax)
	movaps	%xmm2, -95(%rax)
	movaps	%xmm1, -111(%rax)
	movaps	%xmm0, -127(%rax)
L74:
	movl	%esi, -216(%rbp)
	.loc 1 290 0
	leaq	-208(%rbp), %rax
	movl	$16, (%rax)
	leaq	-208(%rbp), %rax
	movl	$48, 4(%rax)
	leaq	-208(%rbp), %rax
	leaq	16(%rbp), %rdx
	movq	%rdx, 8(%rax)
	leaq	-208(%rbp), %rax
	leaq	-176(%rbp), %rdx
	movq	%rdx, 16(%rax)
	.loc 1 291 0
	movl	-208(%rbp), %eax
	cmpl	$48, %eax
	jae	L75
	movq	-192(%rbp), %rdx
	movl	-208(%rbp), %eax
	mov	%eax, %eax
	addq	%rax, %rdx
	movq	%rdx, -224(%rbp)
	movl	-208(%rbp), %eax
	addl	$8, %eax
	movl	%eax, -208(%rbp)
	jmp	L77
L75:
	movq	-200(%rbp), %rax
	movq	%rax, -224(%rbp)
	addq	$8, %rax
	movq	%rax, -200(%rbp)
L77:
	movq	-224(%rbp), %rax
	movq	(%rax), %rdx
	movl	-216(%rbp), %esi
	movl	-212(%rbp), %edi
	movl	$0, %eax
	call	_fcntl
	.loc 1 292 0
	leave
	ret
LFE50:
.globl _Gasketcfsetispeed
_Gasketcfsetispeed:
LFB51:
	.loc 1 298 0
	pushq	%rbp
LCFI102:
	movq	%rsp, %rbp
LCFI103:
	subq	$16, %rsp
LCFI104:
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	.loc 1 299 0
	movq	-16(%rbp), %rsi
	movq	-8(%rbp), %rdi
	call	_cfsetispeed
	.loc 1 300 0
	leave
	ret
LFE51:
.globl _Gasketcfsetospeed
_Gasketcfsetospeed:
LFB52:
	.loc 1 305 0
	pushq	%rbp
LCFI105:
	movq	%rsp, %rbp
LCFI106:
	subq	$16, %rsp
LCFI107:
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	.loc 1 306 0
	movq	-16(%rbp), %rsi
	movq	-8(%rbp), %rdi
	call	_cfsetospeed
	.loc 1 307 0
	leave
	ret
LFE52:
.globl _Gaskettcgetattr
_Gaskettcgetattr:
LFB53:
	.loc 1 312 0
	pushq	%rbp
LCFI108:
	movq	%rsp, %rbp
LCFI109:
	subq	$16, %rsp
LCFI110:
	movl	%edi, -4(%rbp)
	movq	%rsi, -16(%rbp)
	.loc 1 313 0
	movq	-16(%rbp), %rsi
	movl	-4(%rbp), %edi
	call	_tcgetattr
	.loc 1 314 0
	leave
	ret
LFE53:
.globl _Gaskettcsetattr
_Gaskettcsetattr:
LFB54:
	.loc 1 319 0
	pushq	%rbp
LCFI111:
	movq	%rsp, %rbp
LCFI112:
	subq	$16, %rsp
LCFI113:
	movl	%edi, -4(%rbp)
	movl	%esi, -8(%rbp)
	movq	%rdx, -16(%rbp)
	.loc 1 320 0
	movq	-16(%rbp), %rdx
	movl	-8(%rbp), %esi
	movl	-4(%rbp), %edi
	call	_tcsetattr
	.loc 1 321 0
	leave
	ret
LFE54:
.globl _GasketUnixPeCoffGetEntryPoint
_GasketUnixPeCoffGetEntryPoint:
LFB55:
	.loc 1 331 0
	pushq	%rbp
LCFI114:
	movq	%rsp, %rbp
LCFI115:
	subq	$16, %rsp
LCFI116:
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	.loc 1 332 0
	movq	-16(%rbp), %rsi
	movq	-8(%rbp), %rdi
	call	_SecPeCoffGetEntryPoint
	.loc 1 333 0
	leave
	ret
LFE55:
.globl _GasketUnixPeCoffRelocateImageExtraAction
_GasketUnixPeCoffRelocateImageExtraAction:
LFB56:
	.loc 1 341 0
	pushq	%rbp
LCFI117:
	movq	%rsp, %rbp
LCFI118:
	subq	$16, %rsp
LCFI119:
	movq	%rdi, -8(%rbp)
	.loc 1 342 0
	movq	-8(%rbp), %rdi
	call	_SecPeCoffRelocateImageExtraAction
	.loc 1 344 0
	leave
	ret
LFE56:
.globl _GasketUnixPeCoffUnloadImageExtraAction
_GasketUnixPeCoffUnloadImageExtraAction:
LFB57:
	.loc 1 352 0
	pushq	%rbp
LCFI120:
	movq	%rsp, %rbp
LCFI121:
	subq	$16, %rsp
LCFI122:
	movq	%rdi, -8(%rbp)
	.loc 1 353 0
	movq	-8(%rbp), %rdi
	call	_SecPeCoffLoaderUnloadImageExtraAction
	.loc 1 355 0
	leave
	ret
LFE57:
.globl _GasketUgaClose
_GasketUgaClose:
LFB58:
	.loc 1 365 0
	pushq	%rbp
LCFI123:
	movq	%rsp, %rbp
LCFI124:
	subq	$16, %rsp
LCFI125:
	movq	%rdi, -8(%rbp)
	.loc 1 366 0
	movq	-8(%rbp), %rdi
	call	_UgaClose
	.loc 1 367 0
	leave
	ret
LFE58:
.globl _GasketUgaSize
_GasketUgaSize:
LFB59:
	.loc 1 372 0
	pushq	%rbp
LCFI126:
	movq	%rsp, %rbp
LCFI127:
	subq	$16, %rsp
LCFI128:
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movl	%edx, -16(%rbp)
	.loc 1 373 0
	movl	-16(%rbp), %edx
	movl	-12(%rbp), %esi
	movq	-8(%rbp), %rdi
	call	_UgaSize
	.loc 1 374 0
	leave
	ret
LFE59:
.globl _GasketUgaCheckKey
_GasketUgaCheckKey:
LFB60:
	.loc 1 379 0
	pushq	%rbp
LCFI129:
	movq	%rsp, %rbp
LCFI130:
	subq	$16, %rsp
LCFI131:
	movq	%rdi, -8(%rbp)
	.loc 1 380 0
	movq	-8(%rbp), %rdi
	call	_UgaCheckKey
	.loc 1 381 0
	leave
	ret
LFE60:
.globl _GasketUgaGetKey
_GasketUgaGetKey:
LFB61:
	.loc 1 386 0
	pushq	%rbp
LCFI132:
	movq	%rsp, %rbp
LCFI133:
	subq	$16, %rsp
LCFI134:
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	.loc 1 387 0
	movq	-16(%rbp), %rsi
	movq	-8(%rbp), %rdi
	call	_UgaGetKey
	.loc 1 388 0
	leave
	ret
LFE61:
.globl _GasketUgaBlt
_GasketUgaBlt:
LFB62:
	.loc 1 404 0
	pushq	%rbp
LCFI135:
	movq	%rsp, %rbp
LCFI136:
	subq	$80, %rsp
LCFI137:
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movl	%edx, -20(%rbp)
	movq	%rcx, -32(%rbp)
	movq	%r8, -40(%rbp)
	movq	%r9, -48(%rbp)
	.loc 1 405 0
	movq	-48(%rbp), %rdx
	movq	-40(%rbp), %rcx
	movq	-32(%rbp), %rsi
	movl	-20(%rbp), %edi
	movq	-16(%rbp), %r10
	movq	-8(%rbp), %r11
	movq	40(%rbp), %rax
	movq	%rax, 24(%rsp)
	movq	32(%rbp), %rax
	movq	%rax, 16(%rsp)
	movq	24(%rbp), %rax
	movq	%rax, 8(%rsp)
	movq	16(%rbp), %rax
	movq	%rax, (%rsp)
	movq	%rdx, %r9
	movq	%rcx, %r8
	movq	%rsi, %rcx
	movl	%edi, %edx
	movq	%r10, %rsi
	movq	%r11, %rdi
	call	_UgaBlt
	.loc 1 406 0
	leave
	ret
LFE62:

.globl _ReverseGasketUint64
_ReverseGasketUint64:
LFB63:
	.loc 1 413 0
	pushq	%rbp
LCFI138:
	movq	%rsp, %rbp
LCFI139:
	subq	$32, %rsp
LCFI140:
	movq	%rdi, -24(%rbp)
	movq	%rsi, -32(%rbp)
	.loc 1 414 0
	movq	-24(%rbp), %rax
	movq	%rax, -8(%rbp)
	.loc 1 416 0
	movq	-32(%rbp), %rdi
	movq	-8(%rbp), %rax
	call	*%rax
	.loc 1 417 0
	movl	$0, %eax
	.loc 1 418 0
	leave
	ret
	
.globl _EfiReverseGasketUint64
_EfiReverseGasketUint64:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$64, %rsp
	movq	%rdi, -24(%rbp)
	movq	%rsi, -32(%rbp)
	movq	-24(%rbp), %rax
	movq	%rax, -8(%rbp)
	movq	-32(%rbp), %rcx
	movq	-8(%rbp), %rax
	call	*%rax
	leave
	ret
	
// Sec PPI Callbacks

.globl _GasketSecUnixPeiLoadFile
_GasketSecUnixPeiLoadFile:
	jmp	  _SecUnixPeiLoadFile


.globl _GasketSecUnixPeiAutoScan
_GasketSecUnixPeiAutoScan:
	jmp	  _SecUnixPeiAutoScan


.globl _GasketSecUnixUnixThunkAddress
_GasketSecUnixUnixThunkAddress:
	jmp	  _SecUnixUnixThunkAddress


.globl _GasketSecPeiReportStatusCode
_GasketSecPeiReportStatusCode:
	jmp	  _SecPeiReportStatusCode


.globl _GasketSecUnixFdAddress
_GasketSecUnixFdAddress:
	jmp	  _SecUnixFdAddress


.globl _GasketSecTemporaryRamSupport
_GasketSecTemporaryRamSupport:
	jmp	  _SecTemporaryRamSupport

	
	
#if __APPLE__

LFE63:
	.section __DWARF,__debug_frame,regular,debug
Lframe0:
	.set L$set$0,LECIE0-LSCIE0
	.long L$set$0
LSCIE0:
	.long	0xffffffff
	.byte	0x1
	.ascii "\0"
	.byte	0x1
	.byte	0x78
	.byte	0x10
	.byte	0xc
	.byte	0x7
	.byte	0x8
	.byte	0x90
	.byte	0x1
	.align 3
LECIE0:
LSFDE0:
	.set L$set$1,LEFDE0-LASFDE0
	.long L$set$1
LASFDE0:
	.set L$set$2,Lframe0-Lsection__debug_frame
	.long L$set$2
	.quad	LFB16
	.set L$set$3,LFE16-LFB16
	.quad L$set$3
	.byte	0x4
	.set L$set$4,LCFI0-LFB16
	.long L$set$4
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$5,LCFI1-LCFI0
	.long L$set$5
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE0:
LSFDE2:
	.set L$set$6,LEFDE2-LASFDE2
	.long L$set$6
LASFDE2:
	.set L$set$7,Lframe0-Lsection__debug_frame
	.long L$set$7
	.quad	LFB17
	.set L$set$8,LFE17-LFB17
	.quad L$set$8
	.byte	0x4
	.set L$set$9,LCFI3-LFB17
	.long L$set$9
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$10,LCFI4-LCFI3
	.long L$set$10
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE2:
LSFDE4:
	.set L$set$11,LEFDE4-LASFDE4
	.long L$set$11
LASFDE4:
	.set L$set$12,Lframe0-Lsection__debug_frame
	.long L$set$12
	.quad	LFB18
	.set L$set$13,LFE18-LFB18
	.quad L$set$13
	.byte	0x4
	.set L$set$14,LCFI6-LFB18
	.long L$set$14
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$15,LCFI7-LCFI6
	.long L$set$15
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE4:
LSFDE6:
	.set L$set$16,LEFDE6-LASFDE6
	.long L$set$16
LASFDE6:
	.set L$set$17,Lframe0-Lsection__debug_frame
	.long L$set$17
	.quad	LFB19
	.set L$set$18,LFE19-LFB19
	.quad L$set$18
	.byte	0x4
	.set L$set$19,LCFI9-LFB19
	.long L$set$19
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$20,LCFI10-LCFI9
	.long L$set$20
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE6:
LSFDE8:
	.set L$set$21,LEFDE8-LASFDE8
	.long L$set$21
LASFDE8:
	.set L$set$22,Lframe0-Lsection__debug_frame
	.long L$set$22
	.quad	LFB20
	.set L$set$23,LFE20-LFB20
	.quad L$set$23
	.byte	0x4
	.set L$set$24,LCFI12-LFB20
	.long L$set$24
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$25,LCFI13-LCFI12
	.long L$set$25
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE8:
LSFDE10:
	.set L$set$26,LEFDE10-LASFDE10
	.long L$set$26
LASFDE10:
	.set L$set$27,Lframe0-Lsection__debug_frame
	.long L$set$27
	.quad	LFB21
	.set L$set$28,LFE21-LFB21
	.quad L$set$28
	.byte	0x4
	.set L$set$29,LCFI15-LFB21
	.long L$set$29
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$30,LCFI16-LCFI15
	.long L$set$30
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE10:
LSFDE12:
	.set L$set$31,LEFDE12-LASFDE12
	.long L$set$31
LASFDE12:
	.set L$set$32,Lframe0-Lsection__debug_frame
	.long L$set$32
	.quad	LFB22
	.set L$set$33,LFE22-LFB22
	.quad L$set$33
	.byte	0x4
	.set L$set$34,LCFI18-LFB22
	.long L$set$34
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$35,LCFI19-LCFI18
	.long L$set$35
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE12:
LSFDE14:
	.set L$set$36,LEFDE14-LASFDE14
	.long L$set$36
LASFDE14:
	.set L$set$37,Lframe0-Lsection__debug_frame
	.long L$set$37
	.quad	LFB23
	.set L$set$38,LFE23-LFB23
	.quad L$set$38
	.byte	0x4
	.set L$set$39,LCFI21-LFB23
	.long L$set$39
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$40,LCFI22-LCFI21
	.long L$set$40
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE14:
LSFDE16:
	.set L$set$41,LEFDE16-LASFDE16
	.long L$set$41
LASFDE16:
	.set L$set$42,Lframe0-Lsection__debug_frame
	.long L$set$42
	.quad	LFB24
	.set L$set$43,LFE24-LFB24
	.quad L$set$43
	.byte	0x4
	.set L$set$44,LCFI24-LFB24
	.long L$set$44
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$45,LCFI25-LCFI24
	.long L$set$45
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE16:
LSFDE18:
	.set L$set$46,LEFDE18-LASFDE18
	.long L$set$46
LASFDE18:
	.set L$set$47,Lframe0-Lsection__debug_frame
	.long L$set$47
	.quad	LFB25
	.set L$set$48,LFE25-LFB25
	.quad L$set$48
	.byte	0x4
	.set L$set$49,LCFI27-LFB25
	.long L$set$49
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$50,LCFI28-LCFI27
	.long L$set$50
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE18:
LSFDE20:
	.set L$set$51,LEFDE20-LASFDE20
	.long L$set$51
LASFDE20:
	.set L$set$52,Lframe0-Lsection__debug_frame
	.long L$set$52
	.quad	LFB26
	.set L$set$53,LFE26-LFB26
	.quad L$set$53
	.byte	0x4
	.set L$set$54,LCFI29-LFB26
	.long L$set$54
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$55,LCFI30-LCFI29
	.long L$set$55
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE20:
LSFDE22:
	.set L$set$56,LEFDE22-LASFDE22
	.long L$set$56
LASFDE22:
	.set L$set$57,Lframe0-Lsection__debug_frame
	.long L$set$57
	.quad	LFB27
	.set L$set$58,LFE27-LFB27
	.quad L$set$58
	.byte	0x4
	.set L$set$59,LCFI31-LFB27
	.long L$set$59
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$60,LCFI32-LCFI31
	.long L$set$60
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE22:
LSFDE24:
	.set L$set$61,LEFDE24-LASFDE24
	.long L$set$61
LASFDE24:
	.set L$set$62,Lframe0-Lsection__debug_frame
	.long L$set$62
	.quad	LFB28
	.set L$set$63,LFE28-LFB28
	.quad L$set$63
	.byte	0x4
	.set L$set$64,LCFI34-LFB28
	.long L$set$64
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$65,LCFI35-LCFI34
	.long L$set$65
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE24:
LSFDE26:
	.set L$set$66,LEFDE26-LASFDE26
	.long L$set$66
LASFDE26:
	.set L$set$67,Lframe0-Lsection__debug_frame
	.long L$set$67
	.quad	LFB29
	.set L$set$68,LFE29-LFB29
	.quad L$set$68
	.byte	0x4
	.set L$set$69,LCFI37-LFB29
	.long L$set$69
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$70,LCFI38-LCFI37
	.long L$set$70
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE26:
LSFDE28:
	.set L$set$71,LEFDE28-LASFDE28
	.long L$set$71
LASFDE28:
	.set L$set$72,Lframe0-Lsection__debug_frame
	.long L$set$72
	.quad	LFB30
	.set L$set$73,LFE30-LFB30
	.quad L$set$73
	.byte	0x4
	.set L$set$74,LCFI40-LFB30
	.long L$set$74
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$75,LCFI41-LCFI40
	.long L$set$75
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE28:
LSFDE30:
	.set L$set$76,LEFDE30-LASFDE30
	.long L$set$76
LASFDE30:
	.set L$set$77,Lframe0-Lsection__debug_frame
	.long L$set$77
	.quad	LFB31
	.set L$set$78,LFE31-LFB31
	.quad L$set$78
	.byte	0x4
	.set L$set$79,LCFI43-LFB31
	.long L$set$79
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$80,LCFI44-LCFI43
	.long L$set$80
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE30:
LSFDE32:
	.set L$set$81,LEFDE32-LASFDE32
	.long L$set$81
LASFDE32:
	.set L$set$82,Lframe0-Lsection__debug_frame
	.long L$set$82
	.quad	LFB32
	.set L$set$83,LFE32-LFB32
	.quad L$set$83
	.byte	0x4
	.set L$set$84,LCFI46-LFB32
	.long L$set$84
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$85,LCFI47-LCFI46
	.long L$set$85
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE32:
LSFDE34:
	.set L$set$86,LEFDE34-LASFDE34
	.long L$set$86
LASFDE34:
	.set L$set$87,Lframe0-Lsection__debug_frame
	.long L$set$87
	.quad	LFB33
	.set L$set$88,LFE33-LFB33
	.quad L$set$88
	.byte	0x4
	.set L$set$89,LCFI49-LFB33
	.long L$set$89
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$90,LCFI50-LCFI49
	.long L$set$90
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE34:
LSFDE36:
	.set L$set$91,LEFDE36-LASFDE36
	.long L$set$91
LASFDE36:
	.set L$set$92,Lframe0-Lsection__debug_frame
	.long L$set$92
	.quad	LFB34
	.set L$set$93,LFE34-LFB34
	.quad L$set$93
	.byte	0x4
	.set L$set$94,LCFI52-LFB34
	.long L$set$94
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$95,LCFI53-LCFI52
	.long L$set$95
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE36:
LSFDE38:
	.set L$set$96,LEFDE38-LASFDE38
	.long L$set$96
LASFDE38:
	.set L$set$97,Lframe0-Lsection__debug_frame
	.long L$set$97
	.quad	LFB35
	.set L$set$98,LFE35-LFB35
	.quad L$set$98
	.byte	0x4
	.set L$set$99,LCFI55-LFB35
	.long L$set$99
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$100,LCFI56-LCFI55
	.long L$set$100
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE38:
LSFDE40:
	.set L$set$101,LEFDE40-LASFDE40
	.long L$set$101
LASFDE40:
	.set L$set$102,Lframe0-Lsection__debug_frame
	.long L$set$102
	.quad	LFB36
	.set L$set$103,LFE36-LFB36
	.quad L$set$103
	.byte	0x4
	.set L$set$104,LCFI58-LFB36
	.long L$set$104
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$105,LCFI59-LCFI58
	.long L$set$105
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE40:
LSFDE42:
	.set L$set$106,LEFDE42-LASFDE42
	.long L$set$106
LASFDE42:
	.set L$set$107,Lframe0-Lsection__debug_frame
	.long L$set$107
	.quad	LFB37
	.set L$set$108,LFE37-LFB37
	.quad L$set$108
	.byte	0x4
	.set L$set$109,LCFI61-LFB37
	.long L$set$109
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$110,LCFI62-LCFI61
	.long L$set$110
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE42:
LSFDE44:
	.set L$set$111,LEFDE44-LASFDE44
	.long L$set$111
LASFDE44:
	.set L$set$112,Lframe0-Lsection__debug_frame
	.long L$set$112
	.quad	LFB38
	.set L$set$113,LFE38-LFB38
	.quad L$set$113
	.byte	0x4
	.set L$set$114,LCFI63-LFB38
	.long L$set$114
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$115,LCFI64-LCFI63
	.long L$set$115
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE44:
LSFDE46:
	.set L$set$116,LEFDE46-LASFDE46
	.long L$set$116
LASFDE46:
	.set L$set$117,Lframe0-Lsection__debug_frame
	.long L$set$117
	.quad	LFB39
	.set L$set$118,LFE39-LFB39
	.quad L$set$118
	.byte	0x4
	.set L$set$119,LCFI66-LFB39
	.long L$set$119
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$120,LCFI67-LCFI66
	.long L$set$120
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE46:
LSFDE48:
	.set L$set$121,LEFDE48-LASFDE48
	.long L$set$121
LASFDE48:
	.set L$set$122,Lframe0-Lsection__debug_frame
	.long L$set$122
	.quad	LFB40
	.set L$set$123,LFE40-LFB40
	.quad L$set$123
	.byte	0x4
	.set L$set$124,LCFI69-LFB40
	.long L$set$124
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$125,LCFI70-LCFI69
	.long L$set$125
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE48:
LSFDE50:
	.set L$set$126,LEFDE50-LASFDE50
	.long L$set$126
LASFDE50:
	.set L$set$127,Lframe0-Lsection__debug_frame
	.long L$set$127
	.quad	LFB41
	.set L$set$128,LFE41-LFB41
	.quad L$set$128
	.byte	0x4
	.set L$set$129,LCFI72-LFB41
	.long L$set$129
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$130,LCFI73-LCFI72
	.long L$set$130
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE50:
LSFDE52:
	.set L$set$131,LEFDE52-LASFDE52
	.long L$set$131
LASFDE52:
	.set L$set$132,Lframe0-Lsection__debug_frame
	.long L$set$132
	.quad	LFB42
	.set L$set$133,LFE42-LFB42
	.quad L$set$133
	.byte	0x4
	.set L$set$134,LCFI75-LFB42
	.long L$set$134
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$135,LCFI76-LCFI75
	.long L$set$135
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE52:
LSFDE54:
	.set L$set$136,LEFDE54-LASFDE54
	.long L$set$136
LASFDE54:
	.set L$set$137,Lframe0-Lsection__debug_frame
	.long L$set$137
	.quad	LFB43
	.set L$set$138,LFE43-LFB43
	.quad L$set$138
	.byte	0x4
	.set L$set$139,LCFI78-LFB43
	.long L$set$139
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$140,LCFI79-LCFI78
	.long L$set$140
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE54:
LSFDE56:
	.set L$set$141,LEFDE56-LASFDE56
	.long L$set$141
LASFDE56:
	.set L$set$142,Lframe0-Lsection__debug_frame
	.long L$set$142
	.quad	LFB44
	.set L$set$143,LFE44-LFB44
	.quad L$set$143
	.byte	0x4
	.set L$set$144,LCFI81-LFB44
	.long L$set$144
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$145,LCFI82-LCFI81
	.long L$set$145
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE56:
LSFDE58:
	.set L$set$146,LEFDE58-LASFDE58
	.long L$set$146
LASFDE58:
	.set L$set$147,Lframe0-Lsection__debug_frame
	.long L$set$147
	.quad	LFB45
	.set L$set$148,LFE45-LFB45
	.quad L$set$148
	.byte	0x4
	.set L$set$149,LCFI84-LFB45
	.long L$set$149
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$150,LCFI85-LCFI84
	.long L$set$150
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE58:
LSFDE60:
	.set L$set$151,LEFDE60-LASFDE60
	.long L$set$151
LASFDE60:
	.set L$set$152,Lframe0-Lsection__debug_frame
	.long L$set$152
	.quad	LFB46
	.set L$set$153,LFE46-LFB46
	.quad L$set$153
	.byte	0x4
	.set L$set$154,LCFI87-LFB46
	.long L$set$154
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$155,LCFI88-LCFI87
	.long L$set$155
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE60:
LSFDE62:
	.set L$set$156,LEFDE62-LASFDE62
	.long L$set$156
LASFDE62:
	.set L$set$157,Lframe0-Lsection__debug_frame
	.long L$set$157
	.quad	LFB47
	.set L$set$158,LFE47-LFB47
	.quad L$set$158
	.byte	0x4
	.set L$set$159,LCFI90-LFB47
	.long L$set$159
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$160,LCFI91-LCFI90
	.long L$set$160
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE62:
LSFDE64:
	.set L$set$161,LEFDE64-LASFDE64
	.long L$set$161
LASFDE64:
	.set L$set$162,Lframe0-Lsection__debug_frame
	.long L$set$162
	.quad	LFB48
	.set L$set$163,LFE48-LFB48
	.quad L$set$163
	.byte	0x4
	.set L$set$164,LCFI93-LFB48
	.long L$set$164
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$165,LCFI94-LCFI93
	.long L$set$165
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE64:
LSFDE66:
	.set L$set$166,LEFDE66-LASFDE66
	.long L$set$166
LASFDE66:
	.set L$set$167,Lframe0-Lsection__debug_frame
	.long L$set$167
	.quad	LFB49
	.set L$set$168,LFE49-LFB49
	.quad L$set$168
	.byte	0x4
	.set L$set$169,LCFI96-LFB49
	.long L$set$169
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$170,LCFI97-LCFI96
	.long L$set$170
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE66:
LSFDE68:
	.set L$set$171,LEFDE68-LASFDE68
	.long L$set$171
LASFDE68:
	.set L$set$172,Lframe0-Lsection__debug_frame
	.long L$set$172
	.quad	LFB50
	.set L$set$173,LFE50-LFB50
	.quad L$set$173
	.byte	0x4
	.set L$set$174,LCFI99-LFB50
	.long L$set$174
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$175,LCFI100-LCFI99
	.long L$set$175
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE68:
LSFDE70:
	.set L$set$176,LEFDE70-LASFDE70
	.long L$set$176
LASFDE70:
	.set L$set$177,Lframe0-Lsection__debug_frame
	.long L$set$177
	.quad	LFB51
	.set L$set$178,LFE51-LFB51
	.quad L$set$178
	.byte	0x4
	.set L$set$179,LCFI102-LFB51
	.long L$set$179
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$180,LCFI103-LCFI102
	.long L$set$180
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE70:
LSFDE72:
	.set L$set$181,LEFDE72-LASFDE72
	.long L$set$181
LASFDE72:
	.set L$set$182,Lframe0-Lsection__debug_frame
	.long L$set$182
	.quad	LFB52
	.set L$set$183,LFE52-LFB52
	.quad L$set$183
	.byte	0x4
	.set L$set$184,LCFI105-LFB52
	.long L$set$184
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$185,LCFI106-LCFI105
	.long L$set$185
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE72:
LSFDE74:
	.set L$set$186,LEFDE74-LASFDE74
	.long L$set$186
LASFDE74:
	.set L$set$187,Lframe0-Lsection__debug_frame
	.long L$set$187
	.quad	LFB53
	.set L$set$188,LFE53-LFB53
	.quad L$set$188
	.byte	0x4
	.set L$set$189,LCFI108-LFB53
	.long L$set$189
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$190,LCFI109-LCFI108
	.long L$set$190
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE74:
LSFDE76:
	.set L$set$191,LEFDE76-LASFDE76
	.long L$set$191
LASFDE76:
	.set L$set$192,Lframe0-Lsection__debug_frame
	.long L$set$192
	.quad	LFB54
	.set L$set$193,LFE54-LFB54
	.quad L$set$193
	.byte	0x4
	.set L$set$194,LCFI111-LFB54
	.long L$set$194
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$195,LCFI112-LCFI111
	.long L$set$195
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE76:
LSFDE78:
	.set L$set$196,LEFDE78-LASFDE78
	.long L$set$196
LASFDE78:
	.set L$set$197,Lframe0-Lsection__debug_frame
	.long L$set$197
	.quad	LFB55
	.set L$set$198,LFE55-LFB55
	.quad L$set$198
	.byte	0x4
	.set L$set$199,LCFI114-LFB55
	.long L$set$199
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$200,LCFI115-LCFI114
	.long L$set$200
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE78:
LSFDE80:
	.set L$set$201,LEFDE80-LASFDE80
	.long L$set$201
LASFDE80:
	.set L$set$202,Lframe0-Lsection__debug_frame
	.long L$set$202
	.quad	LFB56
	.set L$set$203,LFE56-LFB56
	.quad L$set$203
	.byte	0x4
	.set L$set$204,LCFI117-LFB56
	.long L$set$204
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$205,LCFI118-LCFI117
	.long L$set$205
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE80:
LSFDE82:
	.set L$set$206,LEFDE82-LASFDE82
	.long L$set$206
LASFDE82:
	.set L$set$207,Lframe0-Lsection__debug_frame
	.long L$set$207
	.quad	LFB57
	.set L$set$208,LFE57-LFB57
	.quad L$set$208
	.byte	0x4
	.set L$set$209,LCFI120-LFB57
	.long L$set$209
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$210,LCFI121-LCFI120
	.long L$set$210
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE82:
LSFDE84:
	.set L$set$211,LEFDE84-LASFDE84
	.long L$set$211
LASFDE84:
	.set L$set$212,Lframe0-Lsection__debug_frame
	.long L$set$212
	.quad	LFB58
	.set L$set$213,LFE58-LFB58
	.quad L$set$213
	.byte	0x4
	.set L$set$214,LCFI123-LFB58
	.long L$set$214
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$215,LCFI124-LCFI123
	.long L$set$215
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE84:
LSFDE86:
	.set L$set$216,LEFDE86-LASFDE86
	.long L$set$216
LASFDE86:
	.set L$set$217,Lframe0-Lsection__debug_frame
	.long L$set$217
	.quad	LFB59
	.set L$set$218,LFE59-LFB59
	.quad L$set$218
	.byte	0x4
	.set L$set$219,LCFI126-LFB59
	.long L$set$219
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$220,LCFI127-LCFI126
	.long L$set$220
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE86:
LSFDE88:
	.set L$set$221,LEFDE88-LASFDE88
	.long L$set$221
LASFDE88:
	.set L$set$222,Lframe0-Lsection__debug_frame
	.long L$set$222
	.quad	LFB60
	.set L$set$223,LFE60-LFB60
	.quad L$set$223
	.byte	0x4
	.set L$set$224,LCFI129-LFB60
	.long L$set$224
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$225,LCFI130-LCFI129
	.long L$set$225
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE88:
LSFDE90:
	.set L$set$226,LEFDE90-LASFDE90
	.long L$set$226
LASFDE90:
	.set L$set$227,Lframe0-Lsection__debug_frame
	.long L$set$227
	.quad	LFB61
	.set L$set$228,LFE61-LFB61
	.quad L$set$228
	.byte	0x4
	.set L$set$229,LCFI132-LFB61
	.long L$set$229
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$230,LCFI133-LCFI132
	.long L$set$230
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE90:
LSFDE92:
	.set L$set$231,LEFDE92-LASFDE92
	.long L$set$231
LASFDE92:
	.set L$set$232,Lframe0-Lsection__debug_frame
	.long L$set$232
	.quad	LFB62
	.set L$set$233,LFE62-LFB62
	.quad L$set$233
	.byte	0x4
	.set L$set$234,LCFI135-LFB62
	.long L$set$234
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$235,LCFI136-LCFI135
	.long L$set$235
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE92:
LSFDE94:
	.set L$set$236,LEFDE94-LASFDE94
	.long L$set$236
LASFDE94:
	.set L$set$237,Lframe0-Lsection__debug_frame
	.long L$set$237
	.quad	LFB63
	.set L$set$238,LFE63-LFB63
	.quad L$set$238
	.byte	0x4
	.set L$set$239,LCFI138-LFB63
	.long L$set$239
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$240,LCFI139-LCFI138
	.long L$set$240
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE94:
	.section __TEXT,__eh_frame,coalesced,no_toc+strip_static_syms+live_support
EH_frame1:
	.set L$set$241,LECIE1-LSCIE1
	.long L$set$241
LSCIE1:
	.long	0x0
	.byte	0x1
	.ascii "zR\0"
	.byte	0x1
	.byte	0x78
	.byte	0x10
	.byte	0x1
	.byte	0x10
	.byte	0xc
	.byte	0x7
	.byte	0x8
	.byte	0x90
	.byte	0x1
	.align 3
LECIE1:
.globl _Gasketrmdir.eh
_Gasketrmdir.eh:
LSFDE1:
	.set L$set$242,LEFDE1-LASFDE1
	.long L$set$242
LASFDE1:
	.long	LASFDE1-EH_frame1
	.quad	LFB16-.
	.set L$set$243,LFE16-LFB16
	.quad L$set$243
	.byte	0x0
	.byte	0x4
	.set L$set$244,LCFI0-LFB16
	.long L$set$244
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$245,LCFI1-LCFI0
	.long L$set$245
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE1:
.globl _Gasketopendir.eh
_Gasketopendir.eh:
LSFDE3:
	.set L$set$246,LEFDE3-LASFDE3
	.long L$set$246
LASFDE3:
	.long	LASFDE3-EH_frame1
	.quad	LFB17-.
	.set L$set$247,LFE17-LFB17
	.quad L$set$247
	.byte	0x0
	.byte	0x4
	.set L$set$248,LCFI3-LFB17
	.long L$set$248
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$249,LCFI4-LCFI3
	.long L$set$249
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE3:
.globl _Gasketstat.eh
_Gasketstat.eh:
LSFDE5:
	.set L$set$250,LEFDE5-LASFDE5
	.long L$set$250
LASFDE5:
	.long	LASFDE5-EH_frame1
	.quad	LFB18-.
	.set L$set$251,LFE18-LFB18
	.quad L$set$251
	.byte	0x0
	.byte	0x4
	.set L$set$252,LCFI6-LFB18
	.long L$set$252
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$253,LCFI7-LCFI6
	.long L$set$253
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE5:
.globl _Gasketstatfs.eh
_Gasketstatfs.eh:
LSFDE7:
	.set L$set$254,LEFDE7-LASFDE7
	.long L$set$254
LASFDE7:
	.long	LASFDE7-EH_frame1
	.quad	LFB19-.
	.set L$set$255,LFE19-LFB19
	.quad L$set$255
	.byte	0x0
	.byte	0x4
	.set L$set$256,LCFI9-LFB19
	.long L$set$256
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$257,LCFI10-LCFI9
	.long L$set$257
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE7:
.globl _GasketmsSleep.eh
_GasketmsSleep.eh:
LSFDE9:
	.set L$set$258,LEFDE9-LASFDE9
	.long L$set$258
LASFDE9:
	.long	LASFDE9-EH_frame1
	.quad	LFB20-.
	.set L$set$259,LFE20-LFB20
	.quad L$set$259
	.byte	0x0
	.byte	0x4
	.set L$set$260,LCFI12-LFB20
	.long L$set$260
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$261,LCFI13-LCFI12
	.long L$set$261
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE9:
.globl _Gasketexit.eh
_Gasketexit.eh:
LSFDE11:
	.set L$set$262,LEFDE11-LASFDE11
	.long L$set$262
LASFDE11:
	.long	LASFDE11-EH_frame1
	.quad	LFB21-.
	.set L$set$263,LFE21-LFB21
	.quad L$set$263
	.byte	0x0
	.byte	0x4
	.set L$set$264,LCFI15-LFB21
	.long L$set$264
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$265,LCFI16-LCFI15
	.long L$set$265
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE11:
.globl _GasketSetTimer.eh
_GasketSetTimer.eh:
LSFDE13:
	.set L$set$266,LEFDE13-LASFDE13
	.long L$set$266
LASFDE13:
	.long	LASFDE13-EH_frame1
	.quad	LFB22-.
	.set L$set$267,LFE22-LFB22
	.quad L$set$267
	.byte	0x0
	.byte	0x4
	.set L$set$268,LCFI18-LFB22
	.long L$set$268
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$269,LCFI19-LCFI18
	.long L$set$269
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE13:
.globl _GasketGetLocalTime.eh
_GasketGetLocalTime.eh:
LSFDE15:
	.set L$set$270,LEFDE15-LASFDE15
	.long L$set$270
LASFDE15:
	.long	LASFDE15-EH_frame1
	.quad	LFB23-.
	.set L$set$271,LFE23-LFB23
	.quad L$set$271
	.byte	0x0
	.byte	0x4
	.set L$set$272,LCFI21-LFB23
	.long L$set$272
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$273,LCFI22-LCFI21
	.long L$set$273
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE15:
.globl _Gasketgmtime.eh
_Gasketgmtime.eh:
LSFDE17:
	.set L$set$274,LEFDE17-LASFDE17
	.long L$set$274
LASFDE17:
	.long	LASFDE17-EH_frame1
	.quad	LFB24-.
	.set L$set$275,LFE24-LFB24
	.quad L$set$275
	.byte	0x0
	.byte	0x4
	.set L$set$276,LCFI24-LFB24
	.long L$set$276
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$277,LCFI25-LCFI24
	.long L$set$277
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE17:
.globl _GasketGetTimeZone.eh
_GasketGetTimeZone.eh:
LSFDE19:
	.set L$set$278,LEFDE19-LASFDE19
	.long L$set$278
LASFDE19:
	.long	LASFDE19-EH_frame1
	.quad	LFB25-.
	.set L$set$279,LFE25-LFB25
	.quad L$set$279
	.byte	0x0
	.byte	0x4
	.set L$set$280,LCFI27-LFB25
	.long L$set$280
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$281,LCFI28-LCFI27
	.long L$set$281
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE19:
.globl _GasketGetDayLight.eh
_GasketGetDayLight.eh:
LSFDE21:
	.set L$set$282,LEFDE21-LASFDE21
	.long L$set$282
LASFDE21:
	.long	LASFDE21-EH_frame1
	.quad	LFB26-.
	.set L$set$283,LFE26-LFB26
	.quad L$set$283
	.byte	0x0
	.byte	0x4
	.set L$set$284,LCFI29-LFB26
	.long L$set$284
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$285,LCFI30-LCFI29
	.long L$set$285
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE21:
.globl _Gasketpoll.eh
_Gasketpoll.eh:
LSFDE23:
	.set L$set$286,LEFDE23-LASFDE23
	.long L$set$286
LASFDE23:
	.long	LASFDE23-EH_frame1
	.quad	LFB27-.
	.set L$set$287,LFE27-LFB27
	.quad L$set$287
	.byte	0x0
	.byte	0x4
	.set L$set$288,LCFI31-LFB27
	.long L$set$288
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$289,LCFI32-LCFI31
	.long L$set$289
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE23:
.globl _Gasketread.eh
_Gasketread.eh:
LSFDE25:
	.set L$set$290,LEFDE25-LASFDE25
	.long L$set$290
LASFDE25:
	.long	LASFDE25-EH_frame1
	.quad	LFB28-.
	.set L$set$291,LFE28-LFB28
	.quad L$set$291
	.byte	0x0
	.byte	0x4
	.set L$set$292,LCFI34-LFB28
	.long L$set$292
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$293,LCFI35-LCFI34
	.long L$set$293
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE25:
.globl _Gasketwrite.eh
_Gasketwrite.eh:
LSFDE27:
	.set L$set$294,LEFDE27-LASFDE27
	.long L$set$294
LASFDE27:
	.long	LASFDE27-EH_frame1
	.quad	LFB29-.
	.set L$set$295,LFE29-LFB29
	.quad L$set$295
	.byte	0x0
	.byte	0x4
	.set L$set$296,LCFI37-LFB29
	.long L$set$296
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$297,LCFI38-LCFI37
	.long L$set$297
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE27:
.globl _Gasketgetenv.eh
_Gasketgetenv.eh:
LSFDE29:
	.set L$set$298,LEFDE29-LASFDE29
	.long L$set$298
LASFDE29:
	.long	LASFDE29-EH_frame1
	.quad	LFB30-.
	.set L$set$299,LFE30-LFB30
	.quad L$set$299
	.byte	0x0
	.byte	0x4
	.set L$set$300,LCFI40-LFB30
	.long L$set$300
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$301,LCFI41-LCFI40
	.long L$set$301
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE29:
.globl _Gasketopen.eh
_Gasketopen.eh:
LSFDE31:
	.set L$set$302,LEFDE31-LASFDE31
	.long L$set$302
LASFDE31:
	.long	LASFDE31-EH_frame1
	.quad	LFB31-.
	.set L$set$303,LFE31-LFB31
	.quad L$set$303
	.byte	0x0
	.byte	0x4
	.set L$set$304,LCFI43-LFB31
	.long L$set$304
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$305,LCFI44-LCFI43
	.long L$set$305
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE31:
.globl _Gasketlseek.eh
_Gasketlseek.eh:
LSFDE33:
	.set L$set$306,LEFDE33-LASFDE33
	.long L$set$306
LASFDE33:
	.long	LASFDE33-EH_frame1
	.quad	LFB32-.
	.set L$set$307,LFE32-LFB32
	.quad L$set$307
	.byte	0x0
	.byte	0x4
	.set L$set$308,LCFI46-LFB32
	.long L$set$308
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$309,LCFI47-LCFI46
	.long L$set$309
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE33:
.globl _Gasketftruncate.eh
_Gasketftruncate.eh:
LSFDE35:
	.set L$set$310,LEFDE35-LASFDE35
	.long L$set$310
LASFDE35:
	.long	LASFDE35-EH_frame1
	.quad	LFB33-.
	.set L$set$311,LFE33-LFB33
	.quad L$set$311
	.byte	0x0
	.byte	0x4
	.set L$set$312,LCFI49-LFB33
	.long L$set$312
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$313,LCFI50-LCFI49
	.long L$set$313
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE35:
.globl _Gasketclose.eh
_Gasketclose.eh:
LSFDE37:
	.set L$set$314,LEFDE37-LASFDE37
	.long L$set$314
LASFDE37:
	.long	LASFDE37-EH_frame1
	.quad	LFB34-.
	.set L$set$315,LFE34-LFB34
	.quad L$set$315
	.byte	0x0
	.byte	0x4
	.set L$set$316,LCFI52-LFB34
	.long L$set$316
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$317,LCFI53-LCFI52
	.long L$set$317
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE37:
.globl _Gasketmkdir.eh
_Gasketmkdir.eh:
LSFDE39:
	.set L$set$318,LEFDE39-LASFDE39
	.long L$set$318
LASFDE39:
	.long	LASFDE39-EH_frame1
	.quad	LFB35-.
	.set L$set$319,LFE35-LFB35
	.quad L$set$319
	.byte	0x0
	.byte	0x4
	.set L$set$320,LCFI55-LFB35
	.long L$set$320
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$321,LCFI56-LCFI55
	.long L$set$321
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE39:
.globl _Gasketunlink.eh
_Gasketunlink.eh:
LSFDE41:
	.set L$set$322,LEFDE41-LASFDE41
	.long L$set$322
LASFDE41:
	.long	LASFDE41-EH_frame1
	.quad	LFB36-.
	.set L$set$323,LFE36-LFB36
	.quad L$set$323
	.byte	0x0
	.byte	0x4
	.set L$set$324,LCFI58-LFB36
	.long L$set$324
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$325,LCFI59-LCFI58
	.long L$set$325
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE41:
.globl _GasketGetErrno.eh
_GasketGetErrno.eh:
LSFDE43:
	.set L$set$326,LEFDE43-LASFDE43
	.long L$set$326
LASFDE43:
	.long	LASFDE43-EH_frame1
	.quad	LFB37-.
	.set L$set$327,LFE37-LFB37
	.quad L$set$327
	.byte	0x0
	.byte	0x4
	.set L$set$328,LCFI61-LFB37
	.long L$set$328
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$329,LCFI62-LCFI61
	.long L$set$329
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE43:
.globl _Gasketrewinddir.eh
_Gasketrewinddir.eh:
LSFDE45:
	.set L$set$330,LEFDE45-LASFDE45
	.long L$set$330
LASFDE45:
	.long	LASFDE45-EH_frame1
	.quad	LFB38-.
	.set L$set$331,LFE38-LFB38
	.quad L$set$331
	.byte	0x0
	.byte	0x4
	.set L$set$332,LCFI63-LFB38
	.long L$set$332
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$333,LCFI64-LCFI63
	.long L$set$333
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE45:
.globl _Gasketreaddir.eh
_Gasketreaddir.eh:
LSFDE47:
	.set L$set$334,LEFDE47-LASFDE47
	.long L$set$334
LASFDE47:
	.long	LASFDE47-EH_frame1
	.quad	LFB39-.
	.set L$set$335,LFE39-LFB39
	.quad L$set$335
	.byte	0x0
	.byte	0x4
	.set L$set$336,LCFI66-LFB39
	.long L$set$336
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$337,LCFI67-LCFI66
	.long L$set$337
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE47:
.globl _Gasketclosedir.eh
_Gasketclosedir.eh:
LSFDE49:
	.set L$set$338,LEFDE49-LASFDE49
	.long L$set$338
LASFDE49:
	.long	LASFDE49-EH_frame1
	.quad	LFB40-.
	.set L$set$339,LFE40-LFB40
	.quad L$set$339
	.byte	0x0
	.byte	0x4
	.set L$set$340,LCFI69-LFB40
	.long L$set$340
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$341,LCFI70-LCFI69
	.long L$set$341
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE49:
.globl _Gasketrename.eh
_Gasketrename.eh:
LSFDE51:
	.set L$set$342,LEFDE51-LASFDE51
	.long L$set$342
LASFDE51:
	.long	LASFDE51-EH_frame1
	.quad	LFB41-.
	.set L$set$343,LFE41-LFB41
	.quad L$set$343
	.byte	0x0
	.byte	0x4
	.set L$set$344,LCFI72-LFB41
	.long L$set$344
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$345,LCFI73-LCFI72
	.long L$set$345
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE51:
.globl _Gasketmktime.eh
_Gasketmktime.eh:
LSFDE53:
	.set L$set$346,LEFDE53-LASFDE53
	.long L$set$346
LASFDE53:
	.long	LASFDE53-EH_frame1
	.quad	LFB42-.
	.set L$set$347,LFE42-LFB42
	.quad L$set$347
	.byte	0x0
	.byte	0x4
	.set L$set$348,LCFI75-LFB42
	.long L$set$348
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$349,LCFI76-LCFI75
	.long L$set$349
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE53:
.globl _Gasketfsync.eh
_Gasketfsync.eh:
LSFDE55:
	.set L$set$350,LEFDE55-LASFDE55
	.long L$set$350
LASFDE55:
	.long	LASFDE55-EH_frame1
	.quad	LFB43-.
	.set L$set$351,LFE43-LFB43
	.quad L$set$351
	.byte	0x0
	.byte	0x4
	.set L$set$352,LCFI78-LFB43
	.long L$set$352
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$353,LCFI79-LCFI78
	.long L$set$353
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE55:
.globl _Gasketchmod.eh
_Gasketchmod.eh:
LSFDE57:
	.set L$set$354,LEFDE57-LASFDE57
	.long L$set$354
LASFDE57:
	.long	LASFDE57-EH_frame1
	.quad	LFB44-.
	.set L$set$355,LFE44-LFB44
	.quad L$set$355
	.byte	0x0
	.byte	0x4
	.set L$set$356,LCFI81-LFB44
	.long L$set$356
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$357,LCFI82-LCFI81
	.long L$set$357
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE57:
.globl _Gasketutime.eh
_Gasketutime.eh:
LSFDE59:
	.set L$set$358,LEFDE59-LASFDE59
	.long L$set$358
LASFDE59:
	.long	LASFDE59-EH_frame1
	.quad	LFB45-.
	.set L$set$359,LFE45-LFB45
	.quad L$set$359
	.byte	0x0
	.byte	0x4
	.set L$set$360,LCFI84-LFB45
	.long L$set$360
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$361,LCFI85-LCFI84
	.long L$set$361
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE59:
.globl _Gaskettcflush.eh
_Gaskettcflush.eh:
LSFDE61:
	.set L$set$362,LEFDE61-LASFDE61
	.long L$set$362
LASFDE61:
	.long	LASFDE61-EH_frame1
	.quad	LFB46-.
	.set L$set$363,LFE46-LFB46
	.quad L$set$363
	.byte	0x0
	.byte	0x4
	.set L$set$364,LCFI87-LFB46
	.long L$set$364
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$365,LCFI88-LCFI87
	.long L$set$365
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE61:
.globl _GasketUgaCreate.eh
_GasketUgaCreate.eh:
LSFDE63:
	.set L$set$366,LEFDE63-LASFDE63
	.long L$set$366
LASFDE63:
	.long	LASFDE63-EH_frame1
	.quad	LFB47-.
	.set L$set$367,LFE47-LFB47
	.quad L$set$367
	.byte	0x0
	.byte	0x4
	.set L$set$368,LCFI90-LFB47
	.long L$set$368
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$369,LCFI91-LCFI90
	.long L$set$369
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE63:
.globl _Gasketperror.eh
_Gasketperror.eh:
LSFDE65:
	.set L$set$370,LEFDE65-LASFDE65
	.long L$set$370
LASFDE65:
	.long	LASFDE65-EH_frame1
	.quad	LFB48-.
	.set L$set$371,LFE48-LFB48
	.quad L$set$371
	.byte	0x0
	.byte	0x4
	.set L$set$372,LCFI93-LFB48
	.long L$set$372
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$373,LCFI94-LCFI93
	.long L$set$373
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE65:
.globl _Gasketioctl.eh
_Gasketioctl.eh:
LSFDE67:
	.set L$set$374,LEFDE67-LASFDE67
	.long L$set$374
LASFDE67:
	.long	LASFDE67-EH_frame1
	.quad	LFB49-.
	.set L$set$375,LFE49-LFB49
	.quad L$set$375
	.byte	0x0
	.byte	0x4
	.set L$set$376,LCFI96-LFB49
	.long L$set$376
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$377,LCFI97-LCFI96
	.long L$set$377
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE67:
.globl _Gasketfcntl.eh
_Gasketfcntl.eh:
LSFDE69:
	.set L$set$378,LEFDE69-LASFDE69
	.long L$set$378
LASFDE69:
	.long	LASFDE69-EH_frame1
	.quad	LFB50-.
	.set L$set$379,LFE50-LFB50
	.quad L$set$379
	.byte	0x0
	.byte	0x4
	.set L$set$380,LCFI99-LFB50
	.long L$set$380
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$381,LCFI100-LCFI99
	.long L$set$381
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE69:
.globl _Gasketcfsetispeed.eh
_Gasketcfsetispeed.eh:
LSFDE71:
	.set L$set$382,LEFDE71-LASFDE71
	.long L$set$382
LASFDE71:
	.long	LASFDE71-EH_frame1
	.quad	LFB51-.
	.set L$set$383,LFE51-LFB51
	.quad L$set$383
	.byte	0x0
	.byte	0x4
	.set L$set$384,LCFI102-LFB51
	.long L$set$384
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$385,LCFI103-LCFI102
	.long L$set$385
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE71:
.globl _Gasketcfsetospeed.eh
_Gasketcfsetospeed.eh:
LSFDE73:
	.set L$set$386,LEFDE73-LASFDE73
	.long L$set$386
LASFDE73:
	.long	LASFDE73-EH_frame1
	.quad	LFB52-.
	.set L$set$387,LFE52-LFB52
	.quad L$set$387
	.byte	0x0
	.byte	0x4
	.set L$set$388,LCFI105-LFB52
	.long L$set$388
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$389,LCFI106-LCFI105
	.long L$set$389
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE73:
.globl _Gaskettcgetattr.eh
_Gaskettcgetattr.eh:
LSFDE75:
	.set L$set$390,LEFDE75-LASFDE75
	.long L$set$390
LASFDE75:
	.long	LASFDE75-EH_frame1
	.quad	LFB53-.
	.set L$set$391,LFE53-LFB53
	.quad L$set$391
	.byte	0x0
	.byte	0x4
	.set L$set$392,LCFI108-LFB53
	.long L$set$392
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$393,LCFI109-LCFI108
	.long L$set$393
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE75:
.globl _Gaskettcsetattr.eh
_Gaskettcsetattr.eh:
LSFDE77:
	.set L$set$394,LEFDE77-LASFDE77
	.long L$set$394
LASFDE77:
	.long	LASFDE77-EH_frame1
	.quad	LFB54-.
	.set L$set$395,LFE54-LFB54
	.quad L$set$395
	.byte	0x0
	.byte	0x4
	.set L$set$396,LCFI111-LFB54
	.long L$set$396
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$397,LCFI112-LCFI111
	.long L$set$397
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE77:
.globl _GasketUnixPeCoffGetEntryPoint.eh
_GasketUnixPeCoffGetEntryPoint.eh:
LSFDE79:
	.set L$set$398,LEFDE79-LASFDE79
	.long L$set$398
LASFDE79:
	.long	LASFDE79-EH_frame1
	.quad	LFB55-.
	.set L$set$399,LFE55-LFB55
	.quad L$set$399
	.byte	0x0
	.byte	0x4
	.set L$set$400,LCFI114-LFB55
	.long L$set$400
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$401,LCFI115-LCFI114
	.long L$set$401
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE79:
.globl _GasketUnixPeCoffRelocateImageExtraAction.eh
_GasketUnixPeCoffRelocateImageExtraAction.eh:
LSFDE81:
	.set L$set$402,LEFDE81-LASFDE81
	.long L$set$402
LASFDE81:
	.long	LASFDE81-EH_frame1
	.quad	LFB56-.
	.set L$set$403,LFE56-LFB56
	.quad L$set$403
	.byte	0x0
	.byte	0x4
	.set L$set$404,LCFI117-LFB56
	.long L$set$404
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$405,LCFI118-LCFI117
	.long L$set$405
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE81:
.globl _GasketUnixPeCoffUnloadImageExtraAction.eh
_GasketUnixPeCoffUnloadImageExtraAction.eh:
LSFDE83:
	.set L$set$406,LEFDE83-LASFDE83
	.long L$set$406
LASFDE83:
	.long	LASFDE83-EH_frame1
	.quad	LFB57-.
	.set L$set$407,LFE57-LFB57
	.quad L$set$407
	.byte	0x0
	.byte	0x4
	.set L$set$408,LCFI120-LFB57
	.long L$set$408
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$409,LCFI121-LCFI120
	.long L$set$409
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE83:
.globl _GasketUgaClose.eh
_GasketUgaClose.eh:
LSFDE85:
	.set L$set$410,LEFDE85-LASFDE85
	.long L$set$410
LASFDE85:
	.long	LASFDE85-EH_frame1
	.quad	LFB58-.
	.set L$set$411,LFE58-LFB58
	.quad L$set$411
	.byte	0x0
	.byte	0x4
	.set L$set$412,LCFI123-LFB58
	.long L$set$412
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$413,LCFI124-LCFI123
	.long L$set$413
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE85:
.globl _GasketUgaSize.eh
_GasketUgaSize.eh:
LSFDE87:
	.set L$set$414,LEFDE87-LASFDE87
	.long L$set$414
LASFDE87:
	.long	LASFDE87-EH_frame1
	.quad	LFB59-.
	.set L$set$415,LFE59-LFB59
	.quad L$set$415
	.byte	0x0
	.byte	0x4
	.set L$set$416,LCFI126-LFB59
	.long L$set$416
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$417,LCFI127-LCFI126
	.long L$set$417
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE87:
.globl _GasketUgaCheckKey.eh
_GasketUgaCheckKey.eh:
LSFDE89:
	.set L$set$418,LEFDE89-LASFDE89
	.long L$set$418
LASFDE89:
	.long	LASFDE89-EH_frame1
	.quad	LFB60-.
	.set L$set$419,LFE60-LFB60
	.quad L$set$419
	.byte	0x0
	.byte	0x4
	.set L$set$420,LCFI129-LFB60
	.long L$set$420
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$421,LCFI130-LCFI129
	.long L$set$421
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE89:
.globl _GasketUgaGetKey.eh
_GasketUgaGetKey.eh:
LSFDE91:
	.set L$set$422,LEFDE91-LASFDE91
	.long L$set$422
LASFDE91:
	.long	LASFDE91-EH_frame1
	.quad	LFB61-.
	.set L$set$423,LFE61-LFB61
	.quad L$set$423
	.byte	0x0
	.byte	0x4
	.set L$set$424,LCFI132-LFB61
	.long L$set$424
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$425,LCFI133-LCFI132
	.long L$set$425
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE91:
.globl _GasketUgaBlt.eh
_GasketUgaBlt.eh:
LSFDE93:
	.set L$set$426,LEFDE93-LASFDE93
	.long L$set$426
LASFDE93:
	.long	LASFDE93-EH_frame1
	.quad	LFB62-.
	.set L$set$427,LFE62-LFB62
	.quad L$set$427
	.byte	0x0
	.byte	0x4
	.set L$set$428,LCFI135-LFB62
	.long L$set$428
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$429,LCFI136-LCFI135
	.long L$set$429
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE93:
.globl _ReverseGasketUint64.eh
_ReverseGasketUint64.eh:
LSFDE95:
	.set L$set$430,LEFDE95-LASFDE95
	.long L$set$430
LASFDE95:
	.long	LASFDE95-EH_frame1
	.quad	LFB63-.
	.set L$set$431,LFE63-LFB63
	.quad L$set$431
	.byte	0x0
	.byte	0x4
	.set L$set$432,LCFI138-LFB63
	.long L$set$432
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$433,LCFI139-LCFI138
	.long L$set$433
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE95:
	.text
Letext0:
	.section __DWARF,__debug_loc,regular,debug
Ldebug_loc0:
LLST0:
	.set L$set$434,LFB16-Ltext0
	.quad L$set$434
	.set L$set$435,LCFI0-Ltext0
	.quad L$set$435
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$436,LCFI0-Ltext0
	.quad L$set$436
	.set L$set$437,LCFI1-Ltext0
	.quad L$set$437
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$438,LCFI1-Ltext0
	.quad L$set$438
	.set L$set$439,LFE16-Ltext0
	.quad L$set$439
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST1:
	.set L$set$440,LFB17-Ltext0
	.quad L$set$440
	.set L$set$441,LCFI3-Ltext0
	.quad L$set$441
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$442,LCFI3-Ltext0
	.quad L$set$442
	.set L$set$443,LCFI4-Ltext0
	.quad L$set$443
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$444,LCFI4-Ltext0
	.quad L$set$444
	.set L$set$445,LFE17-Ltext0
	.quad L$set$445
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST2:
	.set L$set$446,LFB18-Ltext0
	.quad L$set$446
	.set L$set$447,LCFI6-Ltext0
	.quad L$set$447
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$448,LCFI6-Ltext0
	.quad L$set$448
	.set L$set$449,LCFI7-Ltext0
	.quad L$set$449
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$450,LCFI7-Ltext0
	.quad L$set$450
	.set L$set$451,LFE18-Ltext0
	.quad L$set$451
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST3:
	.set L$set$452,LFB19-Ltext0
	.quad L$set$452
	.set L$set$453,LCFI9-Ltext0
	.quad L$set$453
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$454,LCFI9-Ltext0
	.quad L$set$454
	.set L$set$455,LCFI10-Ltext0
	.quad L$set$455
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$456,LCFI10-Ltext0
	.quad L$set$456
	.set L$set$457,LFE19-Ltext0
	.quad L$set$457
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST4:
	.set L$set$458,LFB20-Ltext0
	.quad L$set$458
	.set L$set$459,LCFI12-Ltext0
	.quad L$set$459
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$460,LCFI12-Ltext0
	.quad L$set$460
	.set L$set$461,LCFI13-Ltext0
	.quad L$set$461
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$462,LCFI13-Ltext0
	.quad L$set$462
	.set L$set$463,LFE20-Ltext0
	.quad L$set$463
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST5:
	.set L$set$464,LFB21-Ltext0
	.quad L$set$464
	.set L$set$465,LCFI15-Ltext0
	.quad L$set$465
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$466,LCFI15-Ltext0
	.quad L$set$466
	.set L$set$467,LCFI16-Ltext0
	.quad L$set$467
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$468,LCFI16-Ltext0
	.quad L$set$468
	.set L$set$469,LFE21-Ltext0
	.quad L$set$469
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST6:
	.set L$set$470,LFB22-Ltext0
	.quad L$set$470
	.set L$set$471,LCFI18-Ltext0
	.quad L$set$471
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$472,LCFI18-Ltext0
	.quad L$set$472
	.set L$set$473,LCFI19-Ltext0
	.quad L$set$473
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$474,LCFI19-Ltext0
	.quad L$set$474
	.set L$set$475,LFE22-Ltext0
	.quad L$set$475
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST7:
	.set L$set$476,LFB23-Ltext0
	.quad L$set$476
	.set L$set$477,LCFI21-Ltext0
	.quad L$set$477
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$478,LCFI21-Ltext0
	.quad L$set$478
	.set L$set$479,LCFI22-Ltext0
	.quad L$set$479
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$480,LCFI22-Ltext0
	.quad L$set$480
	.set L$set$481,LFE23-Ltext0
	.quad L$set$481
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST8:
	.set L$set$482,LFB24-Ltext0
	.quad L$set$482
	.set L$set$483,LCFI24-Ltext0
	.quad L$set$483
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$484,LCFI24-Ltext0
	.quad L$set$484
	.set L$set$485,LCFI25-Ltext0
	.quad L$set$485
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$486,LCFI25-Ltext0
	.quad L$set$486
	.set L$set$487,LFE24-Ltext0
	.quad L$set$487
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST9:
	.set L$set$488,LFB25-Ltext0
	.quad L$set$488
	.set L$set$489,LCFI27-Ltext0
	.quad L$set$489
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$490,LCFI27-Ltext0
	.quad L$set$490
	.set L$set$491,LCFI28-Ltext0
	.quad L$set$491
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$492,LCFI28-Ltext0
	.quad L$set$492
	.set L$set$493,LFE25-Ltext0
	.quad L$set$493
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST10:
	.set L$set$494,LFB26-Ltext0
	.quad L$set$494
	.set L$set$495,LCFI29-Ltext0
	.quad L$set$495
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$496,LCFI29-Ltext0
	.quad L$set$496
	.set L$set$497,LCFI30-Ltext0
	.quad L$set$497
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$498,LCFI30-Ltext0
	.quad L$set$498
	.set L$set$499,LFE26-Ltext0
	.quad L$set$499
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST11:
	.set L$set$500,LFB27-Ltext0
	.quad L$set$500
	.set L$set$501,LCFI31-Ltext0
	.quad L$set$501
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$502,LCFI31-Ltext0
	.quad L$set$502
	.set L$set$503,LCFI32-Ltext0
	.quad L$set$503
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$504,LCFI32-Ltext0
	.quad L$set$504
	.set L$set$505,LFE27-Ltext0
	.quad L$set$505
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST12:
	.set L$set$506,LFB28-Ltext0
	.quad L$set$506
	.set L$set$507,LCFI34-Ltext0
	.quad L$set$507
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$508,LCFI34-Ltext0
	.quad L$set$508
	.set L$set$509,LCFI35-Ltext0
	.quad L$set$509
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$510,LCFI35-Ltext0
	.quad L$set$510
	.set L$set$511,LFE28-Ltext0
	.quad L$set$511
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST13:
	.set L$set$512,LFB29-Ltext0
	.quad L$set$512
	.set L$set$513,LCFI37-Ltext0
	.quad L$set$513
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$514,LCFI37-Ltext0
	.quad L$set$514
	.set L$set$515,LCFI38-Ltext0
	.quad L$set$515
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$516,LCFI38-Ltext0
	.quad L$set$516
	.set L$set$517,LFE29-Ltext0
	.quad L$set$517
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST14:
	.set L$set$518,LFB30-Ltext0
	.quad L$set$518
	.set L$set$519,LCFI40-Ltext0
	.quad L$set$519
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$520,LCFI40-Ltext0
	.quad L$set$520
	.set L$set$521,LCFI41-Ltext0
	.quad L$set$521
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$522,LCFI41-Ltext0
	.quad L$set$522
	.set L$set$523,LFE30-Ltext0
	.quad L$set$523
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST15:
	.set L$set$524,LFB31-Ltext0
	.quad L$set$524
	.set L$set$525,LCFI43-Ltext0
	.quad L$set$525
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$526,LCFI43-Ltext0
	.quad L$set$526
	.set L$set$527,LCFI44-Ltext0
	.quad L$set$527
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$528,LCFI44-Ltext0
	.quad L$set$528
	.set L$set$529,LFE31-Ltext0
	.quad L$set$529
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST16:
	.set L$set$530,LFB32-Ltext0
	.quad L$set$530
	.set L$set$531,LCFI46-Ltext0
	.quad L$set$531
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$532,LCFI46-Ltext0
	.quad L$set$532
	.set L$set$533,LCFI47-Ltext0
	.quad L$set$533
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$534,LCFI47-Ltext0
	.quad L$set$534
	.set L$set$535,LFE32-Ltext0
	.quad L$set$535
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST17:
	.set L$set$536,LFB33-Ltext0
	.quad L$set$536
	.set L$set$537,LCFI49-Ltext0
	.quad L$set$537
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$538,LCFI49-Ltext0
	.quad L$set$538
	.set L$set$539,LCFI50-Ltext0
	.quad L$set$539
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$540,LCFI50-Ltext0
	.quad L$set$540
	.set L$set$541,LFE33-Ltext0
	.quad L$set$541
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST18:
	.set L$set$542,LFB34-Ltext0
	.quad L$set$542
	.set L$set$543,LCFI52-Ltext0
	.quad L$set$543
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$544,LCFI52-Ltext0
	.quad L$set$544
	.set L$set$545,LCFI53-Ltext0
	.quad L$set$545
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$546,LCFI53-Ltext0
	.quad L$set$546
	.set L$set$547,LFE34-Ltext0
	.quad L$set$547
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST19:
	.set L$set$548,LFB35-Ltext0
	.quad L$set$548
	.set L$set$549,LCFI55-Ltext0
	.quad L$set$549
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$550,LCFI55-Ltext0
	.quad L$set$550
	.set L$set$551,LCFI56-Ltext0
	.quad L$set$551
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$552,LCFI56-Ltext0
	.quad L$set$552
	.set L$set$553,LFE35-Ltext0
	.quad L$set$553
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST20:
	.set L$set$554,LFB36-Ltext0
	.quad L$set$554
	.set L$set$555,LCFI58-Ltext0
	.quad L$set$555
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$556,LCFI58-Ltext0
	.quad L$set$556
	.set L$set$557,LCFI59-Ltext0
	.quad L$set$557
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$558,LCFI59-Ltext0
	.quad L$set$558
	.set L$set$559,LFE36-Ltext0
	.quad L$set$559
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST21:
	.set L$set$560,LFB37-Ltext0
	.quad L$set$560
	.set L$set$561,LCFI61-Ltext0
	.quad L$set$561
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$562,LCFI61-Ltext0
	.quad L$set$562
	.set L$set$563,LCFI62-Ltext0
	.quad L$set$563
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$564,LCFI62-Ltext0
	.quad L$set$564
	.set L$set$565,LFE37-Ltext0
	.quad L$set$565
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST22:
	.set L$set$566,LFB38-Ltext0
	.quad L$set$566
	.set L$set$567,LCFI63-Ltext0
	.quad L$set$567
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$568,LCFI63-Ltext0
	.quad L$set$568
	.set L$set$569,LCFI64-Ltext0
	.quad L$set$569
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$570,LCFI64-Ltext0
	.quad L$set$570
	.set L$set$571,LFE38-Ltext0
	.quad L$set$571
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST23:
	.set L$set$572,LFB39-Ltext0
	.quad L$set$572
	.set L$set$573,LCFI66-Ltext0
	.quad L$set$573
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$574,LCFI66-Ltext0
	.quad L$set$574
	.set L$set$575,LCFI67-Ltext0
	.quad L$set$575
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$576,LCFI67-Ltext0
	.quad L$set$576
	.set L$set$577,LFE39-Ltext0
	.quad L$set$577
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST24:
	.set L$set$578,LFB40-Ltext0
	.quad L$set$578
	.set L$set$579,LCFI69-Ltext0
	.quad L$set$579
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$580,LCFI69-Ltext0
	.quad L$set$580
	.set L$set$581,LCFI70-Ltext0
	.quad L$set$581
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$582,LCFI70-Ltext0
	.quad L$set$582
	.set L$set$583,LFE40-Ltext0
	.quad L$set$583
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST25:
	.set L$set$584,LFB41-Ltext0
	.quad L$set$584
	.set L$set$585,LCFI72-Ltext0
	.quad L$set$585
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$586,LCFI72-Ltext0
	.quad L$set$586
	.set L$set$587,LCFI73-Ltext0
	.quad L$set$587
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$588,LCFI73-Ltext0
	.quad L$set$588
	.set L$set$589,LFE41-Ltext0
	.quad L$set$589
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST26:
	.set L$set$590,LFB42-Ltext0
	.quad L$set$590
	.set L$set$591,LCFI75-Ltext0
	.quad L$set$591
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$592,LCFI75-Ltext0
	.quad L$set$592
	.set L$set$593,LCFI76-Ltext0
	.quad L$set$593
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$594,LCFI76-Ltext0
	.quad L$set$594
	.set L$set$595,LFE42-Ltext0
	.quad L$set$595
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST27:
	.set L$set$596,LFB43-Ltext0
	.quad L$set$596
	.set L$set$597,LCFI78-Ltext0
	.quad L$set$597
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$598,LCFI78-Ltext0
	.quad L$set$598
	.set L$set$599,LCFI79-Ltext0
	.quad L$set$599
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$600,LCFI79-Ltext0
	.quad L$set$600
	.set L$set$601,LFE43-Ltext0
	.quad L$set$601
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST28:
	.set L$set$602,LFB44-Ltext0
	.quad L$set$602
	.set L$set$603,LCFI81-Ltext0
	.quad L$set$603
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$604,LCFI81-Ltext0
	.quad L$set$604
	.set L$set$605,LCFI82-Ltext0
	.quad L$set$605
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$606,LCFI82-Ltext0
	.quad L$set$606
	.set L$set$607,LFE44-Ltext0
	.quad L$set$607
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST29:
	.set L$set$608,LFB45-Ltext0
	.quad L$set$608
	.set L$set$609,LCFI84-Ltext0
	.quad L$set$609
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$610,LCFI84-Ltext0
	.quad L$set$610
	.set L$set$611,LCFI85-Ltext0
	.quad L$set$611
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$612,LCFI85-Ltext0
	.quad L$set$612
	.set L$set$613,LFE45-Ltext0
	.quad L$set$613
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST30:
	.set L$set$614,LFB46-Ltext0
	.quad L$set$614
	.set L$set$615,LCFI87-Ltext0
	.quad L$set$615
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$616,LCFI87-Ltext0
	.quad L$set$616
	.set L$set$617,LCFI88-Ltext0
	.quad L$set$617
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$618,LCFI88-Ltext0
	.quad L$set$618
	.set L$set$619,LFE46-Ltext0
	.quad L$set$619
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST31:
	.set L$set$620,LFB47-Ltext0
	.quad L$set$620
	.set L$set$621,LCFI90-Ltext0
	.quad L$set$621
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$622,LCFI90-Ltext0
	.quad L$set$622
	.set L$set$623,LCFI91-Ltext0
	.quad L$set$623
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$624,LCFI91-Ltext0
	.quad L$set$624
	.set L$set$625,LFE47-Ltext0
	.quad L$set$625
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST32:
	.set L$set$626,LFB48-Ltext0
	.quad L$set$626
	.set L$set$627,LCFI93-Ltext0
	.quad L$set$627
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$628,LCFI93-Ltext0
	.quad L$set$628
	.set L$set$629,LCFI94-Ltext0
	.quad L$set$629
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$630,LCFI94-Ltext0
	.quad L$set$630
	.set L$set$631,LFE48-Ltext0
	.quad L$set$631
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST33:
	.set L$set$632,LFB49-Ltext0
	.quad L$set$632
	.set L$set$633,LCFI96-Ltext0
	.quad L$set$633
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$634,LCFI96-Ltext0
	.quad L$set$634
	.set L$set$635,LCFI97-Ltext0
	.quad L$set$635
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$636,LCFI97-Ltext0
	.quad L$set$636
	.set L$set$637,LFE49-Ltext0
	.quad L$set$637
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST34:
	.set L$set$638,LFB50-Ltext0
	.quad L$set$638
	.set L$set$639,LCFI99-Ltext0
	.quad L$set$639
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$640,LCFI99-Ltext0
	.quad L$set$640
	.set L$set$641,LCFI100-Ltext0
	.quad L$set$641
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$642,LCFI100-Ltext0
	.quad L$set$642
	.set L$set$643,LFE50-Ltext0
	.quad L$set$643
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST35:
	.set L$set$644,LFB51-Ltext0
	.quad L$set$644
	.set L$set$645,LCFI102-Ltext0
	.quad L$set$645
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$646,LCFI102-Ltext0
	.quad L$set$646
	.set L$set$647,LCFI103-Ltext0
	.quad L$set$647
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$648,LCFI103-Ltext0
	.quad L$set$648
	.set L$set$649,LFE51-Ltext0
	.quad L$set$649
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST36:
	.set L$set$650,LFB52-Ltext0
	.quad L$set$650
	.set L$set$651,LCFI105-Ltext0
	.quad L$set$651
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$652,LCFI105-Ltext0
	.quad L$set$652
	.set L$set$653,LCFI106-Ltext0
	.quad L$set$653
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$654,LCFI106-Ltext0
	.quad L$set$654
	.set L$set$655,LFE52-Ltext0
	.quad L$set$655
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST37:
	.set L$set$656,LFB53-Ltext0
	.quad L$set$656
	.set L$set$657,LCFI108-Ltext0
	.quad L$set$657
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$658,LCFI108-Ltext0
	.quad L$set$658
	.set L$set$659,LCFI109-Ltext0
	.quad L$set$659
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$660,LCFI109-Ltext0
	.quad L$set$660
	.set L$set$661,LFE53-Ltext0
	.quad L$set$661
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST38:
	.set L$set$662,LFB54-Ltext0
	.quad L$set$662
	.set L$set$663,LCFI111-Ltext0
	.quad L$set$663
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$664,LCFI111-Ltext0
	.quad L$set$664
	.set L$set$665,LCFI112-Ltext0
	.quad L$set$665
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$666,LCFI112-Ltext0
	.quad L$set$666
	.set L$set$667,LFE54-Ltext0
	.quad L$set$667
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST39:
	.set L$set$668,LFB55-Ltext0
	.quad L$set$668
	.set L$set$669,LCFI114-Ltext0
	.quad L$set$669
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$670,LCFI114-Ltext0
	.quad L$set$670
	.set L$set$671,LCFI115-Ltext0
	.quad L$set$671
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$672,LCFI115-Ltext0
	.quad L$set$672
	.set L$set$673,LFE55-Ltext0
	.quad L$set$673
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST40:
	.set L$set$674,LFB56-Ltext0
	.quad L$set$674
	.set L$set$675,LCFI117-Ltext0
	.quad L$set$675
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$676,LCFI117-Ltext0
	.quad L$set$676
	.set L$set$677,LCFI118-Ltext0
	.quad L$set$677
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$678,LCFI118-Ltext0
	.quad L$set$678
	.set L$set$679,LFE56-Ltext0
	.quad L$set$679
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST41:
	.set L$set$680,LFB57-Ltext0
	.quad L$set$680
	.set L$set$681,LCFI120-Ltext0
	.quad L$set$681
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$682,LCFI120-Ltext0
	.quad L$set$682
	.set L$set$683,LCFI121-Ltext0
	.quad L$set$683
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$684,LCFI121-Ltext0
	.quad L$set$684
	.set L$set$685,LFE57-Ltext0
	.quad L$set$685
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST42:
	.set L$set$686,LFB58-Ltext0
	.quad L$set$686
	.set L$set$687,LCFI123-Ltext0
	.quad L$set$687
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$688,LCFI123-Ltext0
	.quad L$set$688
	.set L$set$689,LCFI124-Ltext0
	.quad L$set$689
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$690,LCFI124-Ltext0
	.quad L$set$690
	.set L$set$691,LFE58-Ltext0
	.quad L$set$691
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST43:
	.set L$set$692,LFB59-Ltext0
	.quad L$set$692
	.set L$set$693,LCFI126-Ltext0
	.quad L$set$693
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$694,LCFI126-Ltext0
	.quad L$set$694
	.set L$set$695,LCFI127-Ltext0
	.quad L$set$695
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$696,LCFI127-Ltext0
	.quad L$set$696
	.set L$set$697,LFE59-Ltext0
	.quad L$set$697
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST44:
	.set L$set$698,LFB60-Ltext0
	.quad L$set$698
	.set L$set$699,LCFI129-Ltext0
	.quad L$set$699
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$700,LCFI129-Ltext0
	.quad L$set$700
	.set L$set$701,LCFI130-Ltext0
	.quad L$set$701
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$702,LCFI130-Ltext0
	.quad L$set$702
	.set L$set$703,LFE60-Ltext0
	.quad L$set$703
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST45:
	.set L$set$704,LFB61-Ltext0
	.quad L$set$704
	.set L$set$705,LCFI132-Ltext0
	.quad L$set$705
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$706,LCFI132-Ltext0
	.quad L$set$706
	.set L$set$707,LCFI133-Ltext0
	.quad L$set$707
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$708,LCFI133-Ltext0
	.quad L$set$708
	.set L$set$709,LFE61-Ltext0
	.quad L$set$709
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST46:
	.set L$set$710,LFB62-Ltext0
	.quad L$set$710
	.set L$set$711,LCFI135-Ltext0
	.quad L$set$711
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$712,LCFI135-Ltext0
	.quad L$set$712
	.set L$set$713,LCFI136-Ltext0
	.quad L$set$713
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$714,LCFI136-Ltext0
	.quad L$set$714
	.set L$set$715,LFE62-Ltext0
	.quad L$set$715
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
LLST47:
	.set L$set$716,LFB63-Ltext0
	.quad L$set$716
	.set L$set$717,LCFI138-Ltext0
	.quad L$set$717
	.word	0x2
	.byte	0x77
	.byte	0x8
	.set L$set$718,LCFI138-Ltext0
	.quad L$set$718
	.set L$set$719,LCFI139-Ltext0
	.quad L$set$719
	.word	0x2
	.byte	0x77
	.byte	0x10
	.set L$set$720,LCFI139-Ltext0
	.quad L$set$720
	.set L$set$721,LFE63-Ltext0
	.quad L$set$721
	.word	0x2
	.byte	0x76
	.byte	0x10
	.quad	0x0
	.quad	0x0
	.file 2 "/Users/fish/work/edk2/MdePkg/Include/X64/ProcessorBind.h"
	.file 3 "/Users/fish/work/edk2/MdePkg/Include/Base.h"
	.file 4 "<built-in>"
	.file 5 "/Users/fish/work/edk2/MdePkg/Include/Uefi/UefiBaseType.h"
	.file 6 "/usr/include/sys/termios.h"
	.file 7 "/usr/include/i386/_types.h"
	.file 8 "/usr/include/sys/_types.h"
	.file 9 "/usr/include/stdio.h"
	.file 10 "/usr/include/sys/_structs.h"
	.file 11 "/usr/include/sys/time.h"
	.file 12 "/usr/include/time.h"
	.file 13 "/usr/include/sys/dirent.h"
	.file 14 "/usr/include/dirent.h"
	.file 15 "/usr/include/unistd.h"
	.file 16 "/usr/include/sys/poll.h"
	.file 17 "/usr/include/i386/types.h"
	.file 18 "/usr/include/sys/types.h"
	.file 19 "/usr/lib/gcc/i686-apple-darwin10/4.2.1/include/stdint.h"
	.file 20 "/usr/include/sys/mount.h"
	.file 21 "/usr/include/utime.h"
	.file 22 "/Users/fish/work/edk2/MdePkg/Include/Library/PeCoffLib.h"
	.file 23 "/Users/fish/work/edk2/UnixPkg/Include/Protocol/UnixThunk.h"
	.file 24 "/Users/fish/work/edk2/UnixPkg/Include/Protocol/UnixUgaIo.h"
	.file 25 "/Users/fish/work/edk2/MdePkg/Include/Protocol/UgaDraw.h"
	.file 26 "/Users/fish/work/edk2/MdePkg/Include/Protocol/SimpleTextIn.h"
	.section __DWARF,__debug_info,regular,debug
	.long	0x20dc
	.word	0x2
	.set L$set$722,Ldebug_abbrev0-Lsection__debug_abbrev
	.long L$set$722
	.byte	0x8
	.byte	0x1
	.ascii "GNU C 4.2.1 (Apple Inc. build 5664)\0"
	.byte	0x1
	.ascii "/Users/fish/work/edk2/UnixPkg/Sec/X64/GasketEfiTemplate.c\0"
	.quad	Ltext0
	.quad	Letext0
	.set L$set$723,Ldebug_line0-Lsection__debug_line
	.long L$set$723
	.byte	0x2
	.ascii "UINT64\0"
	.byte	0x2
	.byte	0x9b
	.long	0x8d
	.byte	0x3
	.byte	0x8
	.byte	0x7
	.ascii "long long unsigned int\0"
	.byte	0x3
	.byte	0x8
	.byte	0x5
	.ascii "long long int\0"
	.byte	0x2
	.ascii "UINT32\0"
	.byte	0x2
	.byte	0xa3
	.long	0xc6
	.byte	0x3
	.byte	0x4
	.byte	0x7
	.ascii "unsigned int\0"
	.byte	0x3
	.byte	0x4
	.byte	0x5
	.ascii "int\0"
	.byte	0x2
	.ascii "UINT16\0"
	.byte	0x2
	.byte	0xab
	.long	0xeb
	.byte	0x3
	.byte	0x2
	.byte	0x7
	.ascii "short unsigned int\0"
	.byte	0x2
	.ascii "CHAR16\0"
	.byte	0x2
	.byte	0xb0
	.long	0xeb
	.byte	0x2
	.ascii "INT16\0"
	.byte	0x2
	.byte	0xb4
	.long	0x11c
	.byte	0x3
	.byte	0x2
	.byte	0x5
	.ascii "short int\0"
	.byte	0x2
	.ascii "BOOLEAN\0"
	.byte	0x2
	.byte	0xb9
	.long	0x138
	.byte	0x3
	.byte	0x1
	.byte	0x8
	.ascii "unsigned char\0"
	.byte	0x2
	.ascii "UINT8\0"
	.byte	0x2
	.byte	0xbd
	.long	0x138
	.byte	0x2
	.ascii "CHAR8\0"
	.byte	0x2
	.byte	0xc1
	.long	0x163
	.byte	0x3
	.byte	0x1
	.byte	0x6
	.ascii "char\0"
	.byte	0x2
	.ascii "UINTN\0"
	.byte	0x2
	.byte	0xcc
	.long	0x7f
	.byte	0x4
	.byte	0x8
	.byte	0x7
	.byte	0x2
	.ascii "PHYSICAL_ADDRESS\0"
	.byte	0x3
	.byte	0x84
	.long	0x7f
	.byte	0x5
	.ascii "VA_LIST\0"
	.byte	0x3
	.word	0x1d2
	.long	0x1a3
	.byte	0x6
	.long	0x1b3
	.long	0x1b3
	.byte	0x7
	.long	0x178
	.byte	0x0
	.byte	0x0
	.byte	0x8
	.ascii "__va_list_tag\0"
	.byte	0x18
	.byte	0x4
	.byte	0x0
	.long	0x226
	.byte	0x9
	.ascii "gp_offset\0"
	.byte	0x4
	.byte	0x0
	.long	0xc6
	.byte	0x2
	.byte	0x23
	.byte	0x0
	.byte	0x9
	.ascii "fp_offset\0"
	.byte	0x4
	.byte	0x0
	.long	0xc6
	.byte	0x2
	.byte	0x23
	.byte	0x4
	.byte	0x9
	.ascii "overflow_arg_area\0"
	.byte	0x4
	.byte	0x0
	.long	0x226
	.byte	0x2
	.byte	0x23
	.byte	0x8
	.byte	0x9
	.ascii "reg_save_area\0"
	.byte	0x4
	.byte	0x0
	.long	0x226
	.byte	0x2
	.byte	0x23
	.byte	0x10
	.byte	0x0
	.byte	0xa
	.byte	0x8
	.byte	0xb
	.byte	0x8
	.long	0x16b
	.byte	0x5
	.ascii "RETURN_STATUS\0"
	.byte	0x3
	.word	0x2a6
	.long	0x16b
	.byte	0x2
	.ascii "EFI_STATUS\0"
	.byte	0x5
	.byte	0x1f
	.long	0x22e
	.byte	0xc
	.byte	0x10
	.byte	0x5
	.byte	0x46
	.long	0x316
	.byte	0x9
	.ascii "Year\0"
	.byte	0x5
	.byte	0x47
	.long	0xdd
	.byte	0x2
	.byte	0x23
	.byte	0x0
	.byte	0x9
	.ascii "Month\0"
	.byte	0x5
	.byte	0x48
	.long	0x149
	.byte	0x2
	.byte	0x23
	.byte	0x2
	.byte	0x9
	.ascii "Day\0"
	.byte	0x5
	.byte	0x49
	.long	0x149
	.byte	0x2
	.byte	0x23
	.byte	0x3
	.byte	0x9
	.ascii "Hour\0"
	.byte	0x5
	.byte	0x4a
	.long	0x149
	.byte	0x2
	.byte	0x23
	.byte	0x4
	.byte	0x9
	.ascii "Minute\0"
	.byte	0x5
	.byte	0x4b
	.long	0x149
	.byte	0x2
	.byte	0x23
	.byte	0x5
	.byte	0x9
	.ascii "Second\0"
	.byte	0x5
	.byte	0x4c
	.long	0x149
	.byte	0x2
	.byte	0x23
	.byte	0x6
	.byte	0x9
	.ascii "Pad1\0"
	.byte	0x5
	.byte	0x4d
	.long	0x149
	.byte	0x2
	.byte	0x23
	.byte	0x7
	.byte	0x9
	.ascii "Nanosecond\0"
	.byte	0x5
	.byte	0x4e
	.long	0xb8
	.byte	0x2
	.byte	0x23
	.byte	0x8
	.byte	0x9
	.ascii "TimeZone\0"
	.byte	0x5
	.byte	0x4f
	.long	0x10f
	.byte	0x2
	.byte	0x23
	.byte	0xc
	.byte	0x9
	.ascii "Daylight\0"
	.byte	0x5
	.byte	0x50
	.long	0x149
	.byte	0x2
	.byte	0x23
	.byte	0xe
	.byte	0x9
	.ascii "Pad2\0"
	.byte	0x5
	.byte	0x51
	.long	0x149
	.byte	0x2
	.byte	0x23
	.byte	0xf
	.byte	0x0
	.byte	0x2
	.ascii "EFI_TIME\0"
	.byte	0x5
	.byte	0x52
	.long	0x256
	.byte	0xb
	.byte	0x8
	.long	0x226
	.byte	0x5
	.ascii "tcflag_t\0"
	.byte	0x6
	.word	0x109
	.long	0x33d
	.byte	0x3
	.byte	0x8
	.byte	0x7
	.ascii "long unsigned int\0"
	.byte	0x5
	.ascii "cc_t\0"
	.byte	0x6
	.word	0x10a
	.long	0x138
	.byte	0x5
	.ascii "speed_t\0"
	.byte	0x6
	.word	0x10b
	.long	0x33d
	.byte	0xd
	.ascii "termios\0"
	.byte	0x48
	.byte	0x6
	.word	0x10d
	.long	0x405
	.byte	0xe
	.ascii "c_iflag\0"
	.byte	0x6
	.word	0x10e
	.long	0x32c
	.byte	0x2
	.byte	0x23
	.byte	0x0
	.byte	0xe
	.ascii "c_oflag\0"
	.byte	0x6
	.word	0x10f
	.long	0x32c
	.byte	0x2
	.byte	0x23
	.byte	0x8
	.byte	0xe
	.ascii "c_cflag\0"
	.byte	0x6
	.word	0x110
	.long	0x32c
	.byte	0x2
	.byte	0x23
	.byte	0x10
	.byte	0xe
	.ascii "c_lflag\0"
	.byte	0x6
	.word	0x111
	.long	0x32c
	.byte	0x2
	.byte	0x23
	.byte	0x18
	.byte	0xe
	.ascii "c_cc\0"
	.byte	0x6
	.word	0x112
	.long	0x405
	.byte	0x2
	.byte	0x23
	.byte	0x20
	.byte	0xe
	.ascii "c_ispeed\0"
	.byte	0x6
	.word	0x113
	.long	0x35f
	.byte	0x2
	.byte	0x23
	.byte	0x38
	.byte	0xe
	.ascii "c_ospeed\0"
	.byte	0x6
	.word	0x114
	.long	0x35f
	.byte	0x2
	.byte	0x23
	.byte	0x40
	.byte	0x0
	.byte	0x6
	.long	0x352
	.long	0x415
	.byte	0x7
	.long	0x178
	.byte	0x13
	.byte	0x0
	.byte	0x3
	.byte	0x1
	.byte	0x6
	.ascii "signed char\0"
	.byte	0x2
	.ascii "__uint8_t\0"
	.byte	0x7
	.byte	0x29
	.long	0x138
	.byte	0x2
	.ascii "__uint16_t\0"
	.byte	0x7
	.byte	0x2b
	.long	0xeb
	.byte	0x2
	.ascii "__int32_t\0"
	.byte	0x7
	.byte	0x2c
	.long	0xd6
	.byte	0x2
	.ascii "__uint32_t\0"
	.byte	0x7
	.byte	0x2d
	.long	0xc6
	.byte	0x2
	.ascii "__int64_t\0"
	.byte	0x7
	.byte	0x2e
	.long	0xa7
	.byte	0x2
	.ascii "__uint64_t\0"
	.byte	0x7
	.byte	0x2f
	.long	0x8d
	.byte	0x3
	.byte	0x8
	.byte	0x5
	.ascii "long int\0"
	.byte	0x2
	.ascii "__darwin_time_t\0"
	.byte	0x7
	.byte	0x76
	.long	0x48d
	.byte	0x6
	.long	0x163
	.long	0x4c0
	.byte	0x7
	.long	0x178
	.byte	0x37
	.byte	0x0
	.byte	0x8
	.ascii "_opaque_pthread_mutex_t\0"
	.byte	0x40
	.byte	0x8
	.byte	0x43
	.long	0x504
	.byte	0x9
	.ascii "__sig\0"
	.byte	0x8
	.byte	0x43
	.long	0x48d
	.byte	0x2
	.byte	0x23
	.byte	0x0
	.byte	0x9
	.ascii "__opaque\0"
	.byte	0x8
	.byte	0x43
	.long	0x4b0
	.byte	0x2
	.byte	0x23
	.byte	0x8
	.byte	0x0
	.byte	0x6
	.long	0x163
	.long	0x514
	.byte	0x7
	.long	0x178
	.byte	0xf
	.byte	0x0
	.byte	0x2
	.ascii "__darwin_blkcnt_t\0"
	.byte	0x8
	.byte	0x5e
	.long	0x46a
	.byte	0x2
	.ascii "__darwin_blksize_t\0"
	.byte	0x8
	.byte	0x5f
	.long	0x447
	.byte	0x2
	.ascii "__darwin_dev_t\0"
	.byte	0x8
	.byte	0x60
	.long	0x447
	.byte	0x2
	.ascii "__darwin_gid_t\0"
	.byte	0x8
	.byte	0x63
	.long	0x458
	.byte	0x2
	.ascii "__darwin_ino64_t\0"
	.byte	0x8
	.byte	0x65
	.long	0x47b
	.byte	0x2
	.ascii "__darwin_mode_t\0"
	.byte	0x8
	.byte	0x6d
	.long	0x435
	.byte	0x2
	.ascii "__darwin_off_t\0"
	.byte	0x8
	.byte	0x6e
	.long	0x46a
	.byte	0x2
	.ascii "__darwin_pthread_mutex_t\0"
	.byte	0x8
	.byte	0x78
	.long	0x4c0
	.byte	0x2
	.ascii "__darwin_uid_t\0"
	.byte	0x8
	.byte	0x85
	.long	0x458
	.byte	0x2
	.ascii "off_t\0"
	.byte	0x9
	.byte	0x4b
	.long	0x5a2
	.byte	0xb
	.byte	0x8
	.long	0x163
	.byte	0xb
	.byte	0x8
	.long	0x607
	.byte	0xf
	.long	0x163
	.byte	0x8
	.ascii "timespec\0"
	.byte	0x10
	.byte	0xa
	.byte	0x59
	.long	0x641
	.byte	0x9
	.ascii "tv_sec\0"
	.byte	0xa
	.byte	0x5a
	.long	0x499
	.byte	0x2
	.byte	0x23
	.byte	0x0
	.byte	0x9
	.ascii "tv_nsec\0"
	.byte	0xa
	.byte	0x5b
	.long	0x48d
	.byte	0x2
	.byte	0x23
	.byte	0x8
	.byte	0x0
	.byte	0x2
	.ascii "time_t\0"
	.byte	0xb
	.byte	0x51
	.long	0x499
	.byte	0x8
	.ascii "tm\0"
	.byte	0x38
	.byte	0xc
	.byte	0x5a
	.long	0x721
	.byte	0x9
	.ascii "tm_sec\0"
	.byte	0xc
	.byte	0x5b
	.long	0xd6
	.byte	0x2
	.byte	0x23
	.byte	0x0
	.byte	0x9
	.ascii "tm_min\0"
	.byte	0xc
	.byte	0x5c
	.long	0xd6
	.byte	0x2
	.byte	0x23
	.byte	0x4
	.byte	0x9
	.ascii "tm_hour\0"
	.byte	0xc
	.byte	0x5d
	.long	0xd6
	.byte	0x2
	.byte	0x23
	.byte	0x8
	.byte	0x9
	.ascii "tm_mday\0"
	.byte	0xc
	.byte	0x5e
	.long	0xd6
	.byte	0x2
	.byte	0x23
	.byte	0xc
	.byte	0x9
	.ascii "tm_mon\0"
	.byte	0xc
	.byte	0x5f
	.long	0xd6
	.byte	0x2
	.byte	0x23
	.byte	0x10
	.byte	0x9
	.ascii "tm_year\0"
	.byte	0xc
	.byte	0x60
	.long	0xd6
	.byte	0x2
	.byte	0x23
	.byte	0x14
	.byte	0x9
	.ascii "tm_wday\0"
	.byte	0xc
	.byte	0x61
	.long	0xd6
	.byte	0x2
	.byte	0x23
	.byte	0x18
	.byte	0x9
	.ascii "tm_yday\0"
	.byte	0xc
	.byte	0x62
	.long	0xd6
	.byte	0x2
	.byte	0x23
	.byte	0x1c
	.byte	0x9
	.ascii "tm_isdst\0"
	.byte	0xc
	.byte	0x63
	.long	0xd6
	.byte	0x2
	.byte	0x23
	.byte	0x20
	.byte	0x9
	.ascii "tm_gmtoff\0"
	.byte	0xc
	.byte	0x64
	.long	0x48d
	.byte	0x2
	.byte	0x23
	.byte	0x28
	.byte	0x9
	.ascii "tm_zone\0"
	.byte	0xc
	.byte	0x65
	.long	0x5fb
	.byte	0x2
	.byte	0x23
	.byte	0x30
	.byte	0x0
	.byte	0x10
	.ascii "dirent\0"
	.word	0x418
	.byte	0xd
	.byte	0x73
	.long	0x79e
	.byte	0x9
	.ascii "d_ino\0"
	.byte	0xd
	.byte	0x73
	.long	0x47b
	.byte	0x2
	.byte	0x23
	.byte	0x0
	.byte	0x9
	.ascii "d_seekoff\0"
	.byte	0xd
	.byte	0x73
	.long	0x47b
	.byte	0x2
	.byte	0x23
	.byte	0x8
	.byte	0x9
	.ascii "d_reclen\0"
	.byte	0xd
	.byte	0x73
	.long	0x435
	.byte	0x2
	.byte	0x23
	.byte	0x10
	.byte	0x9
	.ascii "d_namlen\0"
	.byte	0xd
	.byte	0x73
	.long	0x435
	.byte	0x2
	.byte	0x23
	.byte	0x12
	.byte	0x9
	.ascii "d_type\0"
	.byte	0xd
	.byte	0x73
	.long	0x424
	.byte	0x2
	.byte	0x23
	.byte	0x14
	.byte	0x9
	.ascii "d_name\0"
	.byte	0xd
	.byte	0x73
	.long	0x79e
	.byte	0x2
	.byte	0x23
	.byte	0x15
	.byte	0x0
	.byte	0x6
	.long	0x163
	.long	0x7af
	.byte	0x11
	.long	0x178
	.word	0x3ff
	.byte	0x0
	.byte	0xc
	.byte	0x88
	.byte	0xe
	.byte	0x47
	.long	0x87d
	.byte	0x9
	.ascii "__dd_fd\0"
	.byte	0xe
	.byte	0x48
	.long	0xd6
	.byte	0x2
	.byte	0x23
	.byte	0x0
	.byte	0x9
	.ascii "__dd_loc\0"
	.byte	0xe
	.byte	0x49
	.long	0x48d
	.byte	0x2
	.byte	0x23
	.byte	0x8
	.byte	0x9
	.ascii "__dd_size\0"
	.byte	0xe
	.byte	0x4a
	.long	0x48d
	.byte	0x2
	.byte	0x23
	.byte	0x10
	.byte	0x9
	.ascii "__dd_buf\0"
	.byte	0xe
	.byte	0x4b
	.long	0x5fb
	.byte	0x2
	.byte	0x23
	.byte	0x18
	.byte	0x9
	.ascii "__dd_len\0"
	.byte	0xe
	.byte	0x4c
	.long	0xd6
	.byte	0x2
	.byte	0x23
	.byte	0x20
	.byte	0x9
	.ascii "__dd_seek\0"
	.byte	0xe
	.byte	0x4d
	.long	0x48d
	.byte	0x2
	.byte	0x23
	.byte	0x28
	.byte	0x9
	.ascii "__dd_rewind\0"
	.byte	0xe
	.byte	0x4e
	.long	0x48d
	.byte	0x2
	.byte	0x23
	.byte	0x30
	.byte	0x9
	.ascii "__dd_flags\0"
	.byte	0xe
	.byte	0x4f
	.long	0xd6
	.byte	0x2
	.byte	0x23
	.byte	0x38
	.byte	0x9
	.ascii "__dd_lock\0"
	.byte	0xe
	.byte	0x50
	.long	0x5b8
	.byte	0x2
	.byte	0x23
	.byte	0x40
	.byte	0x9
	.ascii "__dd_td\0"
	.byte	0xe
	.byte	0x51
	.long	0x888
	.byte	0x3
	.byte	0x23
	.byte	0x80,0x1
	.byte	0x0
	.byte	0x12
	.ascii "_telldir\0"
	.byte	0x1
	.byte	0xb
	.byte	0x8
	.long	0x87d
	.byte	0x2
	.ascii "DIR\0"
	.byte	0xe
	.byte	0x52
	.long	0x7af
	.byte	0x2
	.ascii "dev_t\0"
	.byte	0xf
	.byte	0x4d
	.long	0x547
	.byte	0x2
	.ascii "gid_t\0"
	.byte	0xf
	.byte	0x53
	.long	0x55d
	.byte	0x2
	.ascii "mode_t\0"
	.byte	0xf
	.byte	0x5e
	.long	0x58b
	.byte	0x2
	.ascii "uid_t\0"
	.byte	0xf
	.byte	0x7a
	.long	0x5d8
	.byte	0x8
	.ascii "pollfd\0"
	.byte	0x8
	.byte	0x10
	.byte	0x61
	.long	0x90e
	.byte	0x9
	.ascii "fd\0"
	.byte	0x10
	.byte	0x62
	.long	0xd6
	.byte	0x2
	.byte	0x23
	.byte	0x0
	.byte	0x9
	.ascii "events\0"
	.byte	0x10
	.byte	0x63
	.long	0x11c
	.byte	0x2
	.byte	0x23
	.byte	0x4
	.byte	0x9
	.ascii "revents\0"
	.byte	0x10
	.byte	0x64
	.long	0x11c
	.byte	0x2
	.byte	0x23
	.byte	0x6
	.byte	0x0
	.byte	0x2
	.ascii "int32_t\0"
	.byte	0x11
	.byte	0x58
	.long	0xd6
	.byte	0x2
	.ascii "blkcnt_t\0"
	.byte	0x12
	.byte	0x6e
	.long	0x514
	.byte	0x2
	.ascii "blksize_t\0"
	.byte	0x12
	.byte	0x73
	.long	0x52d
	.byte	0x2
	.ascii "nlink_t\0"
	.byte	0x12
	.byte	0x9d
	.long	0x435
	.byte	0x6
	.long	0x46a
	.long	0x95d
	.byte	0x7
	.long	0x178
	.byte	0x1
	.byte	0x0
	.byte	0x2
	.ascii "uint32_t\0"
	.byte	0x13
	.byte	0x32
	.long	0xc6
	.byte	0x2
	.ascii "uint64_t\0"
	.byte	0x13
	.byte	0x37
	.long	0x8d
	.byte	0x8
	.ascii "fsid\0"
	.byte	0x8
	.byte	0x14
	.byte	0x53
	.long	0x999
	.byte	0x9
	.ascii "val\0"
	.byte	0x14
	.byte	0x53
	.long	0x999
	.byte	0x2
	.byte	0x23
	.byte	0x0
	.byte	0x0
	.byte	0x6
	.long	0x90e
	.long	0x9a9
	.byte	0x7
	.long	0x178
	.byte	0x1
	.byte	0x0
	.byte	0x2
	.ascii "fsid_t\0"
	.byte	0x14
	.byte	0x53
	.long	0x97d
	.byte	0x6
	.long	0x95d
	.long	0x9c7
	.byte	0x7
	.long	0x178
	.byte	0x7
	.byte	0x0
	.byte	0x10
	.ascii "statfs\0"
	.word	0x878
	.byte	0x14
	.byte	0x7d
	.long	0xb11
	.byte	0x9
	.ascii "f_bsize\0"
	.byte	0x14
	.byte	0x7d
	.long	0x95d
	.byte	0x2
	.byte	0x23
	.byte	0x0
	.byte	0x9
	.ascii "f_iosize\0"
	.byte	0x14
	.byte	0x7d
	.long	0x90e
	.byte	0x2
	.byte	0x23
	.byte	0x4
	.byte	0x9
	.ascii "f_blocks\0"
	.byte	0x14
	.byte	0x7d
	.long	0x96d
	.byte	0x2
	.byte	0x23
	.byte	0x8
	.byte	0x9
	.ascii "f_bfree\0"
	.byte	0x14
	.byte	0x7d
	.long	0x96d
	.byte	0x2
	.byte	0x23
	.byte	0x10
	.byte	0x9
	.ascii "f_bavail\0"
	.byte	0x14
	.byte	0x7d
	.long	0x96d
	.byte	0x2
	.byte	0x23
	.byte	0x18
	.byte	0x9
	.ascii "f_files\0"
	.byte	0x14
	.byte	0x7d
	.long	0x96d
	.byte	0x2
	.byte	0x23
	.byte	0x20
	.byte	0x9
	.ascii "f_ffree\0"
	.byte	0x14
	.byte	0x7d
	.long	0x96d
	.byte	0x2
	.byte	0x23
	.byte	0x28
	.byte	0x9
	.ascii "f_fsid\0"
	.byte	0x14
	.byte	0x7d
	.long	0x9a9
	.byte	0x2
	.byte	0x23
	.byte	0x30
	.byte	0x9
	.ascii "f_owner\0"
	.byte	0x14
	.byte	0x7d
	.long	0x8c1
	.byte	0x2
	.byte	0x23
	.byte	0x38
	.byte	0x9
	.ascii "f_type\0"
	.byte	0x14
	.byte	0x7d
	.long	0x95d
	.byte	0x2
	.byte	0x23
	.byte	0x3c
	.byte	0x9
	.ascii "f_flags\0"
	.byte	0x14
	.byte	0x7d
	.long	0x95d
	.byte	0x2
	.byte	0x23
	.byte	0x40
	.byte	0x9
	.ascii "f_fssubtype\0"
	.byte	0x14
	.byte	0x7d
	.long	0x95d
	.byte	0x2
	.byte	0x23
	.byte	0x44
	.byte	0x9
	.ascii "f_fstypename\0"
	.byte	0x14
	.byte	0x7d
	.long	0x504
	.byte	0x2
	.byte	0x23
	.byte	0x48
	.byte	0x9
	.ascii "f_mntonname\0"
	.byte	0x14
	.byte	0x7d
	.long	0x79e
	.byte	0x2
	.byte	0x23
	.byte	0x58
	.byte	0x9
	.ascii "f_mntfromname\0"
	.byte	0x14
	.byte	0x7d
	.long	0x79e
	.byte	0x3
	.byte	0x23
	.byte	0xd8,0x8
	.byte	0x9
	.ascii "f_reserved\0"
	.byte	0x14
	.byte	0x7d
	.long	0x9b7
	.byte	0x3
	.byte	0x23
	.byte	0xd8,0x10
	.byte	0x0
	.byte	0x8
	.ascii "utimbuf\0"
	.byte	0x10
	.byte	0x15
	.byte	0x44
	.long	0xb45
	.byte	0x9
	.ascii "actime\0"
	.byte	0x15
	.byte	0x45
	.long	0x641
	.byte	0x2
	.byte	0x23
	.byte	0x0
	.byte	0x9
	.ascii "modtime\0"
	.byte	0x15
	.byte	0x46
	.long	0x641
	.byte	0x2
	.byte	0x23
	.byte	0x8
	.byte	0x0
	.byte	0x2
	.ascii "PE_COFF_LOADER_READ_FILE\0"
	.byte	0x16
	.byte	0x4b
	.long	0xb65
	.byte	0xb
	.byte	0x8
	.long	0xb6b
	.byte	0x13
	.byte	0x1
	.long	0x22e
	.long	0xb8a
	.byte	0x14
	.long	0x226
	.byte	0x14
	.long	0x16b
	.byte	0x14
	.long	0x228
	.byte	0x14
	.long	0x226
	.byte	0x0
	.byte	0xc
	.byte	0x90
	.byte	0x16
	.byte	0x50
	.long	0xdaa
	.byte	0x9
	.ascii "ImageAddress\0"
	.byte	0x16
	.byte	0x54
	.long	0x17b
	.byte	0x2
	.byte	0x23
	.byte	0x0
	.byte	0x9
	.ascii "ImageSize\0"
	.byte	0x16
	.byte	0x59
	.long	0x7f
	.byte	0x2
	.byte	0x23
	.byte	0x8
	.byte	0x9
	.ascii "DestinationAddress\0"
	.byte	0x16
	.byte	0x60
	.long	0x17b
	.byte	0x2
	.byte	0x23
	.byte	0x10
	.byte	0x15
	.set L$set$724,LASF0-Lsection__debug_str
	.long L$set$724
	.byte	0x16
	.byte	0x64
	.long	0x17b
	.byte	0x2
	.byte	0x23
	.byte	0x18
	.byte	0x9
	.ascii "ImageRead\0"
	.byte	0x16
	.byte	0x69
	.long	0xb45
	.byte	0x2
	.byte	0x23
	.byte	0x20
	.byte	0x9
	.ascii "Handle\0"
	.byte	0x16
	.byte	0x6d
	.long	0x226
	.byte	0x2
	.byte	0x23
	.byte	0x28
	.byte	0x9
	.ascii "FixupData\0"
	.byte	0x16
	.byte	0x74
	.long	0x226
	.byte	0x2
	.byte	0x23
	.byte	0x30
	.byte	0x9
	.ascii "SectionAlignment\0"
	.byte	0x16
	.byte	0x79
	.long	0xb8
	.byte	0x2
	.byte	0x23
	.byte	0x38
	.byte	0x9
	.ascii "PeCoffHeaderOffset\0"
	.byte	0x16
	.byte	0x7f
	.long	0xb8
	.byte	0x2
	.byte	0x23
	.byte	0x3c
	.byte	0x9
	.ascii "DebugDirectoryEntryRva\0"
	.byte	0x16
	.byte	0x84
	.long	0xb8
	.byte	0x2
	.byte	0x23
	.byte	0x40
	.byte	0x9
	.ascii "CodeView\0"
	.byte	0x16
	.byte	0x88
	.long	0x226
	.byte	0x2
	.byte	0x23
	.byte	0x48
	.byte	0x9
	.ascii "PdbPointer\0"
	.byte	0x16
	.byte	0x8e
	.long	0xdaa
	.byte	0x2
	.byte	0x23
	.byte	0x50
	.byte	0x9
	.ascii "SizeOfHeaders\0"
	.byte	0x16
	.byte	0x92
	.long	0x16b
	.byte	0x2
	.byte	0x23
	.byte	0x58
	.byte	0x9
	.ascii "ImageCodeMemoryType\0"
	.byte	0x16
	.byte	0x99
	.long	0xb8
	.byte	0x2
	.byte	0x23
	.byte	0x60
	.byte	0x9
	.ascii "ImageDataMemoryType\0"
	.byte	0x16
	.byte	0xa0
	.long	0xb8
	.byte	0x2
	.byte	0x23
	.byte	0x64
	.byte	0x9
	.ascii "ImageError\0"
	.byte	0x16
	.byte	0xa4
	.long	0xb8
	.byte	0x2
	.byte	0x23
	.byte	0x68
	.byte	0x9
	.ascii "FixupDataSize\0"
	.byte	0x16
	.byte	0xa9
	.long	0x16b
	.byte	0x2
	.byte	0x23
	.byte	0x70
	.byte	0x9
	.ascii "Machine\0"
	.byte	0x16
	.byte	0xad
	.long	0xdd
	.byte	0x2
	.byte	0x23
	.byte	0x78
	.byte	0x9
	.ascii "ImageType\0"
	.byte	0x16
	.byte	0xb1
	.long	0xdd
	.byte	0x2
	.byte	0x23
	.byte	0x7a
	.byte	0x9
	.ascii "RelocationsStripped\0"
	.byte	0x16
	.byte	0xb6
	.long	0x129
	.byte	0x2
	.byte	0x23
	.byte	0x7c
	.byte	0x9
	.ascii "IsTeImage\0"
	.byte	0x16
	.byte	0xbc
	.long	0x129
	.byte	0x2
	.byte	0x23
	.byte	0x7d
	.byte	0x9
	.ascii "HiiResourceData\0"
	.byte	0x16
	.byte	0xc2
	.long	0x17b
	.byte	0x3
	.byte	0x23
	.byte	0x80,0x1
	.byte	0x9
	.ascii "Context\0"
	.byte	0x16
	.byte	0xc6
	.long	0x7f
	.byte	0x3
	.byte	0x23
	.byte	0x88,0x1
	.byte	0x0
	.byte	0xb
	.byte	0x8
	.long	0x156
	.byte	0x2
	.ascii "PE_COFF_LOADER_IMAGE_CONTEXT\0"
	.byte	0x16
	.byte	0xc7
	.long	0xb8a
	.byte	0x8
	.ascii "stat_fix\0"
	.byte	0x90
	.byte	0x17
	.byte	0x53
	.long	0xf49
	.byte	0x9
	.ascii "st_dev\0"
	.byte	0x17
	.byte	0x53
	.long	0x899
	.byte	0x2
	.byte	0x23
	.byte	0x0
	.byte	0x9
	.ascii "st_mode\0"
	.byte	0x17
	.byte	0x55
	.long	0x8b3
	.byte	0x2
	.byte	0x23
	.byte	0x4
	.byte	0x9
	.ascii "st_nlink\0"
	.byte	0x17
	.byte	0x56
	.long	0x93e
	.byte	0x2
	.byte	0x23
	.byte	0x6
	.byte	0x9
	.ascii "st_ino\0"
	.byte	0x17
	.byte	0x57
	.long	0x573
	.byte	0x2
	.byte	0x23
	.byte	0x8
	.byte	0x9
	.ascii "st_uid\0"
	.byte	0x17
	.byte	0x58
	.long	0x8c1
	.byte	0x2
	.byte	0x23
	.byte	0x10
	.byte	0x9
	.ascii "st_gid\0"
	.byte	0x17
	.byte	0x59
	.long	0x8a6
	.byte	0x2
	.byte	0x23
	.byte	0x14
	.byte	0x9
	.ascii "st_rdev\0"
	.byte	0x17
	.byte	0x5a
	.long	0x899
	.byte	0x2
	.byte	0x23
	.byte	0x18
	.byte	0x9
	.ascii "st_atimespec\0"
	.byte	0x17
	.byte	0x5b
	.long	0x60c
	.byte	0x2
	.byte	0x23
	.byte	0x20
	.byte	0x9
	.ascii "st_mtimespec\0"
	.byte	0x17
	.byte	0x5b
	.long	0x60c
	.byte	0x2
	.byte	0x23
	.byte	0x30
	.byte	0x9
	.ascii "st_ctimespec\0"
	.byte	0x17
	.byte	0x5b
	.long	0x60c
	.byte	0x2
	.byte	0x23
	.byte	0x40
	.byte	0x9
	.ascii "st_birthtimespec\0"
	.byte	0x17
	.byte	0x5b
	.long	0x60c
	.byte	0x2
	.byte	0x23
	.byte	0x50
	.byte	0x9
	.ascii "st_size\0"
	.byte	0x17
	.byte	0x5c
	.long	0x5ee
	.byte	0x2
	.byte	0x23
	.byte	0x60
	.byte	0x9
	.ascii "st_blocks\0"
	.byte	0x17
	.byte	0x5d
	.long	0x91d
	.byte	0x2
	.byte	0x23
	.byte	0x68
	.byte	0x9
	.ascii "st_blksize\0"
	.byte	0x17
	.byte	0x5e
	.long	0x92d
	.byte	0x2
	.byte	0x23
	.byte	0x70
	.byte	0x9
	.ascii "st_flags\0"
	.byte	0x17
	.byte	0x5f
	.long	0x458
	.byte	0x2
	.byte	0x23
	.byte	0x74
	.byte	0x9
	.ascii "st_gen\0"
	.byte	0x17
	.byte	0x60
	.long	0x458
	.byte	0x2
	.byte	0x23
	.byte	0x78
	.byte	0x9
	.ascii "st_lspare\0"
	.byte	0x17
	.byte	0x61
	.long	0x447
	.byte	0x2
	.byte	0x23
	.byte	0x7c
	.byte	0x9
	.ascii "st_qspare\0"
	.byte	0x17
	.byte	0x62
	.long	0x94d
	.byte	0x3
	.byte	0x23
	.byte	0x80,0x1
	.byte	0x0
	.byte	0x2
	.ascii "STAT_FIX\0"
	.byte	0x17
	.byte	0x63
	.long	0xdd4
	.byte	0xb
	.byte	0x8
	.long	0xf5f
	.byte	0x16
	.byte	0x1
	.long	0xf6b
	.byte	0x14
	.long	0x7f
	.byte	0x0
	.byte	0xb
	.byte	0x8
	.long	0x316
	.byte	0xb
	.byte	0x8
	.long	0x64f
	.byte	0xb
	.byte	0x8
	.long	0xf7d
	.byte	0xf
	.long	0x641
	.byte	0xb
	.byte	0x8
	.long	0x8ce
	.byte	0xb
	.byte	0x8
	.long	0xf8e
	.byte	0x17
	.byte	0x2
	.ascii "UnixRmDir\0"
	.byte	0x17
	.byte	0xf2
	.long	0xfa0
	.byte	0xb
	.byte	0x8
	.long	0xfa6
	.byte	0x13
	.byte	0x1
	.long	0xd6
	.long	0xfb6
	.byte	0x14
	.long	0x601
	.byte	0x0
	.byte	0x2
	.ascii "UnixOpenDir\0"
	.byte	0x17
	.byte	0xfe
	.long	0xfc9
	.byte	0xb
	.byte	0x8
	.long	0xfcf
	.byte	0x13
	.byte	0x1
	.long	0xfdf
	.long	0xfdf
	.byte	0x14
	.long	0x601
	.byte	0x0
	.byte	0xb
	.byte	0x8
	.long	0x88e
	.byte	0xb
	.byte	0x8
	.long	0x721
	.byte	0x5
	.ascii "UnixStat\0"
	.byte	0x17
	.word	0x10e
	.long	0xffc
	.byte	0xb
	.byte	0x8
	.long	0x1002
	.byte	0x13
	.byte	0x1
	.long	0xd6
	.long	0x1017
	.byte	0x14
	.long	0x601
	.byte	0x14
	.long	0x1017
	.byte	0x0
	.byte	0xb
	.byte	0x8
	.long	0xf49
	.byte	0x5
	.ascii "UnixStatFs\0"
	.byte	0x17
	.word	0x112
	.long	0x1030
	.byte	0xb
	.byte	0x8
	.long	0x1036
	.byte	0x13
	.byte	0x1
	.long	0xd6
	.long	0x104b
	.byte	0x14
	.long	0x601
	.byte	0x14
	.long	0x104b
	.byte	0x0
	.byte	0xb
	.byte	0x8
	.long	0x9c7
	.byte	0xb
	.byte	0x8
	.long	0x1057
	.byte	0x18
	.ascii "utimbuf\0"
	.long	0xb11
	.byte	0xb
	.byte	0x8
	.long	0x106a
	.byte	0xb
	.byte	0x8
	.long	0x1070
	.byte	0xd
	.ascii "_EFI_UNIX_UGA_IO_PROTOCOL\0"
	.byte	0x30
	.byte	0x17
	.word	0x128
	.long	0x1106
	.byte	0x9
	.ascii "Private\0"
	.byte	0x18
	.byte	0x46
	.long	0x226
	.byte	0x2
	.byte	0x23
	.byte	0x0
	.byte	0x9
	.ascii "UgaClose\0"
	.byte	0x18
	.byte	0x47
	.long	0x128f
	.byte	0x2
	.byte	0x23
	.byte	0x8
	.byte	0x9
	.ascii "UgaSize\0"
	.byte	0x18
	.byte	0x48
	.long	0x12bb
	.byte	0x2
	.byte	0x23
	.byte	0x10
	.byte	0x9
	.ascii "UgaCheckKey\0"
	.byte	0x18
	.byte	0x49
	.long	0x12ea
	.byte	0x2
	.byte	0x23
	.byte	0x18
	.byte	0x9
	.ascii "UgaGetKey\0"
	.byte	0x18
	.byte	0x4a
	.long	0x12fd
	.byte	0x2
	.byte	0x23
	.byte	0x20
	.byte	0x9
	.ascii "UgaBlt\0"
	.byte	0x18
	.byte	0x4b
	.long	0x1329
	.byte	0x2
	.byte	0x23
	.byte	0x28
	.byte	0x0
	.byte	0xb
	.byte	0x8
	.long	0x110c
	.byte	0xf
	.long	0x101
	.byte	0xb
	.byte	0x8
	.long	0x36f
	.byte	0xb
	.byte	0x8
	.long	0x111d
	.byte	0x18
	.ascii "termios\0"
	.long	0x36f
	.byte	0xb
	.byte	0x8
	.long	0xdb0
	.byte	0xc
	.byte	0x4
	.byte	0x19
	.byte	0x4b
	.long	0x1179
	.byte	0x9
	.ascii "Blue\0"
	.byte	0x19
	.byte	0x4c
	.long	0x149
	.byte	0x2
	.byte	0x23
	.byte	0x0
	.byte	0x9
	.ascii "Green\0"
	.byte	0x19
	.byte	0x4d
	.long	0x149
	.byte	0x2
	.byte	0x23
	.byte	0x1
	.byte	0x9
	.ascii "Red\0"
	.byte	0x19
	.byte	0x4e
	.long	0x149
	.byte	0x2
	.byte	0x23
	.byte	0x2
	.byte	0x9
	.ascii "Reserved\0"
	.byte	0x19
	.byte	0x4f
	.long	0x149
	.byte	0x2
	.byte	0x23
	.byte	0x3
	.byte	0x0
	.byte	0x2
	.ascii "EFI_UGA_PIXEL\0"
	.byte	0x19
	.byte	0x50
	.long	0x1130
	.byte	0x19
	.byte	0x4
	.byte	0x19
	.byte	0x5a
	.long	0x11ff
	.byte	0x1a
	.ascii "EfiUgaVideoFill\0"
	.byte	0x0
	.byte	0x1a
	.ascii "EfiUgaVideoToBltBuffer\0"
	.byte	0x1
	.byte	0x1a
	.ascii "EfiUgaBltBufferToVideo\0"
	.byte	0x2
	.byte	0x1a
	.ascii "EfiUgaVideoToVideo\0"
	.byte	0x3
	.byte	0x1a
	.ascii "EfiUgaBltMax\0"
	.byte	0x4
	.byte	0x0
	.byte	0x2
	.ascii "EFI_UGA_BLT_OPERATION\0"
	.byte	0x19
	.byte	0x75
	.long	0x118e
	.byte	0xb
	.byte	0x8
	.long	0x1179
	.byte	0xc
	.byte	0x4
	.byte	0x1a
	.byte	0x29
	.long	0x1254
	.byte	0x9
	.ascii "ScanCode\0"
	.byte	0x1a
	.byte	0x2a
	.long	0xdd
	.byte	0x2
	.byte	0x23
	.byte	0x0
	.byte	0x9
	.ascii "UnicodeChar\0"
	.byte	0x1a
	.byte	0x2b
	.long	0x101
	.byte	0x2
	.byte	0x23
	.byte	0x2
	.byte	0x0
	.byte	0x2
	.ascii "EFI_INPUT_KEY\0"
	.byte	0x1a
	.byte	0x2c
	.long	0x1222
	.byte	0xb
	.byte	0x8
	.long	0x1254
	.byte	0x2
	.ascii "EFI_UNIX_UGA_IO_PROTOCOL\0"
	.byte	0x18
	.byte	0x19
	.long	0x1070
	.byte	0x2
	.ascii "UGAClose\0"
	.byte	0x18
	.byte	0x1f
	.long	0x129f
	.byte	0xb
	.byte	0x8
	.long	0x12a5
	.byte	0x13
	.byte	0x1
	.long	0x244
	.long	0x12b5
	.byte	0x14
	.long	0x12b5
	.byte	0x0
	.byte	0xb
	.byte	0x8
	.long	0x126f
	.byte	0x2
	.ascii "UGASize\0"
	.byte	0x18
	.byte	0x27
	.long	0x12ca
	.byte	0xb
	.byte	0x8
	.long	0x12d0
	.byte	0x13
	.byte	0x1
	.long	0x244
	.long	0x12ea
	.byte	0x14
	.long	0x12b5
	.byte	0x14
	.long	0xb8
	.byte	0x14
	.long	0xb8
	.byte	0x0
	.byte	0x2
	.ascii "UGACheckKey\0"
	.byte	0x18
	.byte	0x2d
	.long	0x129f
	.byte	0x2
	.ascii "UGAGetKey\0"
	.byte	0x18
	.byte	0x34
	.long	0x130e
	.byte	0xb
	.byte	0x8
	.long	0x1314
	.byte	0x13
	.byte	0x1
	.long	0x244
	.long	0x1329
	.byte	0x14
	.long	0x12b5
	.byte	0x14
	.long	0x1269
	.byte	0x0
	.byte	0x2
	.ascii "UGABlt\0"
	.byte	0x18
	.byte	0x43
	.long	0x1337
	.byte	0xb
	.byte	0x8
	.long	0x133d
	.byte	0x13
	.byte	0x1
	.long	0x244
	.long	0x137a
	.byte	0x14
	.long	0x12b5
	.byte	0x14
	.long	0x121c
	.byte	0x14
	.long	0x11ff
	.byte	0x14
	.long	0x16b
	.byte	0x14
	.long	0x16b
	.byte	0x14
	.long	0x16b
	.byte	0x14
	.long	0x16b
	.byte	0x14
	.long	0x16b
	.byte	0x14
	.long	0x16b
	.byte	0x14
	.long	0x16b
	.byte	0x0
	.byte	0x1b
	.byte	0x1
	.set L$set$725,LASF1-Lsection__debug_str
	.long L$set$725
	.byte	0x1
	.byte	0x23
	.byte	0x1
	.long	0xd6
	.quad	LFB16
	.quad	LFE16
	.set L$set$726,LLST0-Lsection__debug_loc
	.long L$set$726
	.long	0x13ae
	.byte	0x1c
	.set L$set$727,LASF3-Lsection__debug_str
	.long L$set$727
	.byte	0x1
	.byte	0x22
	.long	0x601
	.byte	0x2
	.byte	0x91
	.byte	0x68
	.byte	0x0
	.byte	0x1b
	.byte	0x1
	.set L$set$728,LASF2-Lsection__debug_str
	.long L$set$728
	.byte	0x1
	.byte	0x2a
	.byte	0x1
	.long	0xfdf
	.quad	LFB17
	.quad	LFE17
	.set L$set$729,LLST1-Lsection__debug_loc
	.long L$set$729
	.long	0x13e2
	.byte	0x1c
	.set L$set$730,LASF3-Lsection__debug_str
	.long L$set$730
	.byte	0x1
	.byte	0x29
	.long	0x601
	.byte	0x2
	.byte	0x91
	.byte	0x68
	.byte	0x0
	.byte	0x1b
	.byte	0x1
	.set L$set$731,LASF4-Lsection__debug_str
	.long L$set$731
	.byte	0x1
	.byte	0x31
	.byte	0x1
	.long	0xd6
	.quad	LFB18
	.quad	LFE18
	.set L$set$732,LLST2-Lsection__debug_loc
	.long L$set$732
	.long	0x1425
	.byte	0x1d
	.ascii "path\0"
	.byte	0x1
	.byte	0x30
	.long	0x601
	.byte	0x2
	.byte	0x91
	.byte	0x68
	.byte	0x1d
	.ascii "buf\0"
	.byte	0x1
	.byte	0x30
	.long	0x1017
	.byte	0x2
	.byte	0x91
	.byte	0x60
	.byte	0x0
	.byte	0x1b
	.byte	0x1
	.set L$set$733,LASF5-Lsection__debug_str
	.long L$set$733
	.byte	0x1
	.byte	0x38
	.byte	0x1
	.long	0xd6
	.quad	LFB19
	.quad	LFE19
	.set L$set$734,LLST3-Lsection__debug_loc
	.long L$set$734
	.long	0x1468
	.byte	0x1d
	.ascii "path\0"
	.byte	0x1
	.byte	0x37
	.long	0x601
	.byte	0x2
	.byte	0x91
	.byte	0x68
	.byte	0x1d
	.ascii "buf\0"
	.byte	0x1
	.byte	0x37
	.long	0x104b
	.byte	0x2
	.byte	0x91
	.byte	0x60
	.byte	0x0
	.byte	0x1e
	.byte	0x1
	.set L$set$735,LASF6-Lsection__debug_str
	.long L$set$735
	.byte	0x1
	.byte	0x41
	.byte	0x1
	.quad	LFB20
	.quad	LFE20
	.set L$set$736,LLST4-Lsection__debug_loc
	.long L$set$736
	.long	0x14a1
	.byte	0x1d
	.ascii "Milliseconds\0"
	.byte	0x1
	.byte	0x40
	.long	0x33d
	.byte	0x2
	.byte	0x91
	.byte	0x68
	.byte	0x0
	.byte	0x1e
	.byte	0x1
	.set L$set$737,LASF7-Lsection__debug_str
	.long L$set$737
	.byte	0x1
	.byte	0x48
	.byte	0x1
	.quad	LFB21
	.quad	LFE21
	.set L$set$738,LLST5-Lsection__debug_loc
	.long L$set$738
	.long	0x14d4
	.byte	0x1d
	.ascii "status\0"
	.byte	0x1
	.byte	0x47
	.long	0xd6
	.byte	0x2
	.byte	0x91
	.byte	0x6c
	.byte	0x0
	.byte	0x1e
	.byte	0x1
	.set L$set$739,LASF8-Lsection__debug_str
	.long L$set$739
	.byte	0x1
	.byte	0x50
	.byte	0x1
	.quad	LFB22
	.quad	LFE22
	.set L$set$740,LLST6-Lsection__debug_loc
	.long L$set$740
	.long	0x151c
	.byte	0x1d
	.ascii "PeriodMs\0"
	.byte	0x1
	.byte	0x4f
	.long	0x7f
	.byte	0x2
	.byte	0x91
	.byte	0x68
	.byte	0x1d
	.ascii "CallBack\0"
	.byte	0x1
	.byte	0x4f
	.long	0xf59
	.byte	0x2
	.byte	0x91
	.byte	0x60
	.byte	0x0
	.byte	0x1e
	.byte	0x1
	.set L$set$741,LASF9-Lsection__debug_str
	.long L$set$741
	.byte	0x1
	.byte	0x58
	.byte	0x1
	.quad	LFB23
	.quad	LFE23
	.set L$set$742,LLST7-Lsection__debug_loc
	.long L$set$742
	.long	0x154d
	.byte	0x1d
	.ascii "Time\0"
	.byte	0x1
	.byte	0x57
	.long	0xf6b
	.byte	0x2
	.byte	0x91
	.byte	0x68
	.byte	0x0
	.byte	0x1b
	.byte	0x1
	.set L$set$743,LASF10-Lsection__debug_str
	.long L$set$743
	.byte	0x1
	.byte	0x60
	.byte	0x1
	.long	0xf71
	.quad	LFB24
	.quad	LFE24
	.set L$set$744,LLST8-Lsection__debug_loc
	.long L$set$744
	.long	0x1583
	.byte	0x1d
	.ascii "clock\0"
	.byte	0x1
	.byte	0x5f
	.long	0xf77
	.byte	0x2
	.byte	0x91
	.byte	0x68
	.byte	0x0
	.byte	0x1f
	.byte	0x1
	.set L$set$745,LASF11-Lsection__debug_str
	.long L$set$745
	.byte	0x1
	.byte	0x67
	.byte	0x1
	.long	0x48d
	.quad	LFB25
	.quad	LFE25
	.set L$set$746,LLST9-Lsection__debug_loc
	.long L$set$746
	.byte	0x1f
	.byte	0x1
	.set L$set$747,LASF12-Lsection__debug_str
	.long L$set$747
	.byte	0x1
	.byte	0x6e
	.byte	0x1
	.long	0xd6
	.quad	LFB26
	.quad	LFE26
	.set L$set$748,LLST10-Lsection__debug_loc
	.long L$set$748
	.byte	0x1b
	.byte	0x1
	.set L$set$749,LASF13-Lsection__debug_str
	.long L$set$749
	.byte	0x1
	.byte	0x75
	.byte	0x1
	.long	0xd6
	.quad	LFB27
	.quad	LFE27
	.set L$set$750,LLST11-Lsection__debug_loc
	.long L$set$750
	.long	0x161a
	.byte	0x1d
	.ascii "pfd\0"
	.byte	0x1
	.byte	0x74
	.long	0xf82
	.byte	0x2
	.byte	0x91
	.byte	0x68
	.byte	0x1d
	.ascii "nfds\0"
	.byte	0x1
	.byte	0x74
	.long	0xc6
	.byte	0x2
	.byte	0x91
	.byte	0x64
	.byte	0x1d
	.ascii "timeout\0"
	.byte	0x1
	.byte	0x74
	.long	0xd6
	.byte	0x2
	.byte	0x91
	.byte	0x60
	.byte	0x0
	.byte	0x1b
	.byte	0x1
	.set L$set$751,LASF14-Lsection__debug_str
	.long L$set$751
	.byte	0x1
	.byte	0x7c
	.byte	0x1
	.long	0x48d
	.quad	LFB28
	.quad	LFE28
	.set L$set$752,LLST12-Lsection__debug_loc
	.long L$set$752
	.long	0x166b
	.byte	0x1d
	.ascii "fd\0"
	.byte	0x1
	.byte	0x7b
	.long	0xd6
	.byte	0x2
	.byte	0x91
	.byte	0x6c
	.byte	0x1d
	.ascii "buf\0"
	.byte	0x1
	.byte	0x7b
	.long	0x226
	.byte	0x2
	.byte	0x91
	.byte	0x60
	.byte	0x1d
	.ascii "count\0"
	.byte	0x1
	.byte	0x7b
	.long	0xd6
	.byte	0x2
	.byte	0x91
	.byte	0x5c
	.byte	0x0
	.byte	0x1b
	.byte	0x1
	.set L$set$753,LASF15-Lsection__debug_str
	.long L$set$753
	.byte	0x1
	.byte	0x83
	.byte	0x1
	.long	0x48d
	.quad	LFB29
	.quad	LFE29
	.set L$set$754,LLST13-Lsection__debug_loc
	.long L$set$754
	.long	0x16bc
	.byte	0x1d
	.ascii "fd\0"
	.byte	0x1
	.byte	0x82
	.long	0xd6
	.byte	0x2
	.byte	0x91
	.byte	0x6c
	.byte	0x1d
	.ascii "buf\0"
	.byte	0x1
	.byte	0x82
	.long	0xf88
	.byte	0x2
	.byte	0x91
	.byte	0x60
	.byte	0x1d
	.ascii "count\0"
	.byte	0x1
	.byte	0x82
	.long	0xd6
	.byte	0x2
	.byte	0x91
	.byte	0x5c
	.byte	0x0
	.byte	0x1b
	.byte	0x1
	.set L$set$755,LASF16-Lsection__debug_str
	.long L$set$755
	.byte	0x1
	.byte	0x8a
	.byte	0x1
	.long	0x5fb
	.quad	LFB30
	.quad	LFE30
	.set L$set$756,LLST14-Lsection__debug_loc
	.long L$set$756
	.long	0x16f1
	.byte	0x1d
	.ascii "name\0"
	.byte	0x1
	.byte	0x89
	.long	0x601
	.byte	0x2
	.byte	0x91
	.byte	0x68
	.byte	0x0
	.byte	0x1b
	.byte	0x1
	.set L$set$757,LASF17-Lsection__debug_str
	.long L$set$757
	.byte	0x1
	.byte	0x91
	.byte	0x1
	.long	0xd6
	.quad	LFB31
	.quad	LFE31
	.set L$set$758,LLST15-Lsection__debug_loc
	.long L$set$758
	.long	0x1745
	.byte	0x1d
	.ascii "name\0"
	.byte	0x1
	.byte	0x90
	.long	0x601
	.byte	0x2
	.byte	0x91
	.byte	0x68
	.byte	0x1d
	.ascii "flags\0"
	.byte	0x1
	.byte	0x90
	.long	0xd6
	.byte	0x2
	.byte	0x91
	.byte	0x64
	.byte	0x1d
	.ascii "mode\0"
	.byte	0x1
	.byte	0x90
	.long	0xd6
	.byte	0x2
	.byte	0x91
	.byte	0x60
	.byte	0x0
	.byte	0x1b
	.byte	0x1
	.set L$set$759,LASF18-Lsection__debug_str
	.long L$set$759
	.byte	0x1
	.byte	0x98
	.byte	0x1
	.long	0x5ee
	.quad	LFB32
	.quad	LFE32
	.set L$set$760,LLST16-Lsection__debug_loc
	.long L$set$760
	.long	0x1797
	.byte	0x1d
	.ascii "fd\0"
	.byte	0x1
	.byte	0x97
	.long	0xd6
	.byte	0x2
	.byte	0x91
	.byte	0x6c
	.byte	0x1d
	.ascii "off\0"
	.byte	0x1
	.byte	0x97
	.long	0x5ee
	.byte	0x2
	.byte	0x91
	.byte	0x60
	.byte	0x1d
	.ascii "whence\0"
	.byte	0x1
	.byte	0x97
	.long	0xd6
	.byte	0x2
	.byte	0x91
	.byte	0x5c
	.byte	0x0
	.byte	0x1b
	.byte	0x1
	.set L$set$761,LASF19-Lsection__debug_str
	.long L$set$761
	.byte	0x1
	.byte	0x9f
	.byte	0x1
	.long	0xd6
	.quad	LFB33
	.quad	LFE33
	.set L$set$762,LLST17-Lsection__debug_loc
	.long L$set$762
	.long	0x17d8
	.byte	0x1d
	.ascii "fd\0"
	.byte	0x1
	.byte	0x9e
	.long	0xd6
	.byte	0x2
	.byte	0x91
	.byte	0x6c
	.byte	0x1d
	.ascii "len\0"
	.byte	0x1
	.byte	0x9e
	.long	0x48d
	.byte	0x2
	.byte	0x91
	.byte	0x60
	.byte	0x0
	.byte	0x1b
	.byte	0x1
	.set L$set$763,LASF20-Lsection__debug_str
	.long L$set$763
	.byte	0x1
	.byte	0xa6
	.byte	0x1
	.long	0xd6
	.quad	LFB34
	.quad	LFE34
	.set L$set$764,LLST18-Lsection__debug_loc
	.long L$set$764
	.long	0x180b
	.byte	0x1d
	.ascii "fd\0"
	.byte	0x1
	.byte	0xa5
	.long	0xd6
	.byte	0x2
	.byte	0x91
	.byte	0x6c
	.byte	0x0
	.byte	0x1b
	.byte	0x1
	.set L$set$765,LASF21-Lsection__debug_str
	.long L$set$765
	.byte	0x1
	.byte	0xad
	.byte	0x1
	.long	0xd6
	.quad	LFB35
	.quad	LFE35
	.set L$set$766,LLST19-Lsection__debug_loc
	.long L$set$766
	.long	0x184e
	.byte	0x1c
	.set L$set$767,LASF3-Lsection__debug_str
	.long L$set$767
	.byte	0x1
	.byte	0xac
	.long	0x601
	.byte	0x2
	.byte	0x91
	.byte	0x68
	.byte	0x1d
	.ascii "mode\0"
	.byte	0x1
	.byte	0xac
	.long	0x8b3
	.byte	0x2
	.byte	0x91
	.byte	0x64
	.byte	0x0
	.byte	0x1b
	.byte	0x1
	.set L$set$768,LASF22-Lsection__debug_str
	.long L$set$768
	.byte	0x1
	.byte	0xb4
	.byte	0x1
	.long	0xd6
	.quad	LFB36
	.quad	LFE36
	.set L$set$769,LLST20-Lsection__debug_loc
	.long L$set$769
	.long	0x1882
	.byte	0x1c
	.set L$set$770,LASF3-Lsection__debug_str
	.long L$set$770
	.byte	0x1
	.byte	0xb3
	.long	0x601
	.byte	0x2
	.byte	0x91
	.byte	0x68
	.byte	0x0
	.byte	0x1f
	.byte	0x1
	.set L$set$771,LASF23-Lsection__debug_str
	.long L$set$771
	.byte	0x1
	.byte	0xbb
	.byte	0x1
	.long	0xd6
	.quad	LFB37
	.quad	LFE37
	.set L$set$772,LLST21-Lsection__debug_loc
	.long L$set$772
	.byte	0x1e
	.byte	0x1
	.set L$set$773,LASF24-Lsection__debug_str
	.long L$set$773
	.byte	0x1
	.byte	0xc2
	.byte	0x1
	.quad	LFB38
	.quad	LFE38
	.set L$set$774,LLST22-Lsection__debug_loc
	.long L$set$774
	.long	0x18d3
	.byte	0x1d
	.ascii "dir\0"
	.byte	0x1
	.byte	0xc1
	.long	0xfdf
	.byte	0x2
	.byte	0x91
	.byte	0x68
	.byte	0x0
	.byte	0x1b
	.byte	0x1
	.set L$set$775,LASF25-Lsection__debug_str
	.long L$set$775
	.byte	0x1
	.byte	0xca
	.byte	0x1
	.long	0xfe5
	.quad	LFB39
	.quad	LFE39
	.set L$set$776,LLST23-Lsection__debug_loc
	.long L$set$776
	.long	0x1907
	.byte	0x1d
	.ascii "dir\0"
	.byte	0x1
	.byte	0xc9
	.long	0xfdf
	.byte	0x2
	.byte	0x91
	.byte	0x68
	.byte	0x0
	.byte	0x1b
	.byte	0x1
	.set L$set$777,LASF26-Lsection__debug_str
	.long L$set$777
	.byte	0x1
	.byte	0xd1
	.byte	0x1
	.long	0xd6
	.quad	LFB40
	.quad	LFE40
	.set L$set$778,LLST24-Lsection__debug_loc
	.long L$set$778
	.long	0x193b
	.byte	0x1d
	.ascii "dir\0"
	.byte	0x1
	.byte	0xd0
	.long	0xfdf
	.byte	0x2
	.byte	0x91
	.byte	0x68
	.byte	0x0
	.byte	0x1b
	.byte	0x1
	.set L$set$779,LASF27-Lsection__debug_str
	.long L$set$779
	.byte	0x1
	.byte	0xd8
	.byte	0x1
	.long	0xd6
	.quad	LFB41
	.quad	LFE41
	.set L$set$780,LLST25-Lsection__debug_loc
	.long L$set$780
	.long	0x1985
	.byte	0x1d
	.ascii "oldpath\0"
	.byte	0x1
	.byte	0xd7
	.long	0x601
	.byte	0x2
	.byte	0x91
	.byte	0x68
	.byte	0x1d
	.ascii "newpath\0"
	.byte	0x1
	.byte	0xd7
	.long	0x601
	.byte	0x2
	.byte	0x91
	.byte	0x60
	.byte	0x0
	.byte	0x1b
	.byte	0x1
	.set L$set$781,LASF28-Lsection__debug_str
	.long L$set$781
	.byte	0x1
	.byte	0xdf
	.byte	0x1
	.long	0x641
	.quad	LFB42
	.quad	LFE42
	.set L$set$782,LLST26-Lsection__debug_loc
	.long L$set$782
	.long	0x19b8
	.byte	0x1d
	.ascii "tm\0"
	.byte	0x1
	.byte	0xde
	.long	0xf71
	.byte	0x2
	.byte	0x91
	.byte	0x68
	.byte	0x0
	.byte	0x1b
	.byte	0x1
	.set L$set$783,LASF29-Lsection__debug_str
	.long L$set$783
	.byte	0x1
	.byte	0xe6
	.byte	0x1
	.long	0xd6
	.quad	LFB43
	.quad	LFE43
	.set L$set$784,LLST27-Lsection__debug_loc
	.long L$set$784
	.long	0x19eb
	.byte	0x1d
	.ascii "fd\0"
	.byte	0x1
	.byte	0xe5
	.long	0xd6
	.byte	0x2
	.byte	0x91
	.byte	0x6c
	.byte	0x0
	.byte	0x1b
	.byte	0x1
	.set L$set$785,LASF30-Lsection__debug_str
	.long L$set$785
	.byte	0x1
	.byte	0xed
	.byte	0x1
	.long	0xd6
	.quad	LFB44
	.quad	LFE44
	.set L$set$786,LLST28-Lsection__debug_loc
	.long L$set$786
	.long	0x1a2f
	.byte	0x1d
	.ascii "path\0"
	.byte	0x1
	.byte	0xec
	.long	0x601
	.byte	0x2
	.byte	0x91
	.byte	0x68
	.byte	0x1d
	.ascii "mode\0"
	.byte	0x1
	.byte	0xec
	.long	0x8b3
	.byte	0x2
	.byte	0x91
	.byte	0x64
	.byte	0x0
	.byte	0x1b
	.byte	0x1
	.set L$set$787,LASF31-Lsection__debug_str
	.long L$set$787
	.byte	0x1
	.byte	0xf4
	.byte	0x1
	.long	0xd6
	.quad	LFB45
	.quad	LFE45
	.set L$set$788,LLST29-Lsection__debug_loc
	.long L$set$788
	.long	0x1a76
	.byte	0x1d
	.ascii "filename\0"
	.byte	0x1
	.byte	0xf3
	.long	0x601
	.byte	0x2
	.byte	0x91
	.byte	0x68
	.byte	0x1d
	.ascii "buf\0"
	.byte	0x1
	.byte	0xf3
	.long	0x1051
	.byte	0x2
	.byte	0x91
	.byte	0x60
	.byte	0x0
	.byte	0x1b
	.byte	0x1
	.set L$set$789,LASF32-Lsection__debug_str
	.long L$set$789
	.byte	0x1
	.byte	0xfb
	.byte	0x1
	.long	0xd6
	.quad	LFB46
	.quad	LFE46
	.set L$set$790,LLST30-Lsection__debug_loc
	.long L$set$790
	.long	0x1ac6
	.byte	0x1d
	.ascii "fildes\0"
	.byte	0x1
	.byte	0xfa
	.long	0xd6
	.byte	0x2
	.byte	0x91
	.byte	0x6c
	.byte	0x1d
	.ascii "queue_selector\0"
	.byte	0x1
	.byte	0xfa
	.long	0xd6
	.byte	0x2
	.byte	0x91
	.byte	0x68
	.byte	0x0
	.byte	0x20
	.byte	0x1
	.set L$set$791,LASF33-Lsection__debug_str
	.long L$set$791
	.byte	0x1
	.word	0x102
	.byte	0x1
	.long	0x244
	.quad	LFB47
	.quad	LFE47
	.set L$set$792,LLST31-Lsection__debug_loc
	.long L$set$792
	.long	0x1b0d
	.byte	0x21
	.set L$set$793,LASF34-Lsection__debug_str
	.long L$set$793
	.byte	0x1
	.word	0x101
	.long	0x1064
	.byte	0x2
	.byte	0x91
	.byte	0x68
	.byte	0x22
	.ascii "Title\0"
	.byte	0x1
	.word	0x101
	.long	0x1106
	.byte	0x2
	.byte	0x91
	.byte	0x60
	.byte	0x0
	.byte	0x23
	.byte	0x1
	.set L$set$794,LASF35-Lsection__debug_str
	.long L$set$794
	.byte	0x1
	.word	0x109
	.byte	0x1
	.quad	LFB48
	.quad	LFE48
	.set L$set$795,LLST32-Lsection__debug_loc
	.long L$set$795
	.long	0x1b3f
	.byte	0x22
	.ascii "__s\0"
	.byte	0x1
	.word	0x108
	.long	0x601
	.byte	0x2
	.byte	0x91
	.byte	0x68
	.byte	0x0
	.byte	0x20
	.byte	0x1
	.set L$set$796,LASF36-Lsection__debug_str
	.long L$set$796
	.byte	0x1
	.word	0x115
	.byte	0x1
	.long	0xd6
	.quad	LFB49
	.quad	LFE49
	.set L$set$797,LLST33-Lsection__debug_loc
	.long L$set$797
	.long	0x1b9f
	.byte	0x22
	.ascii "fd\0"
	.byte	0x1
	.word	0x114
	.long	0xd6
	.byte	0x3
	.byte	0x91
	.byte	0x9c,0x7e
	.byte	0x22
	.ascii "__request\0"
	.byte	0x1
	.word	0x114
	.long	0x33d
	.byte	0x3
	.byte	0x91
	.byte	0x90,0x7e
	.byte	0x24
	.byte	0x25
	.ascii "Marker\0"
	.byte	0x1
	.word	0x116
	.long	0x193
	.byte	0x3
	.byte	0x91
	.byte	0xa0,0x7e
	.byte	0x0
	.byte	0x20
	.byte	0x1
	.set L$set$798,LASF37-Lsection__debug_str
	.long L$set$798
	.byte	0x1
	.word	0x11f
	.byte	0x1
	.long	0xd6
	.quad	LFB50
	.quad	LFE50
	.set L$set$799,LLST34-Lsection__debug_loc
	.long L$set$799
	.long	0x1bfd
	.byte	0x22
	.ascii "__fd\0"
	.byte	0x1
	.word	0x11e
	.long	0xd6
	.byte	0x3
	.byte	0x91
	.byte	0x9c,0x7e
	.byte	0x22
	.ascii "__cmd\0"
	.byte	0x1
	.word	0x11e
	.long	0xd6
	.byte	0x3
	.byte	0x91
	.byte	0x98,0x7e
	.byte	0x24
	.byte	0x25
	.ascii "Marker\0"
	.byte	0x1
	.word	0x120
	.long	0x193
	.byte	0x3
	.byte	0x91
	.byte	0xa0,0x7e
	.byte	0x0
	.byte	0x20
	.byte	0x1
	.set L$set$800,LASF38-Lsection__debug_str
	.long L$set$800
	.byte	0x1
	.word	0x12a
	.byte	0x1
	.long	0xd6
	.quad	LFB51
	.quad	LFE51
	.set L$set$801,LLST35-Lsection__debug_loc
	.long L$set$801
	.long	0x1c46
	.byte	0x21
	.set L$set$802,LASF39-Lsection__debug_str
	.long L$set$802
	.byte	0x1
	.word	0x129
	.long	0x1111
	.byte	0x2
	.byte	0x91
	.byte	0x68
	.byte	0x22
	.ascii "__speed\0"
	.byte	0x1
	.word	0x129
	.long	0x35f
	.byte	0x2
	.byte	0x91
	.byte	0x60
	.byte	0x0
	.byte	0x20
	.byte	0x1
	.set L$set$803,LASF40-Lsection__debug_str
	.long L$set$803
	.byte	0x1
	.word	0x131
	.byte	0x1
	.long	0xd6
	.quad	LFB52
	.quad	LFE52
	.set L$set$804,LLST36-Lsection__debug_loc
	.long L$set$804
	.long	0x1c8f
	.byte	0x21
	.set L$set$805,LASF39-Lsection__debug_str
	.long L$set$805
	.byte	0x1
	.word	0x130
	.long	0x1111
	.byte	0x2
	.byte	0x91
	.byte	0x68
	.byte	0x22
	.ascii "__speed\0"
	.byte	0x1
	.word	0x130
	.long	0x35f
	.byte	0x2
	.byte	0x91
	.byte	0x60
	.byte	0x0
	.byte	0x20
	.byte	0x1
	.set L$set$806,LASF41-Lsection__debug_str
	.long L$set$806
	.byte	0x1
	.word	0x138
	.byte	0x1
	.long	0xd6
	.quad	LFB53
	.quad	LFE53
	.set L$set$807,LLST37-Lsection__debug_loc
	.long L$set$807
	.long	0x1cd5
	.byte	0x22
	.ascii "__fd\0"
	.byte	0x1
	.word	0x137
	.long	0xd6
	.byte	0x2
	.byte	0x91
	.byte	0x6c
	.byte	0x21
	.set L$set$808,LASF39-Lsection__debug_str
	.long L$set$808
	.byte	0x1
	.word	0x137
	.long	0x1111
	.byte	0x2
	.byte	0x91
	.byte	0x60
	.byte	0x0
	.byte	0x20
	.byte	0x1
	.set L$set$809,LASF42-Lsection__debug_str
	.long L$set$809
	.byte	0x1
	.word	0x13f
	.byte	0x1
	.long	0xd6
	.quad	LFB54
	.quad	LFE54
	.set L$set$810,LLST38-Lsection__debug_loc
	.long L$set$810
	.long	0x1d39
	.byte	0x22
	.ascii "__fd\0"
	.byte	0x1
	.word	0x13e
	.long	0xd6
	.byte	0x2
	.byte	0x91
	.byte	0x6c
	.byte	0x22
	.ascii "__optional_actions\0"
	.byte	0x1
	.word	0x13e
	.long	0xd6
	.byte	0x2
	.byte	0x91
	.byte	0x68
	.byte	0x21
	.set L$set$811,LASF39-Lsection__debug_str
	.long L$set$811
	.byte	0x1
	.word	0x13e
	.long	0x1117
	.byte	0x2
	.byte	0x91
	.byte	0x60
	.byte	0x0
	.byte	0x20
	.byte	0x1
	.set L$set$812,LASF43-Lsection__debug_str
	.long L$set$812
	.byte	0x1
	.word	0x14b
	.byte	0x1
	.long	0x22e
	.quad	LFB55
	.quad	LFE55
	.set L$set$813,LLST39-Lsection__debug_loc
	.long L$set$813
	.long	0x1d83
	.byte	0x22
	.ascii "Pe32Data\0"
	.byte	0x1
	.word	0x148
	.long	0x226
	.byte	0x2
	.byte	0x91
	.byte	0x68
	.byte	0x21
	.set L$set$814,LASF0-Lsection__debug_str
	.long L$set$814
	.byte	0x1
	.word	0x14a
	.long	0x326
	.byte	0x2
	.byte	0x91
	.byte	0x60
	.byte	0x0
	.byte	0x23
	.byte	0x1
	.set L$set$815,LASF44-Lsection__debug_str
	.long L$set$815
	.byte	0x1
	.word	0x155
	.byte	0x1
	.quad	LFB56
	.quad	LFE56
	.set L$set$816,LLST40-Lsection__debug_loc
	.long L$set$816
	.long	0x1db5
	.byte	0x21
	.set L$set$817,LASF45-Lsection__debug_str
	.long L$set$817
	.byte	0x1
	.word	0x154
	.long	0x112a
	.byte	0x2
	.byte	0x91
	.byte	0x68
	.byte	0x0
	.byte	0x23
	.byte	0x1
	.set L$set$818,LASF46-Lsection__debug_str
	.long L$set$818
	.byte	0x1
	.word	0x160
	.byte	0x1
	.quad	LFB57
	.quad	LFE57
	.set L$set$819,LLST41-Lsection__debug_loc
	.long L$set$819
	.long	0x1de7
	.byte	0x21
	.set L$set$820,LASF45-Lsection__debug_str
	.long L$set$820
	.byte	0x1
	.word	0x15f
	.long	0x112a
	.byte	0x2
	.byte	0x91
	.byte	0x68
	.byte	0x0
	.byte	0x20
	.byte	0x1
	.set L$set$821,LASF47-Lsection__debug_str
	.long L$set$821
	.byte	0x1
	.word	0x16d
	.byte	0x1
	.long	0x244
	.quad	LFB58
	.quad	LFE58
	.set L$set$822,LLST42-Lsection__debug_loc
	.long L$set$822
	.long	0x1e1d
	.byte	0x21
	.set L$set$823,LASF34-Lsection__debug_str
	.long L$set$823
	.byte	0x1
	.word	0x16c
	.long	0x12b5
	.byte	0x2
	.byte	0x91
	.byte	0x68
	.byte	0x0
	.byte	0x20
	.byte	0x1
	.set L$set$824,LASF48-Lsection__debug_str
	.long L$set$824
	.byte	0x1
	.word	0x174
	.byte	0x1
	.long	0x244
	.quad	LFB59
	.quad	LFE59
	.set L$set$825,LLST43-Lsection__debug_loc
	.long L$set$825
	.long	0x1e76
	.byte	0x21
	.set L$set$826,LASF34-Lsection__debug_str
	.long L$set$826
	.byte	0x1
	.word	0x173
	.long	0x12b5
	.byte	0x2
	.byte	0x91
	.byte	0x68
	.byte	0x22
	.ascii "Width\0"
	.byte	0x1
	.word	0x173
	.long	0xb8
	.byte	0x2
	.byte	0x91
	.byte	0x64
	.byte	0x22
	.ascii "Height\0"
	.byte	0x1
	.word	0x173
	.long	0xb8
	.byte	0x2
	.byte	0x91
	.byte	0x60
	.byte	0x0
	.byte	0x20
	.byte	0x1
	.set L$set$827,LASF49-Lsection__debug_str
	.long L$set$827
	.byte	0x1
	.word	0x17b
	.byte	0x1
	.long	0x244
	.quad	LFB60
	.quad	LFE60
	.set L$set$828,LLST44-Lsection__debug_loc
	.long L$set$828
	.long	0x1eac
	.byte	0x21
	.set L$set$829,LASF34-Lsection__debug_str
	.long L$set$829
	.byte	0x1
	.word	0x17a
	.long	0x12b5
	.byte	0x2
	.byte	0x91
	.byte	0x68
	.byte	0x0
	.byte	0x20
	.byte	0x1
	.set L$set$830,LASF50-Lsection__debug_str
	.long L$set$830
	.byte	0x1
	.word	0x182
	.byte	0x1
	.long	0x244
	.quad	LFB61
	.quad	LFE61
	.set L$set$831,LLST45-Lsection__debug_loc
	.long L$set$831
	.long	0x1ef1
	.byte	0x21
	.set L$set$832,LASF34-Lsection__debug_str
	.long L$set$832
	.byte	0x1
	.word	0x181
	.long	0x12b5
	.byte	0x2
	.byte	0x91
	.byte	0x68
	.byte	0x22
	.ascii "key\0"
	.byte	0x1
	.word	0x181
	.long	0x1269
	.byte	0x2
	.byte	0x91
	.byte	0x60
	.byte	0x0
	.byte	0x20
	.byte	0x1
	.set L$set$833,LASF51-Lsection__debug_str
	.long L$set$833
	.byte	0x1
	.word	0x194
	.byte	0x1
	.long	0x244
	.quad	LFB62
	.quad	LFE62
	.set L$set$834,LLST46-Lsection__debug_loc
	.long L$set$834
	.long	0x1fde
	.byte	0x21
	.set L$set$835,LASF34-Lsection__debug_str
	.long L$set$835
	.byte	0x1
	.word	0x189
	.long	0x12b5
	.byte	0x2
	.byte	0x91
	.byte	0x68
	.byte	0x22
	.ascii "BltBuffer\0"
	.byte	0x1
	.word	0x18a
	.long	0x121c
	.byte	0x2
	.byte	0x91
	.byte	0x60
	.byte	0x22
	.ascii "BltOperation\0"
	.byte	0x1
	.word	0x18b
	.long	0x11ff
	.byte	0x2
	.byte	0x91
	.byte	0x5c
	.byte	0x22
	.ascii "SourceX\0"
	.byte	0x1
	.word	0x18c
	.long	0x16b
	.byte	0x2
	.byte	0x91
	.byte	0x50
	.byte	0x22
	.ascii "SourceY\0"
	.byte	0x1
	.word	0x18d
	.long	0x16b
	.byte	0x2
	.byte	0x91
	.byte	0x48
	.byte	0x22
	.ascii "DestinationX\0"
	.byte	0x1
	.word	0x18e
	.long	0x16b
	.byte	0x2
	.byte	0x91
	.byte	0x40
	.byte	0x22
	.ascii "DestinationY\0"
	.byte	0x1
	.word	0x18f
	.long	0x16b
	.byte	0x2
	.byte	0x91
	.byte	0x0
	.byte	0x22
	.ascii "Width\0"
	.byte	0x1
	.word	0x190
	.long	0x16b
	.byte	0x2
	.byte	0x91
	.byte	0x8
	.byte	0x22
	.ascii "Height\0"
	.byte	0x1
	.word	0x191
	.long	0x16b
	.byte	0x2
	.byte	0x91
	.byte	0x10
	.byte	0x22
	.ascii "Delta\0"
	.byte	0x1
	.word	0x193
	.long	0x16b
	.byte	0x2
	.byte	0x91
	.byte	0x18
	.byte	0x0
	.byte	0x5
	.ascii "SET_TIMER_CALLBACK\0"
	.byte	0x1
	.word	0x198
	.long	0xf59
	.byte	0x20
	.byte	0x1
	.set L$set$836,LASF52-Lsection__debug_str
	.long L$set$836
	.byte	0x1
	.word	0x19d
	.byte	0x1
	.long	0x16b
	.quad	LFB63
	.quad	LFE63
	.set L$set$837,LLST47-Lsection__debug_loc
	.long L$set$837
	.long	0x2059
	.byte	0x22
	.ascii "api\0"
	.byte	0x1
	.word	0x19c
	.long	0x226
	.byte	0x2
	.byte	0x91
	.byte	0x58
	.byte	0x22
	.ascii "a\0"
	.byte	0x1
	.word	0x19c
	.long	0x7f
	.byte	0x2
	.byte	0x91
	.byte	0x50
	.byte	0x25
	.ascii "settimer_callback\0"
	.byte	0x1
	.word	0x19e
	.long	0x1fde
	.byte	0x2
	.byte	0x91
	.byte	0x68
	.byte	0x0
	.byte	0x6
	.long	0xd6
	.long	0x2064
	.byte	0x26
	.byte	0x0
	.byte	0x27
	.ascii "__CFConstantStringClassReference\0"
	.long	0x2059
	.byte	0x1
	.byte	0x1
	.byte	0x1
	.byte	0x28
	.ascii "gUnixRmDir\0"
	.byte	0x1
	.byte	0x18
	.long	0xf8f
	.byte	0x1
	.byte	0x1
	.byte	0x28
	.ascii "gUnixOpenDir\0"
	.byte	0x1
	.byte	0x19
	.long	0xfb6
	.byte	0x1
	.byte	0x1
	.byte	0x28
	.ascii "gUnixStat\0"
	.byte	0x1
	.byte	0x1a
	.long	0xfeb
	.byte	0x1
	.byte	0x1
	.byte	0x28
	.ascii "gUnixStatFs\0"
	.byte	0x1
	.byte	0x1b
	.long	0x101d
	.byte	0x1
	.byte	0x1
	.byte	0x0
	.section __DWARF,__debug_abbrev,regular,debug
	.byte	0x1
	.byte	0x11
	.byte	0x1
	.byte	0x25
	.byte	0x8
	.byte	0x13
	.byte	0xb
	.byte	0x3
	.byte	0x8
	.byte	0x11
	.byte	0x1
	.byte	0x12
	.byte	0x1
	.byte	0x10
	.byte	0x6
	.byte	0x0
	.byte	0x0
	.byte	0x2
	.byte	0x16
	.byte	0x0
	.byte	0x3
	.byte	0x8
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0xb
	.byte	0x49
	.byte	0x13
	.byte	0x0
	.byte	0x0
	.byte	0x3
	.byte	0x24
	.byte	0x0
	.byte	0xb
	.byte	0xb
	.byte	0x3e
	.byte	0xb
	.byte	0x3
	.byte	0x8
	.byte	0x0
	.byte	0x0
	.byte	0x4
	.byte	0x24
	.byte	0x0
	.byte	0xb
	.byte	0xb
	.byte	0x3e
	.byte	0xb
	.byte	0x0
	.byte	0x0
	.byte	0x5
	.byte	0x16
	.byte	0x0
	.byte	0x3
	.byte	0x8
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0x5
	.byte	0x49
	.byte	0x13
	.byte	0x0
	.byte	0x0
	.byte	0x6
	.byte	0x1
	.byte	0x1
	.byte	0x49
	.byte	0x13
	.byte	0x1
	.byte	0x13
	.byte	0x0
	.byte	0x0
	.byte	0x7
	.byte	0x21
	.byte	0x0
	.byte	0x49
	.byte	0x13
	.byte	0x2f
	.byte	0xb
	.byte	0x0
	.byte	0x0
	.byte	0x8
	.byte	0x13
	.byte	0x1
	.byte	0x3
	.byte	0x8
	.byte	0xb
	.byte	0xb
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0xb
	.byte	0x1
	.byte	0x13
	.byte	0x0
	.byte	0x0
	.byte	0x9
	.byte	0xd
	.byte	0x0
	.byte	0x3
	.byte	0x8
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0xb
	.byte	0x49
	.byte	0x13
	.byte	0x38
	.byte	0xa
	.byte	0x0
	.byte	0x0
	.byte	0xa
	.byte	0xf
	.byte	0x0
	.byte	0xb
	.byte	0xb
	.byte	0x0
	.byte	0x0
	.byte	0xb
	.byte	0xf
	.byte	0x0
	.byte	0xb
	.byte	0xb
	.byte	0x49
	.byte	0x13
	.byte	0x0
	.byte	0x0
	.byte	0xc
	.byte	0x13
	.byte	0x1
	.byte	0xb
	.byte	0xb
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0xb
	.byte	0x1
	.byte	0x13
	.byte	0x0
	.byte	0x0
	.byte	0xd
	.byte	0x13
	.byte	0x1
	.byte	0x3
	.byte	0x8
	.byte	0xb
	.byte	0xb
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0x5
	.byte	0x1
	.byte	0x13
	.byte	0x0
	.byte	0x0
	.byte	0xe
	.byte	0xd
	.byte	0x0
	.byte	0x3
	.byte	0x8
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0x5
	.byte	0x49
	.byte	0x13
	.byte	0x38
	.byte	0xa
	.byte	0x0
	.byte	0x0
	.byte	0xf
	.byte	0x26
	.byte	0x0
	.byte	0x49
	.byte	0x13
	.byte	0x0
	.byte	0x0
	.byte	0x10
	.byte	0x13
	.byte	0x1
	.byte	0x3
	.byte	0x8
	.byte	0xb
	.byte	0x5
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0xb
	.byte	0x1
	.byte	0x13
	.byte	0x0
	.byte	0x0
	.byte	0x11
	.byte	0x21
	.byte	0x0
	.byte	0x49
	.byte	0x13
	.byte	0x2f
	.byte	0x5
	.byte	0x0
	.byte	0x0
	.byte	0x12
	.byte	0x13
	.byte	0x0
	.byte	0x3
	.byte	0x8
	.byte	0x3c
	.byte	0xc
	.byte	0x0
	.byte	0x0
	.byte	0x13
	.byte	0x15
	.byte	0x1
	.byte	0x27
	.byte	0xc
	.byte	0x49
	.byte	0x13
	.byte	0x1
	.byte	0x13
	.byte	0x0
	.byte	0x0
	.byte	0x14
	.byte	0x5
	.byte	0x0
	.byte	0x49
	.byte	0x13
	.byte	0x0
	.byte	0x0
	.byte	0x15
	.byte	0xd
	.byte	0x0
	.byte	0x3
	.byte	0xe
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0xb
	.byte	0x49
	.byte	0x13
	.byte	0x38
	.byte	0xa
	.byte	0x0
	.byte	0x0
	.byte	0x16
	.byte	0x15
	.byte	0x1
	.byte	0x27
	.byte	0xc
	.byte	0x1
	.byte	0x13
	.byte	0x0
	.byte	0x0
	.byte	0x17
	.byte	0x26
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.byte	0x18
	.byte	0x26
	.byte	0x0
	.byte	0x3
	.byte	0x8
	.byte	0x49
	.byte	0x13
	.byte	0x0
	.byte	0x0
	.byte	0x19
	.byte	0x4
	.byte	0x1
	.byte	0xb
	.byte	0xb
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0xb
	.byte	0x1
	.byte	0x13
	.byte	0x0
	.byte	0x0
	.byte	0x1a
	.byte	0x28
	.byte	0x0
	.byte	0x3
	.byte	0x8
	.byte	0x1c
	.byte	0xd
	.byte	0x0
	.byte	0x0
	.byte	0x1b
	.byte	0x2e
	.byte	0x1
	.byte	0x3f
	.byte	0xc
	.byte	0x3
	.byte	0xe
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0xb
	.byte	0x27
	.byte	0xc
	.byte	0x49
	.byte	0x13
	.byte	0x11
	.byte	0x1
	.byte	0x12
	.byte	0x1
	.byte	0x40
	.byte	0x6
	.byte	0x1
	.byte	0x13
	.byte	0x0
	.byte	0x0
	.byte	0x1c
	.byte	0x5
	.byte	0x0
	.byte	0x3
	.byte	0xe
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0xb
	.byte	0x49
	.byte	0x13
	.byte	0x2
	.byte	0xa
	.byte	0x0
	.byte	0x0
	.byte	0x1d
	.byte	0x5
	.byte	0x0
	.byte	0x3
	.byte	0x8
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0xb
	.byte	0x49
	.byte	0x13
	.byte	0x2
	.byte	0xa
	.byte	0x0
	.byte	0x0
	.byte	0x1e
	.byte	0x2e
	.byte	0x1
	.byte	0x3f
	.byte	0xc
	.byte	0x3
	.byte	0xe
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0xb
	.byte	0x27
	.byte	0xc
	.byte	0x11
	.byte	0x1
	.byte	0x12
	.byte	0x1
	.byte	0x40
	.byte	0x6
	.byte	0x1
	.byte	0x13
	.byte	0x0
	.byte	0x0
	.byte	0x1f
	.byte	0x2e
	.byte	0x0
	.byte	0x3f
	.byte	0xc
	.byte	0x3
	.byte	0xe
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0xb
	.byte	0x27
	.byte	0xc
	.byte	0x49
	.byte	0x13
	.byte	0x11
	.byte	0x1
	.byte	0x12
	.byte	0x1
	.byte	0x40
	.byte	0x6
	.byte	0x0
	.byte	0x0
	.byte	0x20
	.byte	0x2e
	.byte	0x1
	.byte	0x3f
	.byte	0xc
	.byte	0x3
	.byte	0xe
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0x5
	.byte	0x27
	.byte	0xc
	.byte	0x49
	.byte	0x13
	.byte	0x11
	.byte	0x1
	.byte	0x12
	.byte	0x1
	.byte	0x40
	.byte	0x6
	.byte	0x1
	.byte	0x13
	.byte	0x0
	.byte	0x0
	.byte	0x21
	.byte	0x5
	.byte	0x0
	.byte	0x3
	.byte	0xe
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0x5
	.byte	0x49
	.byte	0x13
	.byte	0x2
	.byte	0xa
	.byte	0x0
	.byte	0x0
	.byte	0x22
	.byte	0x5
	.byte	0x0
	.byte	0x3
	.byte	0x8
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0x5
	.byte	0x49
	.byte	0x13
	.byte	0x2
	.byte	0xa
	.byte	0x0
	.byte	0x0
	.byte	0x23
	.byte	0x2e
	.byte	0x1
	.byte	0x3f
	.byte	0xc
	.byte	0x3
	.byte	0xe
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0x5
	.byte	0x27
	.byte	0xc
	.byte	0x11
	.byte	0x1
	.byte	0x12
	.byte	0x1
	.byte	0x40
	.byte	0x6
	.byte	0x1
	.byte	0x13
	.byte	0x0
	.byte	0x0
	.byte	0x24
	.byte	0x18
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.byte	0x25
	.byte	0x34
	.byte	0x0
	.byte	0x3
	.byte	0x8
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0x5
	.byte	0x49
	.byte	0x13
	.byte	0x2
	.byte	0xa
	.byte	0x0
	.byte	0x0
	.byte	0x26
	.byte	0x21
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.byte	0x27
	.byte	0x34
	.byte	0x0
	.byte	0x3
	.byte	0x8
	.byte	0x49
	.byte	0x13
	.byte	0x3f
	.byte	0xc
	.byte	0x34
	.byte	0xc
	.byte	0x3c
	.byte	0xc
	.byte	0x0
	.byte	0x0
	.byte	0x28
	.byte	0x34
	.byte	0x0
	.byte	0x3
	.byte	0x8
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0xb
	.byte	0x49
	.byte	0x13
	.byte	0x3f
	.byte	0xc
	.byte	0x3c
	.byte	0xc
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.section __DWARF,__debug_pubnames,regular,debug
	.long	0x3b6
	.word	0x2
	.set L$set$838,Ldebug_info0-Lsection__debug_info
	.long L$set$838
	.long	0x20e0
	.long	0x137a
	.ascii "Gasketrmdir\0"
	.long	0x13ae
	.ascii "Gasketopendir\0"
	.long	0x13e2
	.ascii "Gasketstat\0"
	.long	0x1425
	.ascii "Gasketstatfs\0"
	.long	0x1468
	.ascii "GasketmsSleep\0"
	.long	0x14a1
	.ascii "Gasketexit\0"
	.long	0x14d4
	.ascii "GasketSetTimer\0"
	.long	0x151c
	.ascii "GasketGetLocalTime\0"
	.long	0x154d
	.ascii "Gasketgmtime\0"
	.long	0x1583
	.ascii "GasketGetTimeZone\0"
	.long	0x15a4
	.ascii "GasketGetDayLight\0"
	.long	0x15c5
	.ascii "Gasketpoll\0"
	.long	0x161a
	.ascii "Gasketread\0"
	.long	0x166b
	.ascii "Gasketwrite\0"
	.long	0x16bc
	.ascii "Gasketgetenv\0"
	.long	0x16f1
	.ascii "Gasketopen\0"
	.long	0x1745
	.ascii "Gasketlseek\0"
	.long	0x1797
	.ascii "Gasketftruncate\0"
	.long	0x17d8
	.ascii "Gasketclose\0"
	.long	0x180b
	.ascii "Gasketmkdir\0"
	.long	0x184e
	.ascii "Gasketunlink\0"
	.long	0x1882
	.ascii "GasketGetErrno\0"
	.long	0x18a3
	.ascii "Gasketrewinddir\0"
	.long	0x18d3
	.ascii "Gasketreaddir\0"
	.long	0x1907
	.ascii "Gasketclosedir\0"
	.long	0x193b
	.ascii "Gasketrename\0"
	.long	0x1985
	.ascii "Gasketmktime\0"
	.long	0x19b8
	.ascii "Gasketfsync\0"
	.long	0x19eb
	.ascii "Gasketchmod\0"
	.long	0x1a2f
	.ascii "Gasketutime\0"
	.long	0x1a76
	.ascii "Gaskettcflush\0"
	.long	0x1ac6
	.ascii "GasketUgaCreate\0"
	.long	0x1b0d
	.ascii "Gasketperror\0"
	.long	0x1b3f
	.ascii "Gasketioctl\0"
	.long	0x1b9f
	.ascii "Gasketfcntl\0"
	.long	0x1bfd
	.ascii "Gasketcfsetispeed\0"
	.long	0x1c46
	.ascii "Gasketcfsetospeed\0"
	.long	0x1c8f
	.ascii "Gaskettcgetattr\0"
	.long	0x1cd5
	.ascii "Gaskettcsetattr\0"
	.long	0x1d39
	.ascii "GasketUnixPeCoffGetEntryPoint\0"
	.long	0x1d83
	.ascii "GasketUnixPeCoffRelocateImageExtraAction\0"
	.long	0x1db5
	.ascii "GasketUnixPeCoffUnloadImageExtraAction\0"
	.long	0x1de7
	.ascii "GasketUgaClose\0"
	.long	0x1e1d
	.ascii "GasketUgaSize\0"
	.long	0x1e76
	.ascii "GasketUgaCheckKey\0"
	.long	0x1eac
	.ascii "GasketUgaGetKey\0"
	.long	0x1ef1
	.ascii "GasketUgaBlt\0"
	.long	0x1ff9
	.ascii "ReverseGasketUint64\0"
	.long	0x0
	.section __DWARF,__debug_pubtypes,regular,debug
	.long	0x472
	.word	0x2
	.set L$set$839,Ldebug_info0-Lsection__debug_info
	.long L$set$839
	.long	0x20e0
	.long	0x7f
	.ascii "UINT64\0"
	.long	0xb8
	.ascii "UINT32\0"
	.long	0xdd
	.ascii "UINT16\0"
	.long	0x101
	.ascii "CHAR16\0"
	.long	0x10f
	.ascii "INT16\0"
	.long	0x129
	.ascii "BOOLEAN\0"
	.long	0x149
	.ascii "UINT8\0"
	.long	0x156
	.ascii "CHAR8\0"
	.long	0x16b
	.ascii "UINTN\0"
	.long	0x17b
	.ascii "PHYSICAL_ADDRESS\0"
	.long	0x1b3
	.ascii "__va_list_tag\0"
	.long	0x193
	.ascii "VA_LIST\0"
	.long	0x22e
	.ascii "RETURN_STATUS\0"
	.long	0x244
	.ascii "EFI_STATUS\0"
	.long	0x316
	.ascii "EFI_TIME\0"
	.long	0x32c
	.ascii "tcflag_t\0"
	.long	0x352
	.ascii "cc_t\0"
	.long	0x35f
	.ascii "speed_t\0"
	.long	0x36f
	.ascii "termios\0"
	.long	0x424
	.ascii "__uint8_t\0"
	.long	0x435
	.ascii "__uint16_t\0"
	.long	0x447
	.ascii "__int32_t\0"
	.long	0x458
	.ascii "__uint32_t\0"
	.long	0x46a
	.ascii "__int64_t\0"
	.long	0x47b
	.ascii "__uint64_t\0"
	.long	0x499
	.ascii "__darwin_time_t\0"
	.long	0x4c0
	.ascii "_opaque_pthread_mutex_t\0"
	.long	0x514
	.ascii "__darwin_blkcnt_t\0"
	.long	0x52d
	.ascii "__darwin_blksize_t\0"
	.long	0x547
	.ascii "__darwin_dev_t\0"
	.long	0x55d
	.ascii "__darwin_gid_t\0"
	.long	0x573
	.ascii "__darwin_ino64_t\0"
	.long	0x58b
	.ascii "__darwin_mode_t\0"
	.long	0x5a2
	.ascii "__darwin_off_t\0"
	.long	0x5b8
	.ascii "__darwin_pthread_mutex_t\0"
	.long	0x5d8
	.ascii "__darwin_uid_t\0"
	.long	0x5ee
	.ascii "off_t\0"
	.long	0x60c
	.ascii "timespec\0"
	.long	0x641
	.ascii "time_t\0"
	.long	0x64f
	.ascii "tm\0"
	.long	0x721
	.ascii "dirent\0"
	.long	0x88e
	.ascii "DIR\0"
	.long	0x899
	.ascii "dev_t\0"
	.long	0x8a6
	.ascii "gid_t\0"
	.long	0x8b3
	.ascii "mode_t\0"
	.long	0x8c1
	.ascii "uid_t\0"
	.long	0x8ce
	.ascii "pollfd\0"
	.long	0x90e
	.ascii "int32_t\0"
	.long	0x91d
	.ascii "blkcnt_t\0"
	.long	0x92d
	.ascii "blksize_t\0"
	.long	0x93e
	.ascii "nlink_t\0"
	.long	0x95d
	.ascii "uint32_t\0"
	.long	0x96d
	.ascii "uint64_t\0"
	.long	0x97d
	.ascii "fsid\0"
	.long	0x9a9
	.ascii "fsid_t\0"
	.long	0x9c7
	.ascii "statfs\0"
	.long	0xb11
	.ascii "utimbuf\0"
	.long	0xb45
	.ascii "PE_COFF_LOADER_READ_FILE\0"
	.long	0xdb0
	.ascii "PE_COFF_LOADER_IMAGE_CONTEXT\0"
	.long	0xdd4
	.ascii "stat_fix\0"
	.long	0xf49
	.ascii "STAT_FIX\0"
	.long	0xf8f
	.ascii "UnixRmDir\0"
	.long	0xfb6
	.ascii "UnixOpenDir\0"
	.long	0xfeb
	.ascii "UnixStat\0"
	.long	0x101d
	.ascii "UnixStatFs\0"
	.long	0x1179
	.ascii "EFI_UGA_PIXEL\0"
	.long	0x11ff
	.ascii "EFI_UGA_BLT_OPERATION\0"
	.long	0x1254
	.ascii "EFI_INPUT_KEY\0"
	.long	0x128f
	.ascii "UGAClose\0"
	.long	0x12bb
	.ascii "UGASize\0"
	.long	0x12ea
	.ascii "UGACheckKey\0"
	.long	0x12fd
	.ascii "UGAGetKey\0"
	.long	0x1329
	.ascii "UGABlt\0"
	.long	0x1070
	.ascii "_EFI_UNIX_UGA_IO_PROTOCOL\0"
	.long	0x1fde
	.ascii "SET_TIMER_CALLBACK\0"
	.long	0x0
	.section __DWARF,__debug_aranges,regular,debug
	.long	0x2c
	.word	0x2
	.set L$set$840,Ldebug_info0-Lsection__debug_info
	.long L$set$840
	.byte	0x8
	.byte	0x0
	.word	0x0
	.word	0x0
	.quad	Ltext0
	.set L$set$841,Letext0-Ltext0
	.quad L$set$841
	.quad	0x0
	.quad	0x0
	.section __DWARF,__debug_str,regular,debug
LASF17:
	.ascii "Gasketopen\0"
LASF18:
	.ascii "Gasketlseek\0"
LASF48:
	.ascii "GasketUgaSize\0"
LASF52:
	.ascii "ReverseGasketUint64\0"
LASF40:
	.ascii "Gasketcfsetospeed\0"
LASF31:
	.ascii "Gasketutime\0"
LASF41:
	.ascii "Gaskettcgetattr\0"
LASF38:
	.ascii "Gasketcfsetispeed\0"
LASF42:
	.ascii "Gaskettcsetattr\0"
LASF15:
	.ascii "Gasketwrite\0"
LASF1:
	.ascii "Gasketrmdir\0"
LASF43:
	.ascii "GasketUnixPeCoffGetEntryPoint\0"
LASF36:
	.ascii "Gasketioctl\0"
LASF39:
	.ascii "__termios_p\0"
LASF23:
	.ascii "GasketGetErrno\0"
LASF26:
	.ascii "Gasketclosedir\0"
LASF44:
	.ascii "GasketUnixPeCoffRelocateImageExtraAction\0"
LASF13:
	.ascii "Gasketpoll\0"
LASF49:
	.ascii "GasketUgaCheckKey\0"
LASF50:
	.ascii "GasketUgaGetKey\0"
LASF11:
	.ascii "GasketGetTimeZone\0"
LASF25:
	.ascii "Gasketreaddir\0"
LASF46:
	.ascii "GasketUnixPeCoffUnloadImageExtraAction\0"
LASF34:
	.ascii "UgaIo\0"
LASF12:
	.ascii "GasketGetDayLight\0"
LASF6:
	.ascii "GasketmsSleep\0"
LASF51:
	.ascii "GasketUgaBlt\0"
LASF32:
	.ascii "Gaskettcflush\0"
LASF22:
	.ascii "Gasketunlink\0"
LASF27:
	.ascii "Gasketrename\0"
LASF8:
	.ascii "GasketSetTimer\0"
LASF24:
	.ascii "Gasketrewinddir\0"
LASF21:
	.ascii "Gasketmkdir\0"
LASF4:
	.ascii "Gasketstat\0"
LASF7:
	.ascii "Gasketexit\0"
LASF9:
	.ascii "GasketGetLocalTime\0"
LASF20:
	.ascii "Gasketclose\0"
LASF35:
	.ascii "Gasketperror\0"
LASF45:
	.ascii "ImageContext\0"
LASF2:
	.ascii "Gasketopendir\0"
LASF28:
	.ascii "Gasketmktime\0"
LASF30:
	.ascii "Gasketchmod\0"
LASF37:
	.ascii "Gasketfcntl\0"
LASF3:
	.ascii "pathname\0"
LASF0:
	.ascii "EntryPoint\0"
LASF5:
	.ascii "Gasketstatfs\0"
LASF33:
	.ascii "GasketUgaCreate\0"
LASF19:
	.ascii "Gasketftruncate\0"
LASF16:
	.ascii "Gasketgetenv\0"
LASF47:
	.ascii "GasketUgaClose\0"
LASF14:
	.ascii "Gasketread\0"
LASF10:
	.ascii "Gasketgmtime\0"
LASF29:
	.ascii "Gasketfsync\0"
	.subsections_via_symbols

#endif

