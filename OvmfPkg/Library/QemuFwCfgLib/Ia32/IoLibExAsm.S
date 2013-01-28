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
#    IN UINTN                  Port,
#    IN UINTN                  Size,
#    IN VOID                   *Buffer
#    );
#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(IoReadFifo8)
ASM_PFX(IoReadFifo8):
    movw    4(%esp), %dx
    movl    8(%esp), %ecx
    pushl   %edi
    movl    16(%esp), %edi
rep insb
    popl    %edi
    ret


#------------------------------------------------------------------------------
#  VOID
#  EFIAPI
#  IoWriteFifo8 (
#    IN UINTN                  Port,
#    IN UINTN                  Size,
#    IN VOID                   *Buffer
#    );
#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(IoWriteFifo8)
ASM_PFX(IoWriteFifo8):
    movw    4(%esp), %dx
    movl    8(%esp), %ecx
    pushl   %esi
    movl    16(%esp), %esi
rep outsb
    popl    %esi
    ret

