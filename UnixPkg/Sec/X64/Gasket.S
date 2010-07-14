	.text
.globl _GasketVoid
_GasketVoid:
LFB2:
	pushq	%rbp
LCFI0:
	movq	%rsp, %rbp
LCFI1:
	subq	$32, %rsp
LCFI2:
	movq	%rdi, -24(%rbp)
	movq	-24(%rbp), %rax
	movq	%rax, -8(%rbp)
	movq	-8(%rbp), %rdx
	movl	$0, %eax
	call	*%rdx
	leave
	ret
LFE2:
.globl _GasketUintn
_GasketUintn:
LFB3:
	pushq	%rbp
LCFI3:
	movq	%rsp, %rbp
LCFI4:
	subq	$32, %rsp
LCFI5:
	movq	%rdi, -24(%rbp)
	movl	%esi, -28(%rbp)
	movq	-24(%rbp), %rax
	movq	%rax, -8(%rbp)
	movl	-28(%rbp), %edi
	movq	-8(%rbp), %rax
	call	*%rax
	leave
	ret
LFE3:
.globl _GasketUintnUintn
_GasketUintnUintn:
LFB4:
	pushq	%rbp
LCFI6:
	movq	%rsp, %rbp
LCFI7:
	subq	$32, %rsp
LCFI8:
	movq	%rdi, -24(%rbp)
	movl	%esi, -28(%rbp)
	movl	%edx, -32(%rbp)
	movq	-24(%rbp), %rax
	movq	%rax, -8(%rbp)
	movl	-32(%rbp), %esi
	movl	-28(%rbp), %edi
	movq	-8(%rbp), %rax
	call	*%rax
	leave
	ret
LFE4:
.globl _GasketUintnUintnUintn
_GasketUintnUintnUintn:
LFB5:
	pushq	%rbp
LCFI9:
	movq	%rsp, %rbp
LCFI10:
	subq	$48, %rsp
LCFI11:
	movq	%rdi, -24(%rbp)
	movl	%esi, -28(%rbp)
	movl	%edx, -32(%rbp)
	movl	%ecx, -36(%rbp)
	movq	-24(%rbp), %rax
	movq	%rax, -8(%rbp)
	movl	-36(%rbp), %edx
	movl	-32(%rbp), %esi
	movl	-28(%rbp), %edi
	movq	-8(%rbp), %rax
	call	*%rax
	leave
	ret
LFE5:
.globl _GasketUintnUintnUintnUintn
_GasketUintnUintnUintnUintn:
LFB6:
	pushq	%rbp
LCFI12:
	movq	%rsp, %rbp
LCFI13:
	subq	$48, %rsp
LCFI14:
	movq	%rdi, -24(%rbp)
	movl	%esi, -28(%rbp)
	movl	%edx, -32(%rbp)
	movl	%ecx, -36(%rbp)
	movl	%r8d, -40(%rbp)
	movq	-24(%rbp), %rax
	movq	%rax, -8(%rbp)
	movl	-40(%rbp), %ecx
	movl	-36(%rbp), %edx
	movl	-32(%rbp), %esi
	movl	-28(%rbp), %edi
	movq	-8(%rbp), %rax
	call	*%rax
	leave
	ret
LFE6:
.globl _GasketUintn10Args
_GasketUintn10Args:
LFB7:
	pushq	%rbp
LCFI15:
	movq	%rsp, %rbp
LCFI16:
	subq	$80, %rsp
LCFI17:
	movq	%rdi, -24(%rbp)
	movl	%esi, -28(%rbp)
	movl	%edx, -32(%rbp)
	movl	%ecx, -36(%rbp)
	movl	%r8d, -40(%rbp)
	movl	%r9d, -44(%rbp)
	movq	-24(%rbp), %rax
	movq	%rax, -8(%rbp)
	movl	-44(%rbp), %edx
	movl	-40(%rbp), %ecx
	movl	-36(%rbp), %esi
	movl	-32(%rbp), %edi
	movl	-28(%rbp), %r10d
	movl	48(%rbp), %eax
	movl	%eax, 24(%rsp)
	movl	40(%rbp), %eax
	movl	%eax, 16(%rsp)
	movl	32(%rbp), %eax
	movl	%eax, 8(%rsp)
	movl	24(%rbp), %eax
	movl	%eax, (%rsp)
	movq	-8(%rbp), %rax
	movl	16(%rbp), %r9d
	movl	%edx, %r8d
	movl	%esi, %edx
	movl	%edi, %esi
	movl	%r10d, %edi
	call	*%rax
	leave
	ret
LFE7:
.globl _GasketUint64Uintn
_GasketUint64Uintn:
LFB8:
	pushq	%rbp
LCFI18:
	movq	%rsp, %rbp
LCFI19:
	subq	$48, %rsp
LCFI20:
	movq	%rdi, -24(%rbp)
	movq	%rsi, -32(%rbp)
	movl	%edx, -36(%rbp)
	movq	-24(%rbp), %rax
	movq	%rax, -8(%rbp)
	movl	-36(%rbp), %esi
	movq	-32(%rbp), %rdi
	movq	-8(%rbp), %rax
	call	*%rax
	leave
	ret
LFE8:
.globl _GasketUintnUint64Uintn
_GasketUintnUint64Uintn:
LFB9:
	pushq	%rbp
LCFI21:
	movq	%rsp, %rbp
LCFI22:
	subq	$48, %rsp
LCFI23:
	movq	%rdi, -24(%rbp)
	movl	%esi, -28(%rbp)
	movq	%rdx, -40(%rbp)
	movl	%ecx, -44(%rbp)
	movq	-24(%rbp), %rax
	movq	%rax, -8(%rbp)
	movl	-44(%rbp), %edx
	movq	-40(%rbp), %rsi
	movl	-28(%rbp), %edi
	movq	-8(%rbp), %rax
	call	*%rax
	leave
	ret
LFE9:
.globl _GasketUintnUint16
_GasketUintnUint16:
LFB10:
	pushq	%rbp
LCFI24:
	movq	%rsp, %rbp
LCFI25:
	subq	$32, %rsp
LCFI26:
	movq	%rdi, -24(%rbp)
	movl	%esi, -28(%rbp)
	movw	%dx, -32(%rbp)
	movq	-24(%rbp), %rax
	movq	%rax, -8(%rbp)
	movzwl	-32(%rbp), %esi
	movl	-28(%rbp), %edi
	movq	-8(%rbp), %rax
	call	*%rax
	leave
	ret
LFE10:
	.section __TEXT,__eh_frame,coalesced,no_toc+strip_static_syms+live_support
EH_frame1:
	.set L$set$0,LECIE1-LSCIE1
	.long L$set$0
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
.globl _GasketVoid.eh
_GasketVoid.eh:
LSFDE1:
	.set L$set$1,LEFDE1-LASFDE1
	.long L$set$1
LASFDE1:
	.long	LASFDE1-EH_frame1
	.quad	LFB2-.
	.set L$set$2,LFE2-LFB2
	.quad L$set$2
	.byte	0x0
	.byte	0x4
	.set L$set$3,LCFI0-LFB2
	.long L$set$3
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$4,LCFI1-LCFI0
	.long L$set$4
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE1:
.globl _GasketUintn.eh
_GasketUintn.eh:
LSFDE3:
	.set L$set$5,LEFDE3-LASFDE3
	.long L$set$5
LASFDE3:
	.long	LASFDE3-EH_frame1
	.quad	LFB3-.
	.set L$set$6,LFE3-LFB3
	.quad L$set$6
	.byte	0x0
	.byte	0x4
	.set L$set$7,LCFI3-LFB3
	.long L$set$7
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$8,LCFI4-LCFI3
	.long L$set$8
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE3:
.globl _GasketUintnUintn.eh
_GasketUintnUintn.eh:
LSFDE5:
	.set L$set$9,LEFDE5-LASFDE5
	.long L$set$9
LASFDE5:
	.long	LASFDE5-EH_frame1
	.quad	LFB4-.
	.set L$set$10,LFE4-LFB4
	.quad L$set$10
	.byte	0x0
	.byte	0x4
	.set L$set$11,LCFI6-LFB4
	.long L$set$11
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$12,LCFI7-LCFI6
	.long L$set$12
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE5:
.globl _GasketUintnUintnUintn.eh
_GasketUintnUintnUintn.eh:
LSFDE7:
	.set L$set$13,LEFDE7-LASFDE7
	.long L$set$13
LASFDE7:
	.long	LASFDE7-EH_frame1
	.quad	LFB5-.
	.set L$set$14,LFE5-LFB5
	.quad L$set$14
	.byte	0x0
	.byte	0x4
	.set L$set$15,LCFI9-LFB5
	.long L$set$15
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$16,LCFI10-LCFI9
	.long L$set$16
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE7:
.globl _GasketUintnUintnUintnUintn.eh
_GasketUintnUintnUintnUintn.eh:
LSFDE9:
	.set L$set$17,LEFDE9-LASFDE9
	.long L$set$17
LASFDE9:
	.long	LASFDE9-EH_frame1
	.quad	LFB6-.
	.set L$set$18,LFE6-LFB6
	.quad L$set$18
	.byte	0x0
	.byte	0x4
	.set L$set$19,LCFI12-LFB6
	.long L$set$19
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$20,LCFI13-LCFI12
	.long L$set$20
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE9:
.globl _GasketUintn10Args.eh
_GasketUintn10Args.eh:
LSFDE11:
	.set L$set$21,LEFDE11-LASFDE11
	.long L$set$21
LASFDE11:
	.long	LASFDE11-EH_frame1
	.quad	LFB7-.
	.set L$set$22,LFE7-LFB7
	.quad L$set$22
	.byte	0x0
	.byte	0x4
	.set L$set$23,LCFI15-LFB7
	.long L$set$23
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$24,LCFI16-LCFI15
	.long L$set$24
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE11:
.globl _GasketUint64Uintn.eh
_GasketUint64Uintn.eh:
LSFDE13:
	.set L$set$25,LEFDE13-LASFDE13
	.long L$set$25
LASFDE13:
	.long	LASFDE13-EH_frame1
	.quad	LFB8-.
	.set L$set$26,LFE8-LFB8
	.quad L$set$26
	.byte	0x0
	.byte	0x4
	.set L$set$27,LCFI18-LFB8
	.long L$set$27
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$28,LCFI19-LCFI18
	.long L$set$28
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE13:
.globl _GasketUintnUint64Uintn.eh
_GasketUintnUint64Uintn.eh:
LSFDE15:
	.set L$set$29,LEFDE15-LASFDE15
	.long L$set$29
LASFDE15:
	.long	LASFDE15-EH_frame1
	.quad	LFB9-.
	.set L$set$30,LFE9-LFB9
	.quad L$set$30
	.byte	0x0
	.byte	0x4
	.set L$set$31,LCFI21-LFB9
	.long L$set$31
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$32,LCFI22-LCFI21
	.long L$set$32
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE15:
.globl _GasketUintnUint16.eh
_GasketUintnUint16.eh:
LSFDE17:
	.set L$set$33,LEFDE17-LASFDE17
	.long L$set$33
LASFDE17:
	.long	LASFDE17-EH_frame1
	.quad	LFB10-.
	.set L$set$34,LFE10-LFB10
	.quad L$set$34
	.byte	0x0
	.byte	0x4
	.set L$set$35,LCFI24-LFB10
	.long L$set$35
	.byte	0xe
	.byte	0x10
	.byte	0x86
	.byte	0x2
	.byte	0x4
	.set L$set$36,LCFI25-LCFI24
	.long L$set$36
	.byte	0xd
	.byte	0x6
	.align 3
LEFDE17:
	.subsections_via_symbols
