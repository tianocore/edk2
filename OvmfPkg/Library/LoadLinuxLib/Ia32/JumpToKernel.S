#------------------------------------------------------------------------------
#
# Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
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

