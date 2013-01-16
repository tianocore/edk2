#------------------------------------------------------------------------------
#
# Copyright (c) 2006 - 2013, Intel Corporation. All rights reserved.<BR>
#
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php.
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#------------------------------------------------------------------------------

ASM_GLOBAL ASM_PFX(JumpToKernel)
ASM_GLOBAL ASM_PFX(JumpToUefiKernel)

#------------------------------------------------------------------------------
# VOID
# EFIAPI
# JumpToKernel (
#   VOID *KernelStart,
#   VOID *KernelBootParams
#   );
#------------------------------------------------------------------------------
ASM_PFX(JumpToKernel):
    movl    0x8(%esp), %esi
    calll   0x4(%esp)
    ret

#------------------------------------------------------------------------------
# VOID
# EFIAPI
# JumpToUefiKernel (
#   EFI_HANDLE ImageHandle,
#   EFI_SYSTEM_TABLE *SystemTable,
#   VOID *KernelBootParams,
#   VOID *KernelStart
#   );
#------------------------------------------------------------------------------
ASM_PFX(JumpToUefiKernel):
    movl    0xc(%esp), %eax
    movl    0x264(%eax), %eax
    addl    0x10(%esp), %eax
    jmp     %eax

