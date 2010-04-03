#------------------------------------------------------------------------------
#
# Copyright (c) 2008-2009 Apple Inc. All rights reserved.
# All rights reserved. This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
# Abstract:
#
# EFI uses a simplified version of the MSFT calling convention, and every one else,
# Mac OS X, BSD, Linux, uses a different one. We can skip dealing with floating point
# other than making sure non volatile registers are preserved. 
#
#   Register for arguments
#   MSFT    Every One Else  
#   ----    --------------
#   rcx     rdi 
#   rdx     rsi
#   r8      rdx
#   r9      rcx
#           r8 
#           r9
#
#   Callee saved registers
#   MSFT    Every One Else  
#   ----    --------------
#   rbx     rbx
#   rbp     rbp
#   r12-r15 r12-r15
#   rsi
#   rdi
#   xmm6-xmm15
#
# cat t.c
##include <stdio.h>
##include <sys/stat.h>
#
#int chmod (int fd, mode_t len){
#  long m = (long)fd;
#}
#
#int Gasketchmod (int fd, mode_t len){
#  return chmod (fd, len);
#}
#
# gcc -arch x86_64  -S t.c
# cat t.s
# this gives you the starting point.... 
#
#
#------------------------------------------------------------------------------

#include <ProcessorBind.h>

  .text
  
#
#
# EFI_UNIX_THUNK_PROTOCOL that gets exported
#
#
  
#------------------------------------------------------------------------------
# VOID GasketmsSleep (unsigned long Milliseconds);
#------------------------------------------------------------------------------    
.globl _GasketmsSleep
_GasketmsSleep:
	pushl	%rbp
	movq  %rsp, %rbp     # does leave use rbp or rsp???
  subq  $148, %rsp
  
  # save registers the OS X will think are volatile
  movaps  %xmm6,    -8(%rbp)
  movaps  %xmm7,   -24(%rbp)
  movaps  %xmm8,   -40(%rbp)
  movaps  %xmm9,   -56(%rbp)
  movaps  %xmm10,  -72(%rbp)
  movaps  %xmm11,  -88(%rbp)
  movaps  %xmm12, -104(%rbp)
  movaps  %xmm13, -120(%rbp)
  movaps  %xmm14, -136(%rbp)
  movaps  %xmm15, -152(%rbp)
  movq    %rsi,   -160(%rbp)
  movq    %rdi,   -168(%rbp)
  
  movq  %rcx, %rdi
	call	_msSleep

  movaps   -8(%rbp), %xmm6,  
  movaps  -24(%rbp), %xmm7  
  movaps  -40(%rbp), %xmm8  
  movaps  -56(%rbp), %xmm9 
  movaps  -72(%rbp), %xmm10 
  movaps  -88(%rbp), %xmm11 
  movaps -104(%rbp), %xmm12 
  movaps -120(%rbp), %xmm13 
  movaps -136(%rbp), %xmm14 
  movaps -152(%rbp), %xmm15 
  movq   -160(%rbp), %rsi   
  movq   -168(%rbp), %rdi   
	
  leave
	ret


#------------------------------------------------------------------------------
# void Gasketexit (int status);
#------------------------------------------------------------------------------
.globl  _Gasketexit
_Gasketexit:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp           # sub extra 0x10 from the stack for the AND
	and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_exit
	leave
	ret


#------------------------------------------------------------------------------
# void GasketSetTimer (UINT64 PeriodMs, VOID (*CallBack)(UINT64 DeltaMs));
#------------------------------------------------------------------------------
.globl  _GasketSetTimer
_GasketSetTimer:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$56, %esp           # sub extra 0x10 from the stack for the AND
	and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
	movl	8(%ebp), %eax
	movl	%eax, -16(%ebp)
	movl	12(%ebp), %eax
	movl	%eax, -12(%ebp)
	movl	16(%ebp), %eax
	movl	%eax, 8(%esp)
	movl	-16(%ebp), %eax
	movl	-12(%ebp), %edx
	movl	%eax, (%esp)
	movl	%edx, 4(%esp)
	call	_SetTimer
	leave
	ret



#------------------------------------------------------------------------------
# void GasketGetLocalTime (EFI_TIME *Time);
#------------------------------------------------------------------------------
.globl  _GasketGetLocalTime
_GasketGetLocalTime:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp           # sub extra 0x10 from the stack for the AND
	and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_GetLocalTime
	leave
	ret



#------------------------------------------------------------------------------
# struct tm *Gasketgmtime (const time_t *clock);
#------------------------------------------------------------------------------
.globl  _Gasketgmtime
_Gasketgmtime:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp           # sub extra 0x10 from the stack for the AND
	and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_gmtime
	leave
	ret




#------------------------------------------------------------------------------
# long GasketGetTimeZone(void);
#------------------------------------------------------------------------------
.globl _GasketGetTimeZone
_GasketGetTimeZone:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$24, %esp           # sub extra 0x10 from the stack for the AND
	and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
	call	_GetTimeZone
	leave
	ret


#------------------------------------------------------------------------------
# int GasketGetDayLight (void);
#------------------------------------------------------------------------------
.globl  _GasketGetDayLight
_GasketGetDayLight:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$24, %esp           # sub extra 0x10 from the stack for the AND
	and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
	call	_GetDayLight
	leave
	ret



#------------------------------------------------------------------------------
# int Gasketpoll (struct pollfd *pfd, int nfds, int timeout);
#------------------------------------------------------------------------------
.globl  _Gasketpoll
_Gasketpoll:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$56, %esp           # sub extra 0x10 from the stack for the AND
	and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
	movl	16(%ebp), %eax
	movl	%eax, 8(%esp)
	movl	12(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_poll
	leave
	ret



#------------------------------------------------------------------------------
# int Gasketread (int fd, void *buf, int count);
#------------------------------------------------------------------------------
.globl  _Gasketread
_Gasketread:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$56, %esp           # sub extra 0x10 from the stack for the AND
	and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
	movl	16(%ebp), %eax
	movl	%eax, 8(%esp)
	movl	12(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_read
	leave
	ret


#------------------------------------------------------------------------------
# int Gasketwrite (int fd, const void *buf, int count);
#------------------------------------------------------------------------------
.globl  _Gasketwrite
_Gasketwrite:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$56, %esp           # sub extra 0x10 from the stack for the AND
	and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
	movl	16(%ebp), %eax
	movl	%eax, 8(%esp)
	movl	12(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_write
	leave
	ret



#------------------------------------------------------------------------------
# char *Gasketgetenv (const char *name);
#------------------------------------------------------------------------------
.globl  _Gasketgetenv
_Gasketgetenv:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp           # sub extra 0x10 from the stack for the AND
	and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_getenv
	leave
	ret



#------------------------------------------------------------------------------
# int Gasketopen (const char *name, int flags, int mode);
#------------------------------------------------------------------------------
.globl  _Gasketopen
_Gasketopen:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$56, %esp           # sub extra 0x10 from the stack for the AND
	and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
	movl	16(%ebp), %eax
	movl	%eax, 8(%esp)
	movl	12(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_open
	leave
	ret



#------------------------------------------------------------------------------
# off_t Gasketlseek (int fd, off_t off, int whence);
#------------------------------------------------------------------------------
.globl  _Gasketlseek
_Gasketlseek:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$56, %esp           # sub extra 0x10 from the stack for the AND
	and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
	movl	12(%ebp), %eax
	movl	%eax, -16(%ebp)
	movl	16(%ebp), %eax
	movl	%eax, -12(%ebp)
	movl	20(%ebp), %eax
	movl	%eax, 12(%esp)
	movl	-16(%ebp), %eax
	movl	-12(%ebp), %edx
	movl	%eax, 4(%esp)
	movl	%edx, 8(%esp)
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_lseek
	leave
	ret



#------------------------------------------------------------------------------
# int Gasketftruncate (int fd, long int len);
#------------------------------------------------------------------------------
.globl  _Gasketftruncate
_Gasketftruncate:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp           # sub extra 0x10 from the stack for the AND
	and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
	movl	12(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_truncate
	leave
	ret



#------------------------------------------------------------------------------
# int Gasketclose (int fd);
#------------------------------------------------------------------------------
.globl  _Gasketclose
_Gasketclose:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp           # sub extra 0x10 from the stack for the AND
	and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_close
	leave
	ret



#------------------------------------------------------------------------------
# int Gasketmkdir (const char *pathname, mode_t mode);
#------------------------------------------------------------------------------
.globl  _Gasketmkdir
_Gasketmkdir:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp           # sub extra 0x10 from the stack for the AND
	and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
	movl	12(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_mkdir
	leave
	ret



#------------------------------------------------------------------------------
# int Gasketrmdir (const char *pathname);
#------------------------------------------------------------------------------
.globl  _Gasketrmdir
_Gasketrmdir:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp           # sub extra 0x10 from the stack for the AND
	and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_rmdir
	leave
	ret



#------------------------------------------------------------------------------
# int Gasketunlink (const char *pathname);
#------------------------------------------------------------------------------
.globl  _Gasketunlink
_Gasketunlink:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp           # sub extra 0x10 from the stack for the AND
	and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_unlink
	leave
	ret



#------------------------------------------------------------------------------
# int GasketGetErrno (void);
#------------------------------------------------------------------------------
.globl  _GasketGetErrno
_GasketGetErrno:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$24, %esp           # sub extra 0x10 from the stack for the AND
	and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
	call	_GetErrno
	leave
	ret



#------------------------------------------------------------------------------
# DIR *Gasketopendir (const char *pathname);
#------------------------------------------------------------------------------
.globl  _Gasketopendir
_Gasketopendir:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp           # sub extra 0x10 from the stack for the AND
	and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_opendir
	leave
	ret



#------------------------------------------------------------------------------
# void *Gasketrewinddir (DIR *dir);
#------------------------------------------------------------------------------
.globl  _Gasketrewinddir
_Gasketrewinddir:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp           # sub extra 0x10 from the stack for the AND
	and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_rewinddir
	leave
	ret



#------------------------------------------------------------------------------
# struct dirent *Gasketreaddir (DIR *dir);
#------------------------------------------------------------------------------
.globl  _Gasketreaddir
_Gasketreaddir:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp           # sub extra 0x10 from the stack for the AND
	and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_readdir
	leave
	ret



#------------------------------------------------------------------------------
# int Gasketclosedir (DIR *dir);
#------------------------------------------------------------------------------
.globl  _Gasketclosedir
_Gasketclosedir:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp           # sub extra 0x10 from the stack for the AND
	and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_closedir
	leave
	ret



#------------------------------------------------------------------------------
# int Gasketstat (const char *path, struct stat *buf);
#------------------------------------------------------------------------------
.globl  _Gasketstat
_Gasketstat:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp           # sub extra 0x10 from the stack for the AND
	and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
	movl	12(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_stat
	leave
	ret



#------------------------------------------------------------------------------
# int Gasketstatfs (const char *path, struct statfs *buf);
#------------------------------------------------------------------------------
.globl  _Gasketstatfs
_Gasketstatfs:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp           # sub extra 0x10 from the stack for the AND
	and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
	movl	12(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_statfs
	leave
	ret




#------------------------------------------------------------------------------
# int Gasketrename (const char *oldpath, const char *newpath);
#------------------------------------------------------------------------------
.globl  _Gasketrename
_Gasketrename:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp           # sub extra 0x10 from the stack for the AND
	and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
	movl	12(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_rename
	leave
	ret




#------------------------------------------------------------------------------
# time_t Gasketmktime (struct tm *tm);
#------------------------------------------------------------------------------
.globl  _Gasketmktime
_Gasketmktime:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp           # sub extra 0x10 from the stack for the AND
	and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_mktime
	leave
	ret



#------------------------------------------------------------------------------
# int Gasketfsync (int fd);
#------------------------------------------------------------------------------
.globl  _Gasketfsync
_Gasketfsync:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp           # sub extra 0x10 from the stack for the AND
	and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_fsync
	leave
	ret



#------------------------------------------------------------------------------
# int Gasketchmod (const char *path, mode_t mode);
#------------------------------------------------------------------------------
.globl  _Gasketchmod
_Gasketchmod:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$56, %esp           # sub extra 0x10 from the stack for the AND
	and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
	movl	12(%ebp), %eax
	movw	%ax, -12(%ebp)
	movzwl	-12(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_chmod
	leave
	ret

#------------------------------------------------------------------------------
# int Gasketutime (const char *filename, const struct utimbuf *buf);
#------------------------------------------------------------------------------
.globl  _Gasketutime
_Gasketutime:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp           # sub extra 0x10 from the stack for the AND
	and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
	movl	12(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_rename
	leave
	ret



#------------------------------------------------------------------------------
# int Gaskettcflush (int fildes, int queue_selector);
#------------------------------------------------------------------------------
.globl  _Gaskettcflush
_Gaskettcflush:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp           # sub extra 0x10 from the stack for the AND
	and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
	movl	12(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_rename
	leave
	ret


#------------------------------------------------------------------------------
# EFI_STATUS UgaCreate (struct _EFI_UNIX_UGA_IO_PROTOCOL **UgaIo, CONST CHAR16 *Title);
#------------------------------------------------------------------------------
.globl _GasketUgaCreate
_GasketUgaCreate:
  pushl   %ebp
  movl    %esp, %ebp
  subl    $40, %esp         #sub extra 0x10 from the stack for the AND
  and     $-16, %esp        # stack needs to end in 0xFFFFFFF0 before call
  movl    12(%ebp), %eax
  movl    %eax, 4(%esp)
  movl    8(%ebp), %eax
  movl    %eax, (%esp)
  call    _UgaCreate
  leave
  ret


#------------------------------------------------------------------------------
# void Gasketperror (__const char *__s);
#------------------------------------------------------------------------------
.globl  _Gasketperror
_Gasketperror:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp           # sub extra 0x10 from the stack for the AND
	and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_perror
	leave
	ret



#------------------------------------------------------------------------------
# int Gasketioctl (int fd, unsigned long int __request, ...);
#
# ... is really int or pointer to structure, so we can treat like an int
#
#------------------------------------------------------------------------------
.globl  _Gasketioctl
_Gasketioctl:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp           # sub extra 0x10 from the stack for the AND
	and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
	movl	16(%ebp), %eax
	movl	%eax, 8(%esp)
	movl	12(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_ioctl
	leave
	ret



#------------------------------------------------------------------------------
# int Gasketfcntl (int __fd, int __cmd, ...);
#
# ... is really int or pointer to structure, so we can treat like an int
#
#------------------------------------------------------------------------------
.globl  _Gasketfcntl
_Gasketfcntl:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp           # sub extra 0x10 from the stack for the AND
	and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
	movl	16(%ebp), %eax
	movl	%eax, 8(%esp)
	movl	12(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_fcntl
	leave
	ret



#------------------------------------------------------------------------------
# int Gasketcfsetispeed (struct termios *__termios_p, speed_t __speed);
#------------------------------------------------------------------------------
.globl  _Gasketcfsetispeed
_Gasketcfsetispeed:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp         # sub extra 0x10 from the stack for the AND
	and   $-16, %esp        # stack needs to end in 0xFFFFFFF0 before call
	movl	12(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_cfsetispeed
	leave
	ret



#------------------------------------------------------------------------------
# int Gasketcfsetospeed (struct termios *__termios_p, speed_t __speed);
#------------------------------------------------------------------------------
.globl  _Gasketcfsetospeed
_Gasketcfsetospeed:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp         # sub extra 0x10 from the stack for the AND
	and   $-16, %esp        # stack needs to end in 0xFFFFFFF0 before call
	movl	12(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_cfsetospeed
	leave
	ret



#------------------------------------------------------------------------------
# int Gaskettcgetattr (int __fd, struct termios *__termios_p); 
#------------------------------------------------------------------------------
.globl  _Gaskettcgetattr
_Gaskettcgetattr:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp         # sub extra 0x10 from the stack for the AND
	and   $-16, %esp        # stack needs to end in 0xFFFFFFF0 before call
	movl	12(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_tcgetattr
	leave
	ret



#------------------------------------------------------------------------------
# int Gaskettcsetattr (int __fd, int __optional_actions, __const struct termios *__termios_p);
#------------------------------------------------------------------------------
.globl  _Gaskettcsetattr
_Gaskettcsetattr:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp           # sub extra 0x10 from the stack for the AND
	and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
	movl	16(%ebp), %eax
	movl	%eax, 8(%esp)
	movl	12(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_tcsetattr
	leave
	ret

#------------------------------------------------------------------------------
# int Gasketsigaction (int sig, const struct sigaction *act, struct sigaction *oact);
#------------------------------------------------------------------------------
.globl  _Gasketsigaction
_Gasketsigaction:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp           # sub extra 0x10 from the stack for the AND
	and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
	movl	16(%ebp), %eax
	movl	%eax, 8(%esp)
	movl	12(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_sigaction
	leave
	ret


#------------------------------------------------------------------------------
# int Gasketsetcontext (const ucontext_t *ucp);
#------------------------------------------------------------------------------
.globl  _Gasketsetcontext
_Gasketsetcontext:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp           # sub extra 0x10 from the stack for the AND
	and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_setcontext
	leave
	ret

#------------------------------------------------------------------------------
# int Gasketgetcontext (ucontext_t *ucp);
#------------------------------------------------------------------------------
.globl  _Gasketgetcontext
_Gasketgetcontext:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp           # sub extra 0x10 from the stack for the AND
	and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_getcontext
	leave
	ret

#------------------------------------------------------------------------------
# int Gasketsigemptyset (sigset_t *set);
#------------------------------------------------------------------------------
.globl  _Gasketsigemptyset
_Gasketsigemptyset:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp           # sub extra 0x10 from the stack for the AND
	and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_sigemptyset
	leave
	ret


#------------------------------------------------------------------------------
# int Gasketsigaltstack (const stack_t *ss, stack_t *oss);
#------------------------------------------------------------------------------
.globl  _Gasketsigaltstack
_Gasketsigaltstack:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp         # sub extra 0x10 from the stack for the AND
	and   $-16, %esp        # stack needs to end in 0xFFFFFFF0 before call
	movl	12(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_sigaltstack
	leave
	ret

#
#
# UGA Functions that get exported
#
#

#------------------------------------------------------------------------------
# EFI_STATUS GasketUgaClose (EFI_UNIX_UGA_IO_PROTOCOL *UgaIo);
#------------------------------------------------------------------------------    
.globl _GasketUgaClose
_GasketUgaClose:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp        # sub extra 0x10 from the stack for the AND
	and   $-16, %esp       # stack needs to end in 0xFFFFFFF0 before call
	movl	8(%ebp), %eax
  movl	%eax, (%esp)
	call	_UgaClose
	leave
	ret

#------------------------------------------------------------------------------
# EFI_STATUS GasketUgaSize (EFI_UNIX_UGA_IO_PROTOCOL *UgaIo, UINT32 Width, UINT32 Height);
#------------------------------------------------------------------------------
.globl _GasketUgaSize
_GasketUgaSize:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp
	and   $-16, %esp       # stack needs to end in 0xFFFFFFF0 before call
	movl	16(%ebp), %eax
	movl	%eax, 8(%esp)
	movl	12(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_UgaSize
	leave
	ret


#------------------------------------------------------------------------------
# EFI_STATUS GasketUgaCheckKey (EFI_UNIX_UGA_IO_PROTOCOL *UgaIo);
#------------------------------------------------------------------------------    
.globl _GasketUgaCheckKey
_GasketUgaCheckKey:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp        # sub extra 0x10 from the stack for the AND
	and   $-16, %esp       # stack needs to end in 0xFFFFFFF0 before call
	movl	8(%ebp), %eax
  movl	%eax, (%esp)
	call	_UgaCheckKey
	leave
	ret

#------------------------------------------------------------------------------
# EFI_STATUS GasketUgaGetKey (EFI_UNIX_UGA_IO_PROTOCOL *UgaIo, EFI_INPUT_KEY *key);
#------------------------------------------------------------------------------
.globl _GasketUgaGetKey
_GasketUgaGetKey:
  pushl   %ebp
  movl    %esp, %ebp
  subl    $40, %esp         #sub extra 0x10 from the stack for the AND
  and     $-16, %esp        # stack needs to end in 0xFFFFFFF0 before call
  movl    12(%ebp), %eax
  movl    %eax, 4(%esp)
  movl    8(%ebp), %eax
  movl    %eax, (%esp)
  call    _UgaGetKey
  leave
  ret


#------------------------------------------------------------------------------
# EFI_STATUS
# GasketUgaBlt(
#    EFI_UNIX_UGA_IO_PROTOCOL *UgaIo,
#    IN  EFI_UGA_PIXEL                           *BltBuffer OPTIONAL,
#    IN  EFI_UGA_BLT_OPERATION                   BltOperation,
#    IN  UINTN                                   SourceX,
#    IN  UINTN                                   SourceY,
#    IN  UINTN                                   DestinationX,
#    IN  UINTN                                   DestinationY,
#    IN  UINTN                                   Width,
#    IN  UINTN                                   Height,
#    IN  UINTN                                   Delta OPTIONAL
#    );
#------------------------------------------------------------------------------
.globl _GasketUgaBlt
_GasketUgaBlt:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$88, %esp         #sub extra 0x10 from the stack for the AND
  and   $-16, %esp        # stack needs to end in 0xFFFFFFF0 before call
	movl	$0, -12(%ebp)
	movl	44(%ebp), %eax
	movl	%eax, 36(%esp)
	movl	40(%ebp), %eax
	movl	%eax, 32(%esp)
	movl	36(%ebp), %eax
	movl	%eax, 28(%esp)
	movl	32(%ebp), %eax
	movl	%eax, 24(%esp)
	movl	28(%ebp), %eax
	movl	%eax, 20(%esp)
	movl	24(%ebp), %eax
	movl	%eax, 16(%esp)
	movl	20(%ebp), %eax
	movl	%eax, 12(%esp)
	movl	16(%ebp), %eax
	movl	%eax, 8(%esp)
	movl	12(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	_UgaBlt
	leave
	ret



