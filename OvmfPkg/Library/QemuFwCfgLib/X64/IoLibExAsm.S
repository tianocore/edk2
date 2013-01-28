#------------------------------------------------------------------------------
#
# Copyright (c) 2006 - 2013, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php.
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
#  VOID
#  EFIAPI
#  IoReadFifo8 (
#    IN UINTN                  Port,              // rcx
#    IN UINTN                  Size,              // rdx
#    IN VOID                   *Buffer            // r8
#    );
#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(IoReadFifo8)
ASM_PFX(IoReadFifo8):
    xchg    %rcx, %rdx
    xchg    %r8, %rdi           # rdi: buffer address; r8: save rdi
rep insb
    mov     %r8, %rdi           # restore rdi
    ret

#------------------------------------------------------------------------------
#  VOID
#  EFIAPI
#  IoWriteFifo8 (
#    IN UINTN                  Port,              // rcx
#    IN UINTN                  Size,              // rdx
#    IN VOID                   *Buffer            // r8
#    );
#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(IoWriteFifo8)
ASM_PFX(IoWriteFifo8):
    xchg    %rcx, %rdx
    xchg    %r8, %rsi           # rdi: buffer address; r8: save rsi
rep outsb
    mov     %r8, %rsi           # restore rsi
    ret

