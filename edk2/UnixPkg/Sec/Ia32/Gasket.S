#------------------------------------------------------------------------------
#
# OS X Application requires 16 byte stack alignment. The problem is these
# APIs are exposed to code that does not have this requirement via 
# EFI_UNIX_THUNK_PROTOCOL. So these are wrapper functions that make sure
# the stack is aligned. This code should also work if the stack is already
# aligned. Extra stack padding is really the same as local varaibles so 
# it gets freed by the leave instruction
#
# I basically used the compiler, added extra 16 bytes to the local stack and
# made sure %esp ended in 0 before the call (16 byte algined)
#
# The EFI_UNIX_THUNK_PROTOCOL member functions call a C API that then calls 
# one of these generic assembly routines.  We do it that way to work around 
# some magic name changing that happens in C. For example stat() is _stat() 
# on Leopard and _stat$INDOE64 on Snow Leopard. That is why we pass stat()
# into one of these gaskets from C code.
#
# Copyright (c) 2008 - 2009 Apple Inc. All rights reserved.
# All rights reserved. This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#------------------------------------------------------------------------------

  .text

#------------------------------------------------------------------------------
# int GasketVoid (void *api)
#------------------------------------------------------------------------------    
.globl _GasketVoid
_GasketVoid:
  pushl %ebp
  movl  %esp, %ebp
  subl  $34, %esp           # sub extra 0x10 from the stack for the AND
  and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
  movl  8(%ebp), %eax
  movl  %eax, -12(%ebp)
  movl  -12(%ebp), %eax
  call  *%eax
  leave
  ret

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
.globl _GasketUintn
_GasketUintn:
  pushl %ebp
  movl  %esp, %ebp
  subl  $50, %esp           # sub extra 0x10 from the stack for the AND
  and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
  movl  8(%ebp), %eax
  movl  %eax, -12(%ebp)
  movl  12(%ebp), %eax
  movl  %eax, (%esp)
  movl  -12(%ebp), %eax
  call  *%eax
  leave
  ret

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
.globl _GasketUintnUintn
_GasketUintnUintn:
  pushl %ebp
  movl  %esp, %ebp
  subl  $50, %esp           # sub extra 0x10 from the stack for the AND 
  and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
  movl  8(%ebp), %eax
  movl  %eax, -12(%ebp)
  movl  16(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  12(%ebp), %eax
  movl  %eax, (%esp)
  movl  -12(%ebp), %eax
  call  *%eax
  leave
  ret

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
.globl _GasketUintnUintnUintn
_GasketUintnUintnUintn:
  pushl %ebp
  movl  %esp, %ebp
  subl  $50, %esp           # sub extra 0x10 from the stack for the AND
  and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
  movl  8(%ebp), %eax
  movl  %eax, -12(%ebp)
  movl  20(%ebp), %eax
  movl  %eax, 8(%esp)
  movl  16(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  12(%ebp), %eax
  movl  %eax, (%esp)
  movl  -12(%ebp), %eax
  call  *%eax
  leave
  ret

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
.globl _GasketUintnUintnUintnUintn
_GasketUintnUintnUintnUintn:
  pushl %ebp
  movl  %esp, %ebp
  subl  $50, %esp           # sub extra 0x10 from the stack for the AND
  and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
  movl  8(%ebp), %eax
  movl  %eax, -12(%ebp)
  movl  24(%ebp), %eax
  movl  %eax, 12(%esp)
  movl  20(%ebp), %eax
  movl  %eax, 8(%esp)
  movl  16(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  12(%ebp), %eax
  movl  %eax, (%esp)
  movl  -12(%ebp), %eax
  call  *%eax
  leave
  ret

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
.globl _GasketUintn10Args
_GasketUintn10Args:
  pushl %ebp
  movl  %esp, %ebp
  subl  $82, %esp         # sub extra 0x10 from the stack for the AND
  and   $-16, %esp        # stack needs to end in 0xFFFFFFF0 before call
  movl  8(%ebp), %eax     
  movl  %eax, -12(%ebp)
  movl  48(%ebp), %eax
  movl  %eax, 36(%esp)
  movl  44(%ebp), %eax
  movl  %eax, 32(%esp)
  movl  40(%ebp), %eax
  movl  %eax, 28(%esp)
  movl  36(%ebp), %eax
  movl  %eax, 24(%esp)
  movl  32(%ebp), %eax
  movl  %eax, 20(%esp)
  movl  28(%ebp), %eax
  movl  %eax, 16(%esp)
  movl  24(%ebp), %eax
  movl  %eax, 12(%esp)
  movl  20(%ebp), %eax
  movl  %eax, 8(%esp)
  movl  16(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  12(%ebp), %eax
  movl  %eax, (%esp)
  movl  -12(%ebp), %eax
  call  *%eax
  leave
  ret


#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
.globl _GasketUint64Uintn
_GasketUint64Uintn:
  pushl %ebp
  movl  %esp, %ebp
  subl  $66, %esp           # sub extra 0x10 from the stack for the AND
  and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
  movl  12(%ebp), %eax
  movl  %eax, -32(%ebp)
  movl  16(%ebp), %eax
  movl  %eax, -28(%ebp)
  movl  8(%ebp), %eax
  movl  %eax, -12(%ebp)
  movl  20(%ebp), %eax
  movl  %eax, 8(%esp)
  movl  -32(%ebp), %eax
  movl  -28(%ebp), %edx
  movl  %eax, (%esp)
  movl  %edx, 4(%esp)
  movl  -12(%ebp), %eax
  call  *%eax
  leave
  ret

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
.globl _GasketUintnUint64Uintn
_GasketUintnUint64Uintn:
  pushl %ebp
  movl  %esp, %ebp
  subl  $66, %esp           # sub extra 0x10 from the stack for the AND
  and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
  movl  16(%ebp), %eax
  movl  %eax, -32(%ebp)
  movl  20(%ebp), %eax
  movl  %eax, -28(%ebp)
  movl  8(%ebp), %eax
  movl  %eax, -12(%ebp)
  movl  24(%ebp), %eax
  movl  %eax, 12(%esp)
  movl  -32(%ebp), %eax
  movl  -28(%ebp), %edx
  movl  %eax, 4(%esp)
  movl  %edx, 8(%esp)
  movl  12(%ebp), %eax
  movl  %eax, (%esp)
  movl  -12(%ebp), %eax
  call  *%eax
  leave
  ret

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
.globl _GasketUintnUint16
_GasketUintnUint16:
  pushl %ebp
  movl  %esp, %ebp
  subl  $66, %esp           # sub extra 0x10 from the stack for the AND
  and   $-16, %esp          # stack needs to end in 0xFFFFFFF0 before call
  movl  16(%ebp), %eax
  movw  %ax, -28(%ebp)
  movl  8(%ebp), %eax
  movl  %eax, -12(%ebp)
  movzwl  -28(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  12(%ebp), %eax
  movl  %eax, (%esp)
  movl  -12(%ebp), %eax
  call  *%eax
  leave
  ret

  .subsections_via_symbols
