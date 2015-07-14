#------------------------------------------------------------------------------
#
# Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
#
# This program and the accompanying materials are licensed and made available
# under the terms and conditions of the BSD License which accompanies this
# distribution.  The full text of the license may be found at
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
#    IN UINTN                  Count,             // rdx
#    IN VOID                   *Buffer            // r8
#    );
#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(IoReadFifo8)
ASM_PFX(IoReadFifo8):
    cld
    xchg    %rcx, %rdx
    xchg    %r8, %rdi           # rdi: buffer address; r8: save register
rep insb
    mov     %r8, %rdi           # restore rdi
    ret

#------------------------------------------------------------------------------
#  VOID
#  EFIAPI
#  IoReadFifo16 (
#    IN UINTN                  Port,              // rcx
#    IN UINTN                  Count,             // rdx
#    IN VOID                   *Buffer            // r8
#    );
#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(IoReadFifo16)
ASM_PFX(IoReadFifo16):
    cld
    xchg    %rcx, %rdx
    xchg    %r8, %rdi           # rdi: buffer address; r8: save register
rep insw
    mov     %r8, %rdi           # restore rdi
    ret

#------------------------------------------------------------------------------
#  VOID
#  EFIAPI
#  IoReadFifo32 (
#    IN UINTN                  Port,              // rcx
#    IN UINTN                  Count,             // rdx
#    IN VOID                   *Buffer            // r8
#    );
#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(IoReadFifo32)
ASM_PFX(IoReadFifo32):
    cld
    xchg    %rcx, %rdx
    xchg    %r8, %rdi           # rdi: buffer address; r8: save register
rep insl
    mov     %r8, %rdi           # restore rdi
    ret

#------------------------------------------------------------------------------
#  VOID
#  EFIAPI
#  IoWriteFifo8 (
#    IN UINTN                  Port,              // rcx
#    IN UINTN                  Count,             // rdx
#    IN VOID                   *Buffer            // r8
#    );
#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(IoWriteFifo8)
ASM_PFX(IoWriteFifo8):
    cld
    xchg    %rcx, %rdx
    xchg    %r8, %rsi           # rsi: buffer address; r8: save register
rep outsb
    mov     %r8, %rsi           # restore rsi
    ret

#------------------------------------------------------------------------------
#  VOID
#  EFIAPI
#  IoWriteFifo16 (
#    IN UINTN                  Port,              // rcx
#    IN UINTN                  Count,             // rdx
#    IN VOID                   *Buffer            // r8
#    );
#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(IoWriteFifo16)
ASM_PFX(IoWriteFifo16):
    cld
    xchg    %rcx, %rdx
    xchg    %r8, %rsi           # rsi: buffer address; r8: save register
rep outsw
    mov     %r8, %rsi           # restore rsi
    ret

#------------------------------------------------------------------------------
#  VOID
#  EFIAPI
#  IoWriteFifo32 (
#    IN UINTN                  Port,              // rcx
#    IN UINTN                  Count,             // rdx
#    IN VOID                   *Buffer            // r8
#    );
#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(IoWriteFifo32)
ASM_PFX(IoWriteFifo32):
    cld
    xchg    %rcx, %rdx
    xchg    %r8, %rsi           # rsi: buffer address; r8: save register
rep outsl
    mov     %r8, %rsi           # restore rsi
    ret

