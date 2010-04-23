#------------------------------------------------------------------------------
#
# Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
# Module Name:
#
#   Monitor.S
#
# Abstract:
#
#   AsmMonitor function
#
# Notes:
#
#------------------------------------------------------------------------------


#------------------------------------------------------------------------------
# UINT64
# EFIAPI
# AsmMonitor (
#   IN      UINTN                     Eax,
#   IN      UINTN                     Ecx,
#   IN      UINTN                     Edx
#   );
#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(AsmMonitor)
ASM_PFX(AsmMonitor):
    mov    %ecx,%eax
    mov    %edx,%ecx
    mov    %r8d,%edx
    monitor
    ret
